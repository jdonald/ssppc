#include <sys/param.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <time.h>

double second()
{
    long sec;
    double secx;
    //struct tms realbuf;
    struct rusage r_usage;
    //times(&realbuf);
    getrusage(RUSAGE_SELF, &r_usage);
    //printf("In second stime is %ld utime is %ld\n",realbuf.tms_stime,realbuf.tms_utime);
   // secx = ( realbuf.tms_stime + realbuf.tms_utime ) / (float) CLK_TCK;
    secx = (double)r_usage.ru_utime.tv_sec + (double)r_usage.ru_utime.tv_usec/1000000 ;
    secx = secx + (double)r_usage.ru_stime.tv_sec + (double)r_usage.ru_stime.tv_usec/1000000;
    return ((double) secx);
    //return (double)((double)r_usage.ru_utime.tv_usec/1000000.00);
}
