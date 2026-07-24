#ifndef SOUL_PLUGIN_PLUGIN_GLOBAL_H
#define SOUL_PLUGIN_PLUGIN_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(SC_PLUGIN_STATIC_LIB)
#  define SC_PLUGIN_EXPORT
#elif defined(SOUL_PLUGIN_LIBRARY)
#  define SC_PLUGIN_EXPORT Q_DECL_EXPORT
#else
#  define SC_PLUGIN_EXPORT Q_DECL_IMPORT
#endif

#endif