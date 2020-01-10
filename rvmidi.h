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
#ifndef RVMIDI_H
#define RVMIDI_H

#include <QObject>

#include "rvmidi_global.h"

#include "rvmidiportinfo.h"

#include "rvmidievent.h"

#ifdef Q_OS_LINUX
typedef struct _snd_seq snd_seq_t;
typedef snd_seq_t* MidiClientHandle;

Q_DECLARE_METATYPE(RvMidiClientPortId)

#endif

#ifdef Q_OS_MACOS
typedef quint32 MidiClientHandle;
#endif

#ifdef Q_OS_WIN
typedef quint32 MidiClientHandle;
#endif

class RVMIDISHARED_EXPORT RvMidi : public QObject
{
    Q_OBJECT

public:
    RvMidi() = delete;
    explicit RvMidi( const QString &clientName, QEvent::Type qmidieventusertype, QObject *parent = nullptr);

    RvMidi( RvMidi const&) = delete;
    RvMidi& operator=( RvMidi const&) = delete;

    ~RvMidi();

    QEvent::Type QEventMidiType() const { return qeventmiditype; }

    bool connectReadablePort( RvMidiClientPortId portID);
    bool connectWritablePort( RvMidiClientPortId portID);

    bool disconnectReadablePort( RvMidiClientPortId portID);
    bool disconnectWritablePort( RvMidiClientPortId portID);

    bool isConnectedToReadablePort( RvMidiClientPortId portID) const;
    bool isConnectedToWritablePort( RvMidiClientPortId portID) const;

    QList<RvMidiPortInfo> readableMidiPorts() const;
    QList<RvMidiPortInfo> writableMidiPorts() const;

Q_SIGNALS:
    void readablePortConnectionChanged( RvMidiClientPortId portID, bool isConnected);
    void writablePortConnectionChanged( RvMidiClientPortId portID, bool isConnected);

    void portListChanged();

private:
    MidiClientHandle handle;
    RvMidiClientPortId thisInPort;
    RvMidiClientPortId thisOutPort;

    QEvent::Type qeventmiditype{ QEvent::Type::None};
    QObject *midieventreceiver;
#ifdef Q_OS_LINUX
    QList<RvMidiPortInfo> midiPortsAlsa( unsigned int capFilter) const;
    bool subscribeReadablePortAlsa( unsigned char senderClient, unsigned char senderPort);
    bool subscribeWritablePortAlsa( unsigned char destinationClient, unsigned char destinationPort);
#endif
};

#endif // RVMIDI_H
