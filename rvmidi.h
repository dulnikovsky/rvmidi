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

#ifdef Q_OS_LINUX
typedef struct _snd_seq snd_seq_t;
typedef snd_seq_t* MidiClientHandle;

Q_DECLARE_METATYPE(RvMidiClientPortId)

#include <QFuture>
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
    explicit RvMidi( const QString &clientName, QObject *parent = nullptr);

    ~RvMidi();

    QList<RvMidiPortInfo> readableMidiPorts();
    QList<RvMidiPortInfo> writableMidiPorts();

    //TODO
    QSet<RvMidiClientPortId> connectedReadableMidiPortSet();
    QSet<RvMidiClientPortId> connectedWritableMidiPortSet();

public slots:

    bool connectToRedeablePort(RvMidiClientPortId id);

    bool connectToWriteblePort(RvMidiClientPortId id);

    bool disconnectFromReadeablePort(RvMidiClientPortId id);

    bool disconnectFromWriteblePort(RvMidiClientPortId id);

signals:
    void readableMidiPortCreated( RvMidiClientPortId id);

    void writableMidiPortCreated( RvMidiClientPortId id);

    void readableMidiPortDestroyed( RvMidiClientPortId id);

    void writableMidiPortDestroyed( RvMidiClientPortId id);

    void readableMidiPortConnected( RvMidiClientPortId id);

    void writableMidiPortConnected( RvMidiClientPortId id);

    void readableMidiPortDisconnected( RvMidiClientPortId id);

    void writableMidiPortDisconnected( RvMidiClientPortId id);

private:
    MidiClientHandle handle;
    RvMidiClientPortId thisInPort;
    RvMidiClientPortId thisOutPort;

#ifdef Q_OS_LINUX
    QList<RvMidiPortInfo> midiPortsAlsa( unsigned int capFilter);
    QFuture<void>inThreadFuture;
    bool subscribeUnsubscribePort( RvMidiClientPortId srcId,  RvMidiClientPortId destId, bool unsubscribe = false);
#endif
};

#endif // RVMIDI_H
