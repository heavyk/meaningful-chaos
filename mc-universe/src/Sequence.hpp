#ifndef __SEQUENCE_HPP__
#define __SEQUENCE_HPP__

#include "common.hpp"
#include "Grid.hpp"
#include "Simple-WebSocket-Server/mutex.hpp"

using LockGuard = SimpleWeb::LockGuard;
using Mutex = SimpleWeb::Mutex;

/*******************************************************\

### input data

the grid px reresents the input to the sequence.
for now it is a uint16_t[width][height],
where width and height are both limited
to be max uint16_t, but they could easily
be 24 or 32 bit sizes, and the value type
could be a float or double.

### a memory address

all "pointers" are relative to the "origin" and are defined as
Offset{int16_t x, int16_t y} - relative modulo position
Origin{uint16_t x, uint16_t y} - the absolute position

### an operation

an operation is a 64 bit data composed of:
- 255 possible operation types (0 = noop)
- 256 possible input layers (0 = grid, others are of a defined type, prolly a float)
- 255 possible output layers (0 = noop)
- pointer

input value is fetched from the origin + the offset
  |-> perform operation (op) on the input
    |-> store result into output layer

(finally), through anther set of contrived operations, reduce the output layers down to an output vector

it occurs to me this would be quite optimisable with a gpu. this current implementaiton (likely to change) does not include any conditionals or branching operations, so a gpu could transform the data in the specified way pretty easily[^1].

the promise of being able to potentially easily optimise for gpus makes the non branching idea a whloe lot more attractive. it also makes the fitness function for the genetic algorithim a whole lot easier to implement.

[1]: if by easily, I mean have each one of the sequences compile down to a shader? I don't really know how opencl or whatever works. I assume they do something similar.

\*******************************************************/

typedef struct Op1 Op;
typedef struct Op1 Op1;
typedef struct Op2 Op2;
typedef struct Op4 Op4;

struct Op1 {
    uint8_t size:2;
    uint8_t op:6;
    uint8_t from; // from which layer is the input to the data
    uint8_t to; // output layer to the data
    uint8_t __reserved__; // unused
};

struct Op2 {
    uint8_t size:2;
    uint8_t op:6;
    uint8_t from; // from which layer is the input to the data
    uint8_t to; // output layer to the data
    uint8_t __reserved__; // unused
    Offset ptr; // intermediate address, if necessary
};

struct Op3 {
    uint8_t size:2;
    uint8_t op:6;
    uint8_t from; // from which layer is the input to the data
    uint8_t to; // output layer to the data
    uint8_t __reserved__; // unused
    Offset ptr1;
    Offset ptr2;
};

struct Op4 {
    uint8_t size:2;
    uint8_t op:6;
    uint8_t from; // from which layer is the input to the data
    uint8_t to; // output layer to the data
    uint8_t __reserved__; // unused
    Offset ptr1;
    Offset ptr2;
    Offset ptr3;
};

enum op_t {
    add,
    sub,
    mul,
    div
};

struct Output {
    float* vec;  // must be freed by the receiever of the output
    float** values; // must be freed by the receiever of the output
    uint16_t dimensions;

    Output(Grid* grid, uint16_t num_layers) :
    dimensions(grid->_dimensions_) {
        vec = (number_t*) calloc(sizeof(number_t), dimensions);
        if (num_layers) values = (number_t**) calloc(sizeof(number_t) * grid->_width_ * grid->_height_, num_layers);
    };

    ~Output() { free(vec); free(values); };
};

template<typename Offset>
size_t get_pos (Offset& offset, Grid* grid) {
    auto x = offset.x;
    auto y = offset.y;
    auto w = grid->w;
    auto h = grid->h;
    while (x > w) x -= w;
    while (x < 0) x += w;
    while (y > h) y -= h;
    while (y < 0) y += h;
    return y * h + x;
}

// template<typename In, typename Out, typename Aux1>
// void op_add (In in, Out out, Aux aux, size_t offset ) {
//     In val = in[offset];
//
// }



struct Sequence : public enable_shared_from_this<Sequence> {
    Sequence (shared_ptr<Grid> grid, uint16_t dimensions) noexcept :
    _grid(grid), _dimensions(dimensions) {
        _num_opts = _max_opts = 0;
    }

    ~Sequence () {
        free(_ops);
    }

    Output run (Initialiser& init) {
        Grid* grid = _grid.get();
        Output out(grid, num_layers);
        Origin pos = init.origin;
        // pos.width = grid.width;
        // pos.height = grid.height;
        UNUSED(init);
        UNUSED(pos);
        // LockGuard(write_mutex); // lock for the duration of this function
        for (uint32_t i = 0; i < num_opts;) {
            const Op cur = ops[i];

            switch (cur.op) {
                case add:
                    const Op2& op = (Op2&) cur;
                    // if (op.from == 0)
                    auto offset = get_offset(pos, grid);


            }

            switch (cur.size) {
                // increment based on its size
                case 4: i++; // fallthrough
                case 3: i++; // fallthrough
                case 2: i++; // fallthrough
                case 1: i++; // fallthrough
            }
        }

        return out;
    }

    shared_ptr<Grid> _grid_;

    Mutex _write_mutex_;

    uint32_t _num_opts_;
    uint32_t _max_opts_;
    Op* _ops_;

    uint16_t _dimensions_;
    uint16_t _num_layers_;
};


#ifdef BUILD_TESTING

// moved to cpp file...

#endif // BUILD_TESTING
#endif // __SEQUENCE_HPP__
