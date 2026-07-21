#ifndef SOUL_EXPORT_H
#define SOUL_EXPORT_H

#include "SoulCoreKit_export.h"

#ifndef SC_EXPORT
#  ifdef SOUL_COREKIT_BUILD
#    define SC_EXPORT SOUL_COREKIT_EXPORT
#  else
#    define SC_EXPORT
#  endif
#endif

#endif