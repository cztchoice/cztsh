#ifndef CZT_H
#define CZT_H

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/*
void err_quit(char *s){
    fprintf(stderr, "%s %s\n", s, strerror(errno));
    exit(1);
}

void err_sys(char *s){
    fprintf(stderr, "%s %s\n", s, strerror(errno));
}
*/

#define MAXLINE 1000

static void err_doit(int, int, const char*, va_list);


/*
* Fatal error related to system call.
* Print a message and terminate.
*/

void err_sys(const char *fmt, ...)
{

    va_list ap;
    va_start(ap, fmt);
    err_doit(1, errno, fmt, ap);
    va_end(ap);
    exit(1);

}

/*
* Fatal error unrelated to a system call.
* Print message and terminate.
*/
void
err_quit( const char *fmt, ...)
{

    va_list ap;
    va_start(ap, fmt);
    err_doit(0, 0, fmt, ap);
    va_end(ap);
    exit(1);

} 

void err_ret( const char * fmt, ... ){
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, errno, fmt, ap);
    va_end(ap);
}

// void err_output(int errno, const char * fmt, ...){
//     va_list ap;
//     va_start(ap, fmt);
//     err_doit(1, errno, fmt, ap);
//     va_end(ap);
// }

/*
* Print a message and return to caller.
* Caller specifies "errnoflag"
*/

static void
err_doit( int errnoflag, int error, const char *fmt, va_list ap)
{


    char buf[MAXLINE];
    vsnprintf(buf, MAXLINE, fmt, ap);
    if (errnoflag)
        snprintf(buf+strlen(buf), MAXLINE-strlen(buf), ":%s",strerror(error));
    strcat(buf, "\n");
    fflush(stdout);
    fputs(buf, stderr);
    fflush(NULL);
}

#endif //CZT_H
