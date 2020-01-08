/****************************************************************************
**
** Copyright (C) 2020 Robert Vetter.
**
** This file is part of the RvMidi - a MIDI library for Qt
**
** THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
** ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
** IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
** PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version . The licenses are
** as published by the Free Software Foundation and appearing in the file LICENSE
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**/
#include "rvmidi.h"

#ifdef Q_OS_LINUX
#include <alsa/asoundlib.h>
#include <QtConcurrent/QtConcurrent>
#endif
#ifdef Q_OS_MACOS
#include <AudioToolbox/AudioToolbox.h>
#endif
#ifdef Q_OS_WIN
#include <windows.h>
#include <mmsystem.h>
#endif

#include "rvmidiportinfo_p.h"
#include <QScopedPointer>

#include "rvmidievent.h"

#include <QCoreApplication>

RvMidi::RvMidi( const QString &clientName, QObject *parent)
    :QObject( parent)
{
#ifdef Q_OS_LINUX
    QMetaType::registerConverter( &RvMidiClientPortId::toString);

    int err;
    err = snd_seq_open(&handle, "default", SND_SEQ_OPEN_DUPLEX, 0);
    if( err < 0)
        return;

    int clientID = snd_seq_client_id( handle);
    if( clientID < 0)
        return;

    snd_seq_set_client_name(handle, clientName.toLocal8Bit().constData());

    int inPort = snd_seq_create_simple_port(handle, clientName.toLocal8Bit().constData(),
                                            SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
    int outPort = snd_seq_create_simple_port(handle, clientName.toLocal8Bit().constData(),
                                             SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ, SND_SEQ_PORT_TYPE_MIDI_GENERIC );

    thisInPort = RvMidiClientPortId( static_cast< unsigned char >(clientID), static_cast< unsigned char >(inPort));
    thisOutPort = RvMidiClientPortId( static_cast< unsigned char >(clientID), static_cast< unsigned char >( outPort));

    // Subscribe to the announce port.
    subscribeReadablePortAlsa( SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE);

    QtConcurrent::run([this]
    {
        snd_seq_event_t *ev;
        RvMidiEvent *midievent = nullptr;
        while (snd_seq_event_input(handle, &ev) >= 0)
        {
            if(ev->type==SND_SEQ_EVENT_SYSEX)
            {
                QByteArray arr(static_cast<char *>(ev->data.ext.ptr), static_cast<int>(ev->data.ext.len));
                if( midievent != nullptr)
                {
                    QByteArray *data = midievent->sysExData();
                    data->append(arr);
                }
                else
                {
                    midievent = new RvMidiEvent(static_cast<QEvent::Type>(UserEventTypes::MidiSysEx));
                    quint32 port = ev->source.port;
                    port |= static_cast<quint32>(ev->source.client) << 8;
                    midievent->setPort(port);
                    QByteArray *data = midievent->sysExData();
                    *data=arr;
                }
                if(static_cast<unsigned char>(arr.at(arr.size()-1)) == 0xF7)
                {
                    //QApplication::postEvent(parent(), midievent);
                    midievent=nullptr;
                }
            }
            else if(ev->type==SND_SEQ_EVENT_PGMCHANGE)
            {
                midievent = new RvMidiEvent(static_cast<QEvent::Type>(UserEventTypes::MidiCommon));
                midievent->setStatusByte( (static_cast< unsigned char>( RvMidiEvent::MidiEventType::ProgramChange) << 4) | ( ev->data.raw8.d[0] & 0x0F ) );
                midievent->setData1( ev->data.raw8.d[8] );
                midievent->setData2(0);
                //QApplication::postEvent(parent(), midievent);
            }
            else if(ev->type==SND_SEQ_EVENT_CONTROLLER)
            {
                midievent = new RvMidiEvent(static_cast<QEvent::Type>(UserEventTypes::MidiCommon));
                midievent->setStatusByte( (static_cast< unsigned char>( RvMidiEvent::MidiEventType::ControlChange) << 4) | ( ev->data.raw8.d[0] & 0x0F ) );
                midievent->setData1( static_cast<quint8>( ev->data.control.param));
                midievent->setData2( static_cast<quint8>( ev->data.control.value));
                QCoreApplication::postEvent( this->parent(), midievent, Qt::HighEventPriority);
            }
            else if(ev->type==SND_SEQ_EVENT_PORT_SUBSCRIBED)
            {
                RvMidiClientPortId sender(ev->data.connect.sender.client, ev->data.connect.sender.port);
                RvMidiClientPortId destination(ev->data.connect.dest.client, ev->data.connect.dest.port);
                if( sender == thisOutPort )
                    emit writablePortConnectionChanged( destination, true);
                else if( destination == thisInPort)
                    emit readablePortConnectionChanged( sender, true);

            }
            else if(ev->type==SND_SEQ_EVENT_PORT_UNSUBSCRIBED)
            {
                RvMidiClientPortId sender(ev->data.connect.sender.client, ev->data.connect.sender.port);
                RvMidiClientPortId destination(ev->data.connect.dest.client, ev->data.connect.dest.port);
                if( sender == thisOutPort )
                    emit writablePortConnectionChanged( destination, false);
                else if( destination == thisInPort)
                    emit readablePortConnectionChanged( sender, false);
            }
            else if(ev->type==SND_SEQ_EVENT_PORT_START || ev->type==SND_SEQ_EVENT_PORT_CHANGE)
            {
                //snd_seq_addr_t addr = ev->data.addr;
                emit portListChanged();
            }
            else if(ev->type==SND_SEQ_EVENT_PORT_EXIT)
            {
                //snd_seq_addr_t addr = ev->data.addr;
                emit portListChanged();
            }
            else if(ev->type==SND_SEQ_EVENT_CLIENT_START)
            {
                ;
            }
            else if(ev->type==SND_SEQ_EVENT_CLIENT_EXIT)
            {
                snd_seq_addr_t addr = ev->source;
                if( addr.port == thisOutPort.portId() && addr.client == thisOutPort.clientId())
                    break;
            }
            else if(ev->type==SND_SEQ_EVENT_SENSING)
            {
                //qDebug("got SND_SEQ_EVENT_SENSING");
            }

            qDebug("MIDI Event. Type = %d",ev->type);
            snd_seq_free_event(ev);
        }
    });

#endif
#ifdef Q_OS_MACOS
    MIDIClientCreate( CFStringCreateWithCString( kCFAllocatorDefault, clientName.toLocal8Bit().constData(), kCFStringEncodingASCII), MIDIEngineNotifyProc, this, &handle);
#endif
}

RvMidi::~RvMidi()
{
#ifdef Q_OS_LINUX
    snd_seq_event_t event;
    snd_seq_ev_clear( &event);
    snd_seq_ev_set_subs( &event);
    snd_seq_ev_set_direct( &event);
    event.type = SND_SEQ_EVENT_CLIENT_EXIT;

    event.source.port = thisOutPort.portId();
    event.source.client = thisOutPort.clientId();

    event.dest.port = thisInPort.portId();
    event.dest.client = thisInPort.clientId();

    snd_seq_event_output_direct(handle, &event);
    snd_seq_drain_output( handle);
#endif
}

bool RvMidi::connectReadablePort( RvMidiClientPortId portID)
{
#ifdef Q_OS_LINUX
    return subscribeReadablePortAlsa( portID.clientId(), portID.portId());
#endif
}

bool RvMidi::connectWritablePort( RvMidiClientPortId portID)
{
#ifdef Q_OS_LINUX
    return subscribeWritablePortAlsa( portID.clientId(), portID.portId());
#endif
}

bool RvMidi::disconnectReadablePort(RvMidiClientPortId portID)
{
#ifdef Q_OS_LINUX
    return true;
#endif
}

bool RvMidi::disconnectWritablePort(RvMidiClientPortId portID)
{
#ifdef Q_OS_LINUX
    return true;
#endif
}

bool RvMidi::isConnectedToReadablePort(RvMidiClientPortId portID) const
{
    bool ret = false;

#ifdef Q_OS_LINUX
    int err = 0;
    snd_seq_query_subscribe_t *subsquery;
    snd_seq_query_subscribe_alloca( &subsquery);

    snd_seq_query_subscribe_set_type( subsquery, SND_SEQ_QUERY_SUBS_READ);
    snd_seq_query_subscribe_set_client( subsquery, portID.clientId());
    snd_seq_query_subscribe_set_port( subsquery, portID.portId());

    int i = 0;
    while(1)
    {
        snd_seq_query_subscribe_set_index( subsquery, i);
        err = snd_seq_query_port_subscribers( handle, subsquery);
        if( err < 0)
            break;

        const snd_seq_addr_t *addr;
        addr = snd_seq_query_subscribe_get_addr( subsquery);
        if(addr->client == thisInPort.clientId() && addr->port == thisInPort.portId())
            return true;

        i++;
    }
#endif
    return ret;
}

bool RvMidi::isConnectedToWritablePort(RvMidiClientPortId portID) const
{
    bool ret = false;

#ifdef Q_OS_LINUX
    int err = 0;
    snd_seq_query_subscribe_t *subsquery;
    snd_seq_query_subscribe_alloca( &subsquery);

    snd_seq_query_subscribe_set_type( subsquery, SND_SEQ_QUERY_SUBS_WRITE);
    snd_seq_query_subscribe_set_client( subsquery, portID.clientId());
    snd_seq_query_subscribe_set_port( subsquery, portID.portId());

    int i = 0;
    while(1)
    {
        snd_seq_query_subscribe_set_index( subsquery, i);
        err = snd_seq_query_port_subscribers( handle, subsquery);
        if( err < 0)
            break;

        const snd_seq_addr_t *addr;
        addr = snd_seq_query_subscribe_get_addr( subsquery);
        if(addr->client == thisOutPort.clientId() && addr->port == thisOutPort.portId())
            return true;

        i++;
    }
#endif
    return ret;
}

QList<RvMidiPortInfo> RvMidi::readableMidiPorts() const
{
    QList<RvMidiPortInfo> portlist;
#ifdef Q_OS_LINUX
    portlist = midiPortsAlsa( SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ);
#endif

#ifdef Q_OS_WIN
    unsigned int numdevs = midiInGetNumDevs();
    for( unsigned int i = 0; i<numdevs; i++)
    {
        MIDIINCAPS caps;
        midiInGetDevCaps(i, &caps, sizeof (caps));

        QScopedPointer<RvMidiPortInfoPrivate> info(new RvMidiPortInfoPrivate);

        info->id = i;
        info->name = QString::fromWCharArray( caps.szPname);
        portlist.append(RvMidiPortInfo( *info.take()));
    }
#endif
    return portlist;
}


QList<RvMidiPortInfo> RvMidi::writableMidiPorts() const
{
    QList<RvMidiPortInfo> portlist;
#ifdef Q_OS_LINUX
    portlist = midiPortsAlsa( SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE);
#endif

#ifdef Q_OS_WIN
    unsigned int numdevs = midiOutGetNumDevs();
    for( unsigned int i = 0; i<numdevs; i++)
    {
        MIDIOUTCAPS caps;
        midiOutGetDevCaps(i, &caps, sizeof (caps));

        QScopedPointer<RvMidiPortInfoPrivate> info(new RvMidiPortInfoPrivate);

        info->id = i;
        info->name = QString::fromWCharArray( caps.szPname);
        portlist.append(RvMidiPortInfo( *info.take()));

    }
#endif
    return portlist;
}

#ifdef Q_OS_LINUX
QList<RvMidiPortInfo> RvMidi::midiPortsAlsa(unsigned int capFilter) const
{
    QList<RvMidiPortInfo> portlist;

    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int count;

    snd_seq_client_info_alloca( &cinfo);
    snd_seq_port_info_alloca( &pinfo);
    snd_seq_client_info_set_client( cinfo, -1);
    while (snd_seq_query_next_client( handle, cinfo) >= 0)
    {
        int clientId = snd_seq_client_info_get_client( cinfo);
        qDebug("Client  %3d '%-16s'", clientId, snd_seq_client_info_get_name( cinfo));
        if( (clientId == SND_SEQ_CLIENT_SYSTEM) || ( clientId == snd_seq_client_id(handle)))
            continue;
        /* reset query info */
        snd_seq_port_info_set_client( pinfo, clientId);
        snd_seq_port_info_set_port( pinfo, -1);
        count = 0;
        while (snd_seq_query_next_port( handle, pinfo) >= 0)
        {
            unsigned int cap = snd_seq_port_info_get_capability(pinfo);
            if( cap & capFilter)
            {
                QScopedPointer<RvMidiPortInfoPrivate> info(new RvMidiPortInfoPrivate);
                RvMidiClientPortId mcpId( clientId, snd_seq_port_info_get_port(pinfo));
                info->id = mcpId;
                info->name = QString(snd_seq_port_info_get_name(pinfo));
                info->isVirtual = !(cap & SND_SEQ_PORT_TYPE_HARDWARE);
                portlist.append( RvMidiPortInfo( *info.take()));
            }
            count++;
        }
    }
    return portlist;
}

bool RvMidi::subscribeReadablePortAlsa(unsigned char senderClient, unsigned char senderPort)
{
    int clientID = snd_seq_client_id( handle);
    if( clientID < 0)
        return false;

    snd_seq_port_subscribe_t *subs;
    snd_seq_port_subscribe_alloca( &subs);
    snd_seq_addr_t senderAddr;
    snd_seq_addr_t destAddr;
    senderAddr.client = senderClient;
    senderAddr.port = senderPort;
    destAddr.client = static_cast< unsigned char >( clientID);
    destAddr.port = thisInPort.portId();
    snd_seq_port_subscribe_set_sender( subs, &senderAddr);
    snd_seq_port_subscribe_set_dest( subs, &destAddr);
    int err = snd_seq_subscribe_port( handle, subs);
    return err == 0;
}

bool RvMidi::subscribeWritablePortAlsa( unsigned char destinationClient, unsigned char destinationPort)
{
    int clientID = snd_seq_client_id( handle);
    if( clientID < 0)
        return false;

    snd_seq_port_subscribe_t *subs;
    snd_seq_port_subscribe_alloca( &subs);
    snd_seq_addr_t senderAddr;
    snd_seq_addr_t destAddr;
    senderAddr.client = static_cast< unsigned char >( clientID);
    senderAddr.port = thisOutPort.portId();
    destAddr.client = destinationClient;
    destAddr.port = destinationPort;
    snd_seq_port_subscribe_set_sender( subs, &senderAddr);
    snd_seq_port_subscribe_set_dest( subs, &destAddr);
    int err = snd_seq_subscribe_port( handle, subs);
    return err == 0;
}

#endif

