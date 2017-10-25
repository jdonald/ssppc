#ifndef __UTIL_H
#define __UTIL_H

#include <math.h>

#ifndef MAX // trust the SimpleScalar misc.h's MAX /jdonald
#define MAX(x,y)	(((x)>(y))?(x):(y))
#endif
#ifndef MIN
#define MIN(x,y)	(((x)<(y))?(x):(y))
#endif
#define DELTA		1.0e-10

int eq(float x, float y);
int le(float x, float y);
int ge(float x, float y);
#ifndef fatal // trust the SimpleScalar custom fatal() /jdonald
void fatal (char *s);
#endif

#endif
