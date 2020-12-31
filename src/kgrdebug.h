/*
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGRDEBUG_H
#define KGRDEBUG_H

static int dbgLevel = 0;	// Local to file where kgrdebug.h is included.
#include "kgoldrunner_debug.h"
#define dbk  qCDebug(KGOLDRUNNER_LOG)
#define dbk1 if(dbgLevel>=1)qCDebug(KGOLDRUNNER_LOG)
#define dbk2 if(dbgLevel>=2)qCDebug(KGOLDRUNNER_LOG)
#define dbk3 if(dbgLevel>=3)qCDebug(KGOLDRUNNER_LOG)

#define dbo  printf(
#define dbo1 if(dbgLevel>=1)printf(
#define dbo2 if(dbgLevel>=2)printf(
#define dbo3 if(dbgLevel>=3)printf(

#define dbe  fprintf(stderr,
#define dbe1 if(dbgLevel>=1)fprintf(stderr,
#define dbe2 if(dbgLevel>=2)fprintf(stderr,
#define dbe3 if(dbgLevel>=3)fprintf(stderr,

#endif
