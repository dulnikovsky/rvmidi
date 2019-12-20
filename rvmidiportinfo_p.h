#ifndef RVMIDIPORTINFO_P_H
#define RVMIDIPORTINFO_P_H

#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>

#include "rvmidiclientportid.h"

class RvMidiPortInfoPrivate : public QSharedData {
public:
    RvMidiPortInfoPrivate() { }

    ~RvMidiPortInfoPrivate()
    {
    }

    RvMidiClientPortId id;
    QString name;
    bool isVirtual = false;
};


#endif // RVMIDIPORTINFO_P_H
