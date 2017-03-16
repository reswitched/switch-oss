/*
 * WKCSystem.cpp
 *
 * Copyright(c) 2012, 2013 ACCESS CO., LTD. All rights reserved.
 */

#include "WKCSystem.h"
#include <wkc/wkcclib.h>

namespace WKC {

int
WKCSystemSetTimeZone(int offset, bool isSummerTime)
{
    return wkcSetTimeZonePeer(offset, isSummerTime);
}

}
