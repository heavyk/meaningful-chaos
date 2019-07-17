#ifndef __MEANINGFUL_CHAOS_H__
#define __MEANINGFUL_CHAOS_H__

#include "redismodule.h"
#include "redishackery.h"

#ifdef REDIS_TESTS
#include "tests.hpp"
#endif

// increment when changing the rdb data layout
#define RDB_VERSION 1

// #define USE_DOUBLE
#ifdef USE_DOUBLE
typedef double number_t;
#else
typedef float number_t;
#endif

// global constant
static const char* UV_STR = "__UV__";
static RedisModuleString* UV;



typedef struct Point Point;
typedef struct Sector Sector;
typedef struct Universe Universe;

// should these become classes?
struct Point {
    RedisModuleString* id;
    Sector* sector;
    number_t* vector;
};

struct Sector {
    uint32_t num_points;
    uint32_t max_points;
    uint32_t dimensions;
    uint32_t flags;

    number_t* vectors;
    Point** _points;

    Sector* next;
    // Sector* prev;
};

struct Universe {

    uint64_t num_points; // number of points in the universe
    uint64_t max_points; // the maximum number of points the universe can hold

    uint32_t num_sectors; // number of sectors currently allocated
    uint32_t sector_points; // the number of points that are stored in each sector
                            // each time the universe has reached its limit, a new sector with this many points are created

    uint16_t dimensions; // number of dimensions (individual sectors can have a different number of dimensions, because distance function is inner product)
    uint16_t flags; // any flags that need to be set on it

    Sector* sectors_head; // pointer to a linked list of sectors of array of vectors
    Sector* sectors_tail; // pointer to a linked list of sectors of array of vectors

    RedisModuleDict* points; // dict mapping point id a point struct (TODO)

};

#endif // __MEANINGFUL_CHAOS__
