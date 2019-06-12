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
