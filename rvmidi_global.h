#ifndef RVMIDI_GLOBAL_H
#define RVMIDI_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(RVMIDI_LIBRARY)
#  define RVMIDISHARED_EXPORT Q_DECL_EXPORT
#else
#  define RVMIDISHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // RVMIDI_GLOBAL_H
