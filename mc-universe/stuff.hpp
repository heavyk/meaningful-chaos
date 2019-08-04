#ifndef __STUFF_HPP__
#define __STUFF_HPP__

#include <stdio.h>
#include <vector>
#include <string>

using namespace std;

#define UNUSED(V) ((void) V)

#ifdef BUILD_TESTING
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#endif // __STUFF_HPP__
