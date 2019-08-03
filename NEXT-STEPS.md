### convert simple websocket server project

- write CMakeLists.txt for rax
- include rax into mc-universe
- rename examples -> mc-universe
- include SimpleWebSocket-Server into mc-universe

### simple websocket server

I'm abandoning the idea of running this as a redis command set. it's just too dificult to debug. instead, I'm just going to use websockets to connect to the client(s). it already uses threads, so I can do the queries in a threaded way, and lock for writes.

https://github.com/eidheim/Simple-WebSocket-Server

the main disadvantage of this is that now there is no data storage, so (maybe) I'll want to include some components of redis (like the rdb) and store it there. if I just use flat files, then it may not be necessary. the whole point is to simplify this.

instead of commands, it has different endpoints:

#### /grid/:width/:height/:id

each message to this grid is an update frame. upon receipt, it'll accumulate the buffer and run any events it receives.

#### /events

listen for events on this id. every time an event happens, a message will be sent to each client, along with any additional data.




### redis module (c++ code)
- essentials:
  - mc.grid.create <grid_id> <width> <height>
  - mc.grid.event <grid_id> <timestamp> <event>
  - mc.grid.moment <grid_id> <timestamp> <data>
    - adds the latest frame to be processed. can be accompanied with an event.
    - if an event had occurred, then begin the process to search for a sequence
    - run all sequences on the grid and fire any events found
- add custom rax functions to have raxFreeWithCallback and raxHeapSize
- use linear scan w/ inner product for searches. for a small universe, it should be fast enough
- force dimensions to be divisible by 16
- implement the tests instead with `catch` and just connect with hiredis (or ioredis)

### how the grid works

when a grid receives a moment, it accumulates all of the values into its data, for all overflows, this is where a sequence will begin.

the values grid + the overflow grid will then be processed, calling each sequence on it in the universe. each sequence is associated with an event_id

### redis module developer
- make a redis module watcher which executes some things when <path> has changed:
  - `mc.tests` - returns a list of the tests
- after reload, calls each checked test and logs the output
- ability to see server log


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

### d-version?

I liked the idea of writing in d, perhaps. c++ is such an unproductive language. there are optimised vector routines (which supposedly go faster than eigen, too)

https://github.com/libmir/mir-glas

the downside is that c/c++ can be run now on ARM, which means mobiles, so for the future that may be pretty good. eigen is supposedly optimised for arm neon as well, so full mobile support already exists in C++. with d, mir can probably support it (cause it uses llvm under the hood).

android support seems pretty good:
https://wiki.dlang.org/Build_D_for_Android

iOS support is pretty much nonexistent.

### clang stuff

after the project reaches some size do unity builds: https://github.com/sakra/cotire
