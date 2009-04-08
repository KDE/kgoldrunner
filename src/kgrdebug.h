#ifndef KGRDEBUG_H
#define KGRDEBUG_H

static int dbgLevel = 0;	// Local to file where kgrdebug.h is included.

#define dbk  kDebug()
#define dbk1 if(dbgLevel>=1)kDebug()
#define dbk2 if(dbgLevel>=2)kDebug()
#define dbk3 if(dbgLevel>=3)kDebug()

#define dbo  printf(
#define dbo1 if(dbgLevel>=1)printf(
#define dbo2 if(dbgLevel>=2)printf(
#define dbo3 if(dbgLevel>=3)printf(

#define dbe  fprintf(stderr,
#define dbe1 if(dbgLevel>=1)fprintf(stderr,
#define dbe2 if(dbgLevel>=2)fprintf(stderr,
#define dbe3 if(dbgLevel>=3)fprintf(stderr,

#endif
