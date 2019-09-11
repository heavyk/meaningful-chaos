### simple websocket server

I'm abandoning the idea of running this as a redis command set. it's just too dificult to debug. instead, I'm just going to use websockets to connect to the client(s). it already uses threads, so I can do the queries in a threaded way, and lock for writes.

https://github.com/eidheim/Simple-WebSocket-Server

the main disadvantage of this is that now there is no data storage, so (maybe) I'll want to import some components of redis (like the rdb?) and store it there. if I just use flat files, then it may not be necessary. instead, I will be using boost multi_index for hash maps and boost serialization (https://www.boost.org/doc/libs/1_70_0/libs/serialization/doc/index.html) to serialise data to the disk. the whole point is to simplify this, and boost provides most of what I need to do it.

so, instead of using redis commands, it has different endpoints which can be connected to. I wanted grid moments to have an unobstructed channel to be processed, such that large grids won't clog up the events pipeline and vice versa.

#### /grid/:width/:height/:id

each message to this grid is an update frame. upon receipt, it'll accumulate the buffer and run any events it receives.

#### /events

listen for events on this id. every time an event happens, a message will be sent to each client, along with any additional data.


### how the grid works

when a grid receives a moment, it accumulates all of the values into its data, for all overflows, this is where a sequence will begin.

the values grid + the overflow grid will then be processed, calling each sequence on it in the universe. each sequence is associated with an event_id


### the important takeaway

the bytecode bit is the future for generating meaningful outputs. the present is to neural net the events to recognise them and then generate actions from it.

------------------------------------
------------------------------------
#           NEXT STEPS
------------------------------------
------------------------------------

### mc-universe
- move the tests out to their own cpp files, so that a header which includes another header does not include both of their tests
- serialise the data using boost
  - https://theboostcpplibraries.com/boost.serialization-archive
  - https://www.boost.org/doc/libs/1_70_0/libs/serialization/doc/index.html

### render process
- add tabs to hide/show the camera
- add a svg or some html div which gets movement from arrow keys / wasd
  - allow those keys to be configurable
// - make a mini-action-logger which sends html key events up to the action-logger


### main process
- connect to the action-logger to receive its events
  - request the renderer process to save the grid for any events received at corresponding timestamps
- sequence add/remove/modify notification
- every frame taken from the camera:
  - run all sequences on it and emit all of the calculated positions
  - store it temporarily w/ the timestamp



### meaningful chaos
- write bytecode which is able to retrieve values from grid px around it and apply some math operation to the trajectory's: power / direction / dimension x's value
- the trajectory should also have an entropy that occurs per step as well. this means that it'll stop eventually


### optimisation
- it may be beneficial to pass the grid to C++, and then in parallel, run the bytecode sequences, then match any positions to concepts, and emit those.
  - an effective way to do this may be with a redis module: once redis receives the grid, it can immediately run all the sequences stored, then calculate the positions
- another option is to do any processing that can be done in assemblyscript (asc)
  - https://assemblyscript.github.io/assemblyscript/examples/game-of-life/
  - https://docs.assemblyscript.org/quick-start

### c++ improvements

- use boost containers: https://www.boost.org/doc/libs/1_70_0/libs/bimap/doc/html/boost_bimap/the_tutorial/controlling_collection_types.html
- abseil-cpp: https://github.com/abseil/abseil-cpp/tree/master/CMake
  - stack traces
  - string join, split, substitute, cat, format, match, replace, etc.
  - use cityhash for hash functions (if it's easy, cause it's not necessary)
  - stack allocated vector for the vector results?
  - inlined_vector for the vector of inits

### d-version?

I liked the idea of writing in d, perhaps. c++ is such an unproductive language. there are optimised vector routines (which supposedly go faster than eigen, too)

https://github.com/libmir/mir-glas

the downside is that c/c++ can be run now on ARM, which means mobiles, so for the future that may be pretty good. eigen is supposedly optimised for arm neon as well, so full mobile support already exists in C++. with d, mir can probably support it (cause it uses llvm under the hood).

android support seems pretty good:
https://wiki.dlang.org/Build_D_for_Android

iOS support is pretty much nonexistent.

### clang stuff

after the project reaches some size do unity builds: https://github.com/sakra/cotire
