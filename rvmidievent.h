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
#ifndef RVMIDIEVENT_H
#define RVMIDIEVENT_H

#include <QByteArray>

class RvMidiEvent
{

public:

    enum class Type : quint8 { NoteOff=0x80, NoteOn=0x90, PolyphonicKeyPressure=0xA0, ControlChange=0xB0, ProgramChange=0xC0, ChannelPressure=0xD0, PitchBend=0xE0,
                               System=0xF0, SysEx=System, MtcQuarterFrame, SongPositionPointer, SongSelect, Reserved1, Reserved2, TuneRequest, EndOfSysEx,
                                           TimingClock, Reserved3, Start, Continue, Stop, Reserved4, ActiveSensing, Reset };

    RvMidiEvent() = delete;

    explicit RvMidiEvent(Type type) : port(0)
    {
        if( type == Type::SysEx)
            dataUnion.dataArray = new QByteArray();
        else
            dataUnion.status = static_cast< uint8_t>(type);
    }

    RvMidiEvent( RvMidiEvent const&) = delete;
    RvMidiEvent& operator=( RvMidiEvent const&) = delete;

    ~RvMidiEvent();

    inline Type Type() const
    {
        if( dataUnion.status < static_cast< quint8>(Type::System))
            return static_cast<enum Type> (dataUnion.status & 0xF0);
        else
            return static_cast<enum Type>(dataUnion.status);
    }

    inline quint8 Channel() const { return dataUnion.status & 0x0F; }
    inline void setChannel( quint8 ch) { dataUnion.status = (dataUnion.status & 0xF0) | ( ch & 0x0F); }

    inline quint8 Data1() const { return dataUnion.data1; }
    inline quint8 Data2() const { return dataUnion.data2; }

    inline void setData1( quint8 data) { dataUnion.data1 = data; }
    inline void setData2( quint8 data) { dataUnion.data2 = data; }

    inline quint32 Port() const { return port; }
    inline void setPort(quint32 val) { port=val; }

    inline QByteArray* sysExData()
    {
        Q_ASSERT( Type()==Type::SysEx);
        return dataUnion.dataArray;
    }

private:
    union
    {
        QByteArray *dataArray;
        struct
        {
            quint8 status;
            quint8 data1;
            quint8 data2;
        };
    }dataUnion;
    quint32 port;
};

#endif // RVMIDIEVENT_H
