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
#ifndef RVMIDICLIENTPORTID_H
#define RVMIDICLIENTPORTID_H

#include <QString>

#ifdef Q_OS_MACOS
typedef quint32 RvMidiClientPortId;
#endif

#ifdef Q_OS_WIN
typedef quint32 RvMidiClientPortId;
#endif

#ifdef Q_OS_LINUX
class RvMidiClientPortId
{
public:
    RvMidiClientPortId() {}
    RvMidiClientPortId(const RvMidiClientPortId &other)
    {
        client = other.client;
        port = other.port;
    }
    ~RvMidiClientPortId() {}

    RvMidiClientPortId( unsigned char clientId, unsigned char portId)
        : client(clientId), port(portId) {}

    unsigned char clientId() const  { return client; }
    unsigned char portId() const { return port; }

    friend bool operator ==(const RvMidiClientPortId &a, const RvMidiClientPortId &b);

    friend bool operator <(const RvMidiClientPortId &a, const RvMidiClientPortId &b);

    QString toString() const
    {
        return QString::number(client) + ":" + QString::number(port);
    }

private:
    unsigned char client;
    unsigned char port;
};

uint qHash(const RvMidiClientPortId &id);

bool operator ==(const RvMidiClientPortId &a, const RvMidiClientPortId &b);

#endif

#endif // RVMIDICLIENTPORTID_H
