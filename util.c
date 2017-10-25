#include "misc.h" // jdonald
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

int eq(float x, float y)
{
	return (fabs(x-y) <  DELTA);
}

int le(float x, float y)
{
	return ((x < y) || eq(x,y));
}

int ge(float x, float y)
{
	return ((x > y) || eq(x,y));
}

#ifndef fatal // jdonald avoiding conflict with SimpleScalar
void fatal(char *s)
{
	fprintf(stderr, s);
	exit(1);
}
#endif // fatal
