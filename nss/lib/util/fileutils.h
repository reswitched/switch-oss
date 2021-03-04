/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef _FILEUTILS_H_
#define _FILEUTILS_H_
#pragma once

#include "prtypes.h"
#include "prio.h"
#include "prsystem.h"
#include "plstr.h"

extern char *PORT_fgets(char *str, int size, PRFileDesc *file);
extern int PORT_fputs(char *str, PRFileDesc *file);
extern int PORT_feof(PRFileDesc *file);


#endif    /*  _FILEUTILS_H_  */
