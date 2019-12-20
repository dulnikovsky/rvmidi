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
#include "rvmidiclientportid.h"

#include <QHashFunctions>
#include <tuple>

#ifdef Q_OS_LINUX
uint qHash(const RvMidiClientPortId &id)
{
    qint64 int64 = id.portId();
    int64 &= ((qint64)id.portId() << 32);
    return qHash( int64);
}
bool operator ==(const RvMidiClientPortId &a, const RvMidiClientPortId &b)
{
    return std::tie(a.client, a.port) == std::tie(b.client, b.port);
}
#endif
