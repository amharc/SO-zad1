#ifndef _ERR_
#define _ERR_

/* wypisuje informacje o blednym zakonczeniu funkcji systemowej 
i konczy dzialanie */
extern _Noreturn void syserr(const char *fmt, ...);

/* wypisuje informacje o bledzie i konczy dzialanie */
extern _Noreturn void fatal(const char *fmt, ...);

#endif
