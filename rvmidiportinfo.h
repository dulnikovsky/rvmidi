#ifndef RVMIDIPORTINFO_H
#define RVMIDIPORTINFO_H

#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>

#include "rvmidiclientportid.h"

class RvMidiPortInfoPrivate;

class RvMidiPortInfo
{
public:
   RvMidiPortInfo() = delete;
   RvMidiPortInfo(const RvMidiPortInfo &other);
   ~RvMidiPortInfo();

   void swap(RvMidiPortInfo &other) Q_DECL_NOTHROW
   {
        qSwap(d_ptr, other.d_ptr);
   }

   RvMidiPortInfo &operator=(const RvMidiPortInfo &other);
   RvMidiPortInfo &operator=(RvMidiPortInfo &&other) Q_DECL_NOTHROW
   {
       swap(other);
       return *this;
   }

   RvMidiClientPortId ID() const;
   QString name() const;
   bool isVirtual() const;

   static QList<RvMidiPortInfo> readableMidiPorts();

   static QList<RvMidiPortInfo> writableMidiPorts();

private:
   explicit RvMidiPortInfo(RvMidiPortInfoPrivate &dd);

   QSharedDataPointer<RvMidiPortInfoPrivate> d_ptr;
};

Q_DECLARE_SHARED(RvMidiPortInfo)

#endif // RVMIDIPORTINFO_H
