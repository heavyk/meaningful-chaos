### redis module (c++ code)
- use inner product for searches. for a small universe, it should be fast enough
- force dimensions to be divisible by 16


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

the downside is that redis can be run now on ARM, which means mobiles, so for the future that may be pretty good. eigen is supposedly optimised for arm neon as well, so full mobile support already exists in C++. with d, mir can probably support it (cause it uses llvm under the hood).

android support seems pretty good:
https://wiki.dlang.org/Build_D_for_Android

iOS support is pretty much nonexistent.
