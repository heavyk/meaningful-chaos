#ifndef __STUFF_HPP__
#define __STUFF_HPP__

#include <stdio.h>
#include <string>

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

#endif // __STUFF_HPP__
