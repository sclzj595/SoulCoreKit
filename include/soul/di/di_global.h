#ifndef SOUL_DI_DI_GLOBAL_H
#define SOUL_DI_DI_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(SOUL_DI_LIBRARY)
#  define SC_DI_EXPORT Q_DECL_EXPORT
#else
#  define SC_DI_EXPORT Q_DECL_IMPORT
#endif

#endif