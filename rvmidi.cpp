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

RvMidi::RvMidi( const QString &clientName, QObject *parent)
    :QObject( parent)
{
#ifdef Q_OS_LINUX
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
        puts ("snd_seq_subscribe_port on the announce port fails: ");
    }
#endif
#ifdef Q_OS_MACOS
    MIDIClientCreate( CFStringCreateWithCString( kCFAllocatorDefault, clientName.toLocal8Bit().constData(), kCFStringEncodingASCII), MIDIEngineNotifyProc, this, &handle);
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

#ifdef Q_OS_LINUX
QList<RvMidiPortInfo> RvMidi::midiPortsAlsa(unsigned int capFilter)
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
#endif
