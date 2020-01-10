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

#include "rvmidievent.h"

RvMidiEvent::~RvMidiEvent()
{
    if( MidiType() == MidiType::SysEx)
        delete dataUnion.dataArray;
    //qDebug("MIDI Event deleted");
}

void RvMidiEvent::copy(const RvMidiEvent &other)
{
    if( other.status != static_cast< quint8>(MidiType::SysEx) && this->status != static_cast< quint8>(MidiType::SysEx))
    {
        this->dataUnion.data1 = other.dataUnion.data1;
        this->dataUnion.data2 = other.dataUnion.data2;
    }
    else if( other.status == static_cast< quint8>(MidiType::SysEx) && this->status != static_cast< quint8>(MidiType::SysEx))
    {
        dataUnion.dataArray = new QByteArray();
        *(this->dataUnion.dataArray) = *(other.dataUnion.dataArray);
    }
    else if( other.status != static_cast< quint8>(MidiType::SysEx) && this->status == static_cast< quint8>(MidiType::SysEx))
    {
        delete dataUnion.dataArray;
        this->dataUnion.data1 = other.dataUnion.data1;
        this->dataUnion.data2 = other.dataUnion.data2;
    }
    else if( other.status == static_cast< quint8>(MidiType::SysEx) && this->status == static_cast< quint8>(MidiType::SysEx))
    {
        *(this->dataUnion.dataArray) = *(other.dataUnion.dataArray);
    }
    else
    {
        this->dataUnion.data1 = other.dataUnion.data1;
        this->dataUnion.data2 = other.dataUnion.data2;
    }

    this->status = other.status;
    this->port = other.port;
}
