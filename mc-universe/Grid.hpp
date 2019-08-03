#ifndef __GRID_HPP__
#define __GRID_HPP__

#include <vector>
#include <string>

using namespace std;

struct Initialiser {
public:
    Initialiser (uint16_t _x, uint16_t _y, uint32_t _value) : x(_x), y(_y), value(_value) {};

    uint16_t x;
    uint16_t y;
    uint32_t value;
};

// @Incomplete: grids need to be reference conunted, because it sounds like more than one connection can listen to the same grid.
// @Incomplete: grids should also be serialised/unserialised on open/close
class Grid : public std::enable_shared_from_this<Grid> {
public:
    Grid (string _id, int _width, int _height) noexcept : width(_width), height(_height) {
        // values = calloc(sizeof(float), width * hight);
        px = (char *) calloc(1, width * height);
        id = _id;
    }

    ~Grid () {
        free(px);
    }

    void accumulate (uint16_t *dd, vector<Initialiser*> &inits) {
        // @Incomplete: lock the grid while writing to it
        for (int y = 0; y < height; y++) {
            auto offset = y * width;
            for (int x = 0; x < width; x++) {
                uint32_t value = px[offset + x];
                uint32_t inc = dd[offset + x];
                uint32_t new_value = value + inc;
                if (new_value > 255) {
                    auto init = new Initialiser(x, y, new_value);
                    inits.push_back(init);
                    new_value = 0;
                }

                px[offset + x] = new_value;
            }
        }

    }

    uint16_t width, height;
    string id;

    // float *values;
    char *px;
};

#ifdef BUILD_TESTING
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    REQUIRE( Factorial(1) == 1 );
    REQUIRE( Factorial(2) == 2 );
    REQUIRE( Factorial(3) == 6 );
    REQUIRE( Factorial(10) == 3628800 );
}

#endif // BUILD_TESTING
#endif // __GRID_HPP__
