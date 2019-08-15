#ifndef __GRID_HPP__
#define __GRID_HPP__
#define CATCH_CONFIG_MAIN

#include "common.hpp"
#include "Simple-WebSocket-Server/mutex.hpp"

// @Optimise: replace this with absil's stack vector.
#include <vector>

struct Initialiser {
    Initialiser (u16 _x, u16 _y, u32 _value) :
    origin(_x, _y), value(_value) {};

    // u16 x;
    // u16 y;
    Origin origin;
    u32 value;
};

// @Incomplete: grids need to be reference conunted, because it sounds like more than one connection can listen to the same grid.
// @Incomplete: grids should also be serialised/unserialised on open/close
struct Grid : public std::enable_shared_from_this<Grid> {
    Grid (string id, int width, int height, u16 dimensions = 64) noexcept :
    w(width), h(height), _dimensions(dimensions) {
        px = (grid_t *) calloc(sizeof(grid_t), width * height);
        id = _id;
        cutoff = 255;
    }

    ~Grid () {
        free(px);
    }

    void accumulate (u16* dd, vector<Initialiser*> &inits) {
        // @Incomplete: lock the grid while writing to it
        for (int y = 0; y < height; y++) {
            auto offset = y * width;
            for (int x = 0; x < width; x++) {
                u32 value = px[offset + x];
                u32 inc = dd[offset + x];
                u32 new_value = value + inc;
                // PRINTF("px[%d] = %d + (%d) = %d\n", offset + x, value, inc, new_value);
                if (new_value > cutoff) {
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

    u16 _width, _height;
    u16 _dimensions;
    u16 _cutoff;
    string _id;

    grid_t* _px;
};


#ifdef BUILD_TESTING

TEST_CASE("Grid accumulates properly", "[grid]" ) {
    Grid* grid = new Grid("lala", 8, 8);
    vector<Initialiser*> inits;
    u16 px[8][8] = {{0}};

    // set a few contrived values
    px[0][0] = 65;
    px[1][2] = 129;

    grid->accumulate(*px, inits);
    REQUIRE(*(grid->_px_+0)+0 == 65);
    REQUIRE(*(grid->_px_+(1*8)+2) == 129);
    REQUIRE(inits.size() == 0);

    grid->accumulate(*px, inits);
    REQUIRE(*(grid->_px_+0)+0 == 130);
    REQUIRE(*(grid->_px_+(1*8)+2) == 0);
    REQUIRE(inits.size() == 1);
    REQUIRE(inits[0]->value == 258);
}

#endif // BUILD_TESTING
#endif // __GRID_HPP__
