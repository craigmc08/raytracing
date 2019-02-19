#ifndef _UTIL_H
#define _UTIL_H

#include <stdlib.h>

float min(float l, float r) { return l < r ? l : r; }
float max(float l, float r) { return l > r ? l : r; }
float random() { return (float)rand() / RAND_MAX; }

#endif