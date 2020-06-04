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

RvMidi::RvMidi( const QString &clientName, QObject *parent)
    :QObject( parent)
{
#ifdef Q_OS_LINUX
    QMetaType::registerConverter( &RvMidiClientPortId::toString);

    int err;
    err = snd_seq_open(&handle, "default", SND_SEQ_OPEN_DUPLEX, 0);

    snd_seq_set_client_name(handle, clientName.toLocal8Bit().constData());

    int inPort = snd_seq_create_simple_port(handle, clientName.toLocal8Bit().constData(),
                                            SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
    int outPort = snd_seq_create_simple_port(handle, clientName.toLocal8Bit().constData(),
                                             SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ, SND_SEQ_PORT_TYPE_MIDI_GENERIC );

    thisInPort = RvMidiClientPortId( snd_seq_client_id(handle), inPort);
    thisOutPort = RvMidiClientPortId( snd_seq_client_id(handle), outPort);

    // Subscribe to the announce port.
    snd_seq_port_subscribe_t* subs;
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_addr_t announce_sender;
    snd_seq_addr_t announce_dest;
    announce_sender.client = SND_SEQ_CLIENT_SYSTEM;
    announce_sender.port = SND_SEQ_PORT_SYSTEM_ANNOUNCE;
    announce_dest.client = snd_seq_client_id( handle);
    announce_dest.port = thisInPort.portId();
    snd_seq_port_subscribe_set_sender(subs, &announce_sender);
    snd_seq_port_subscribe_set_dest(subs, &announce_dest);
    err = snd_seq_subscribe_port(handle, subs);
    if (err != 0)
    {
        puts( "snd_seq_subscribe_port on the announce port failed!");
    }

    inThreadFuture = QtConcurrent::run([this]
    {

        QMap<RvMidiClientPortId, unsigned int> capacityCacheMap;
        snd_seq_client_info_t *clientinfo;
        snd_seq_client_info_alloca( &clientinfo);
        snd_seq_port_info_t *portinfo;
        snd_seq_port_info_alloca( &portinfo);
        snd_seq_client_info_set_client( clientinfo, -1);
        while (snd_seq_query_next_client( handle, clientinfo) >= 0)
        {
            int clientId = snd_seq_client_info_get_client( clientinfo);
            /* reset query info */
            snd_seq_port_info_set_client( portinfo, clientId);
            snd_seq_port_info_set_port( portinfo, -1);
            while (snd_seq_query_next_port( handle, portinfo) >= 0)
            {
                unsigned int cap = snd_seq_port_info_get_capability(portinfo);
                RvMidiClientPortId mcpId( clientId, snd_seq_port_info_get_port(portinfo));
                capacityCacheMap.insert( mcpId, cap);
            }
        }

        snd_seq_event_t *ev;
        RvMidiEvent *midievent = nullptr;
        while( snd_seq_event_input(handle, &ev) >= 0)
        {
            if( ev->type==SND_SEQ_EVENT_SYSEX)
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
            else if( ev->type==SND_SEQ_EVENT_PGMCHANGE)
            {
                midievent = new RvMidiEvent(static_cast<QEvent::Type>(UserEventTypes::MidiCommon));
                midievent->setStatusByte( (static_cast< unsigned char>( RvMidiEvent::MidiEventType::ProgramChange) << 4) | ( ev->data.raw8.d[0] & 0x0F ) );
                midievent->setData1( ev->data.raw8.d[8] );
                midievent->setData2(0);
                //QApplication::postEvent(parent(), midievent);
            }
            else if( ev->type==SND_SEQ_EVENT_CONTROLLER)
            {
                midievent = new RvMidiEvent(static_cast<QEvent::Type>(UserEventTypes::MidiCommon));
                midievent->setStatusByte( (static_cast< unsigned char>( RvMidiEvent::MidiEventType::ControlChange) << 4) | ( ev->data.raw8.d[0] & 0x0F ) );
                midievent->setData1( ev->data.raw8.d[4]);
                midievent->setData2( ev->data.raw8.d[8]);
                //QApplication::postEvent(parent(), midievent);
            }
            else if( ev->type==SND_SEQ_EVENT_PORT_SUBSCRIBED)
            {
                snd_seq_connect conn = ev->data.connect;
                //emit portConnectionStatusChanged( RvMidiClientPortId(conn.sender.client, conn.sender.port),
                //                                  RvMidiClientPortId(conn.dest.client, conn.dest.port),
                //                                  true);

            }
            else if( ev->type==SND_SEQ_EVENT_PORT_UNSUBSCRIBED)
            {
                snd_seq_connect conn = ev->data.connect;
                //emit portConnectionStatusChanged( RvMidiClientPortId(conn.sender.client, conn.sender.port),
                //                                  RvMidiClientPortId(conn.dest.client, conn.dest.port),
                //                                  false);

            }
            else if(ev->type==SND_SEQ_EVENT_CLIENT_EXIT)
            {
                if(ev->source.client == snd_seq_client_id(handle))
                {
                    snd_seq_free_event(ev);
                    break;
                }
            }
            else if( ev->type==SND_SEQ_EVENT_PORT_START)
            {
                RvMidiClientPortId mcpid( ev->data.addr.client, ev->data.addr.port);

                snd_seq_get_any_port_info(handle, ev->data.addr.client, ev->data.addr.port, portinfo);
                unsigned int cap = snd_seq_port_info_get_capability(portinfo);
                if( (cap & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ)) == (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
                    emit readableMidiPortCreated( mcpid);
                if( (cap & (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE)) == (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE))
                    emit writableMidiPortCreated( mcpid);

                capacityCacheMap.insert( mcpid, cap);

            }
            else if( ev->type==SND_SEQ_EVENT_PORT_EXIT)
            {
                RvMidiClientPortId mcpid( ev->data.addr.client, ev->data.addr.port);
                unsigned int cap = capacityCacheMap.take( mcpid);

                if( (cap & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ)) == (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
                    emit readableMidiPortDestroyed( RvMidiClientPortId( ev->data.addr.client, ev->data.addr.port));
                if( (cap & (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE)) == (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE))
                    emit writableMidiPortDestroyed( RvMidiClientPortId( ev->data.addr.client, ev->data.addr.port));
            }
            else if( ev->type==SND_SEQ_EVENT_SENSING)
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

    inThreadFuture.waitForFinished();
}

bool RvMidi::connectToRedeablePort(RvMidiClientPortId id)
{
#ifdef Q_OS_LINUX
    bool ret =  subscribeUnsubscribePort( id, thisInPort );
    if( ret == true)
    {
        emit( readableMidiPortConnected(id));
    }
    return ret;
#endif
}

bool RvMidi::connectToWriteblePort(RvMidiClientPortId id)
{
#ifdef Q_OS_LINUX
    bool ret =  subscribeUnsubscribePort( thisOutPort, id);
    if( ret == true)
    {
        emit( writableMidiPortConnected(id));
    }
    return ret;
#endif
}

bool RvMidi::disconnectFromReadeablePort(RvMidiClientPortId id)
{
#ifdef Q_OS_LINUX
    bool ret =  subscribeUnsubscribePort( id, thisInPort, true);
    if( ret == true)
    {
        emit( readableMidiPortDisconnected(id));
    }
    return ret;
#endif
}

bool RvMidi::disconnectFromWriteblePort(RvMidiClientPortId id)
{
#ifdef Q_OS_LINUX
    bool ret =  subscribeUnsubscribePort( thisOutPort, id, true);
    if( ret == true)
    {
        emit( writableMidiPortDisconnected(id));
    }
    return ret;
#endif
}

QList<RvMidiPortInfo> RvMidi::readableMidiPorts()
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


QList<RvMidiPortInfo> RvMidi::writableMidiPorts()
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

QSet<RvMidiClientPortId> RvMidi::connectedReadableMidiPortSet()
{
    QSet<RvMidiClientPortId> idList;
// TODO
    return idList;
}
QSet<RvMidiClientPortId> RvMidi::connectedWritableMidiPortSet()
{
    QSet<RvMidiClientPortId> idList;
//TODO
    return idList;
}

#ifdef Q_OS_LINUX
QList<RvMidiPortInfo> RvMidi::midiPortsAlsa(unsigned int capFilter)
{
    QList<RvMidiPortInfo> portlist;

    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;

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

        while (snd_seq_query_next_port( handle, pinfo) >= 0)
        {
            unsigned int cap = snd_seq_port_info_get_capability(pinfo);
            qDebug("Cap=%d", cap);
            if( (cap & capFilter) == capFilter)
            {
                QScopedPointer<RvMidiPortInfoPrivate> info(new RvMidiPortInfoPrivate);
                RvMidiClientPortId mcpId( clientId, snd_seq_port_info_get_port(pinfo));
                info->id = mcpId;
                info->name = QString(snd_seq_port_info_get_name(pinfo));
                info->isVirtual = !(cap & SND_SEQ_PORT_TYPE_HARDWARE);
                portlist.append( RvMidiPortInfo( *info.take()));
            }
        }
    }
    return portlist;
}

bool RvMidi::subscribeUnsubscribePort( RvMidiClientPortId srcId,  RvMidiClientPortId destId, bool unsubscribe)
{
    snd_seq_addr_t sender, dest;
    snd_seq_port_subscribe_t* subs;
    snd_seq_port_subscribe_alloca(&subs);

    sender.client = static_cast<unsigned char>( srcId.clientId());
    sender.port = static_cast<unsigned char>( srcId.portId());
    dest.client = static_cast<unsigned char>( destId.clientId());
    dest.port = static_cast<unsigned char>( destId.portId());

    snd_seq_port_subscribe_set_sender(subs, &sender);
    snd_seq_port_subscribe_set_dest(subs, &dest);
    if( unsubscribe)
        return snd_seq_unsubscribe_port(handle, subs) == 0;
    else
        return snd_seq_subscribe_port(handle, subs) == 0;
}

#endif

