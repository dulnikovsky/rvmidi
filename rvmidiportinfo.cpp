#include "rvmidiportinfo.h"
#include "rvmidiportinfo_p.h"

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

#include <QScopedPointer>

/*!
   \class RvMidiPortInfo


   \brief The RvMidiPortInfo provides information about midi ports.
*/

/*!
   Constructs a copy of \a other.
*/
RvMidiPortInfo::RvMidiPortInfo(const RvMidiPortInfo &) = default;

/*!
   Constructs a midi port info from RvMidiPortInfoPrivate \a dd.
   \internal
*/
RvMidiPortInfo::RvMidiPortInfo(RvMidiPortInfoPrivate &dd) :
   d_ptr(&dd)
{
}

/*!
   Destroys the midi port device info.
*/
RvMidiPortInfo::~RvMidiPortInfo() = default;

/*!
   \fn void RvMidiPortInfo::swap(RvMidiPortInfo &other)
   Swaps this midi port info with \a other. This operation is very fast
   and never fails.
*/

/*!
   \fn RvMidiPortInfo &RvMidiPortInfo::operator=(RvMidiPortInfo &&other)

   Move-assigns other to this RvMidiPortInfo instance.
*/

/*!
   Assigns \a other to this CAN bus device info and returns a reference to this
   CAN bus device info.
*/
RvMidiPortInfo &RvMidiPortInfo::operator=(const RvMidiPortInfo &) = default;


/*!
   Returns the midi port Id
*/
RvMidiClientPortId RvMidiPortInfo::ID() const
{
   return d_ptr->id;
}

/*!
   Returns the interface name of this CAN bus interface, e.g. can0.
*/
QString RvMidiPortInfo::name() const
{
   return d_ptr->name;
}

/*!
   Returns true, if the midi port is virtual (i.e. not connected to real
   MIDI hardware).

   If this information is not available, false is returned.
*/
bool RvMidiPortInfo::isVirtual() const
{
   return d_ptr->isVirtual;
}

QList<RvMidiPortInfo> RvMidiPortInfo::readableMidiPorts()
{
    QList<RvMidiPortInfo> portlist;

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
    return portlist;
}

QList<RvMidiPortInfo> RvMidiPortInfo::writableMidiPorts()
{
    QList<RvMidiPortInfo> portlist;

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
    return portlist;
}
