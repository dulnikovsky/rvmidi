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

#include <QEvent>

#include <QByteArray>

class RvMidiEvent : public QEvent
{

public:

    enum class MidiType : quint8 { Invalid=0, NoteOff=0x80, NoteOn=0x90, PolyphonicKeyPressure=0xA0, ControlChange=0xB0, ProgramChange=0xC0, ChannelPressure=0xD0, PitchBend=0xE0,
                               System=0xF0, SysEx=System, MtcQuarterFrame, SongPositionPointer, SongSelect, Reserved1, Reserved2, TuneRequest, EndOfSysEx,
                                           TimingClock, Reserved3, Start, Continue, Stop, Reserved4, ActiveSensing, Reset };

    //RvMidiEvent() {}

    explicit RvMidiEvent(MidiType miditype, QEvent::Type type) : QEvent(type), port(0)
    {
        if( miditype == MidiType::SysEx)
            dataUnion.dataArray = new QByteArray();
        else
            dataUnion.data1 = dataUnion.data2 = 0;

        status = static_cast< uint8_t>(miditype);
    }

    explicit RvMidiEvent(quint8 statusByte, QEvent::Type type) : QEvent(type), port(0)
    {
        if( statusByte == static_cast< quint8>(MidiType::SysEx))
            dataUnion.dataArray = new QByteArray();
        else
            dataUnion.data1 = dataUnion.data2 = 0;

        status = statusByte;
    }

    RvMidiEvent( RvMidiEvent const &other) : QEvent( other)
    {
        copy( other);
    }

    RvMidiEvent& operator=( RvMidiEvent const &other)
    {
        copy( other);
        return *this;
    }

    ~RvMidiEvent();

    inline MidiType MidiType() const
    {
        if( status < static_cast< quint8>( MidiType::System))
            return static_cast<enum MidiType> (status & 0xF0);
        else
            return static_cast<enum MidiType>(status);
    }

    inline quint8 Channel() const { return status & 0x0F; }
    inline void setChannel( quint8 ch) { status = ( status & 0xF0) | ( ch & 0x0F); }

    inline quint8 Data1() const { return dataUnion.data1; }
    inline quint8 Data2() const { return dataUnion.data2; }

    inline void setData1( quint8 data) { dataUnion.data1 = data; }
    inline void setData2( quint8 data) { dataUnion.data2 = data; }

    inline quint32 Port() const { return port; }
    inline void setPort(quint32 val) { port=val; }

    inline QByteArray* sysExData()
    {
        Q_ASSERT( MidiType() == MidiType::SysEx);
        return dataUnion.dataArray;
    }

private:
    union
    {
        QByteArray *dataArray;
        struct
        {
            quint8 data1;
            quint8 data2;
        };
    }dataUnion;
    quint32 port{0};
    quint8 status{static_cast< quint8>(MidiType::Invalid)};

    void copy(RvMidiEvent const &other);
};

#endif // RVMIDIEVENT_H
