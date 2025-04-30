#pragma once

#include "Types/Types.h"

#if __cplusplus

#define SPME_HEADER_TOP \
    extern "C" {
#define SPME_HEADER_BOTTOM \
    }

#else

#define SPME_HEADER_TOP
#define SPME_HEADER_BOTTOM 

typedef unsigned char bool;
#define true 1
#define false 0

#endif
