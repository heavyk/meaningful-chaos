#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <stdio.h>
#include <string>

typedef uint64_t u64;
typedef int64_t s64;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint8_t u8;
typedef int8_t s8;

using namespace std;

#define UNUSED(V) ((void) V)

#ifdef BUILD_TESTING
#include <catch2/catch.hpp>
#define PRINTF(...) printf(__VA_ARGS__)
#define COUT std::cout
#define CERR std::cerr
// #define PRINT(str) do { std::cout << str << std::endl; } while( false )
// #define PRINT(str) do { std::cerr << str << std::endl; } while( false )
#else
#define PRINTF(...)
class log_disabled_output {};
static log_disabled_output log_disabled_output_instance;
template<typename T> log_disabled_output& operator << (log_disabled_output& out, T const&) { return out; }
log_disabled_output& operator << (log_disabled_output& any, std::ostream&(*)(std::ostream&)) { return any; }
#define COUT log_disabled_output_instance
#define CERR log_disabled_output_instance
// #define PRINT(str) do { } while ( false )
// #define PRINT(str) do { } while ( false )
#endif

#include "rc4rand.h"

double random_number (double min = 0.0, double max = 1.0) {
    double val = rc4rand();
    val /= 0xFFFFFFFF;
    if (min != 0 || max != 1.0) {
        double diff = max - min;
        val = val * diff + min;
    }

    return val;
}

/* TEST_CASE("random_number", "") {
    for (int i = 0; i < 1000; i++) {
        auto n = random_number();
        REQUIRE(n >= 0);
        REQUIRE(n <= 1);
    }

    for (int i = 0; i < 1000; i++) {
        auto n = random_number(-1, -2);
        REQUIRE(n >= -2);
        REQUIRE(n <= -1);
    }

    for (int i = 0; i < 1000; i++) {
        auto n = random_number(-1000, 2000);
        REQUIRE(n >= -1000);
        REQUIRE(n <= 2000);
    }
} */

typedef float number_t;
typedef u16 grid_t;

typedef struct Offset Offset;
typedef struct Origin Origin;

struct Offset {
    s16 x;
    s16 y;
};

struct Origin {
    Origin (u16 _x, u16 _y) : x(_x), y(_y) {};
    u16 x;
    u16 y;
};



#endif // __COMMON_HPP__
