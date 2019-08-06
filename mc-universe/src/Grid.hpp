#ifndef __GRID_HPP__
#define __GRID_HPP__
#define CATCH_CONFIG_MAIN

#include "common.hpp"

#include <vector>

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
        px = (uint8_t *) calloc(1, width * height);
        id = _id;
    }

    ~Grid () {
        free(px);
    }

    void accumulate (uint16_t* dd, vector<Initialiser*> &inits) {
        // @Incomplete: lock the grid while writing to it
        for (int y = 0; y < height; y++) {
            auto offset = y * width;
            for (int x = 0; x < width; x++) {
                uint32_t value = px[offset + x];
                uint32_t inc = dd[offset + x];
                uint32_t new_value = value + inc;
                // PRINTF("px[%d] = %d + (%d) = %d\n", offset + x, value, inc, new_value);
                if (new_value > 255) {
                    auto init = new Initialiser(x, y, new_value);
                    inits.push_back(init);
                    // do I save some sort of residual value?
                    // maybe a fraction of the original value?
                    new_value = 0;
                }

                px[offset + x] = new_value;
            }
        }

    }

    uint16_t width, height;
    string id;

    // float *values;
    uint8_t* px;
};

#ifdef BUILD_TESTING

#include "rc4rand.h"

TEST_CASE("Grid accumulates properly", "[grid]" ) {
    Grid* g = new Grid("lala", 8, 8);
    vector<Initialiser*> inits;
    uint16_t px[8][8] = {{0}};

    // set a few contrived values
    px[0][0] = 65;
    px[1][2] = 129;

    g->accumulate(*px, inits);
    REQUIRE(*(g->px+0)+0 == 65);
    REQUIRE(*(g->px+(1*8)+2) == 129);
    REQUIRE(inits.size() == 0);

    g->accumulate(*px, inits);
    REQUIRE(*(g->px+0)+0 == 130);
    REQUIRE(*(g->px+(1*8)+2) == 0);
    REQUIRE(inits.size() == 1);
    REQUIRE(inits[0]->value == 258);
}

#endif // BUILD_TESTING
#endif // __GRID_HPP__
