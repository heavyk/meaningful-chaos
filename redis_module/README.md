ZRANGE 1 1 - will give me the second element in the sorted set

if I set the first value to always equal the origin, this corresponds to the exactly the cardinal ids associated with the LHS index. so each of these will store a text id usable for the user's storage.

---

### MC.UNIVERSE.CREATE <universe_id> <num dimensions>

create a new universe at key.

ex: create a universe called my_universe with 128 dimensions in it

> mc.universe.create my_universe 128

### MC.UNIVERSE.QUERY <universe_id> <positions[dimensions]> [radius=1] [results=10]

ex: query my_universe for up to ten (nearest) results located within 5.2 units of coordinates 1.4, 2.5, ...

> mc.universe.query my_universe 1.4 2.5 ... 5.2 10

### MC.UNIVERSE.NEAR <universe_id> <radius> <results> [position ...]

in <universe_id>, query for nearest <results> elements within <radius> of [position]. position are not required, as any positions not given are considered as zeros (which doesn't add any distance to the inner product calculation).

### MC.POINT.CREATE <universe_id> <point_id> [pos: float element list]

create a concept with id at position, (any missing dimensions will become random values)

### MC.POINT.POS <universe_id> <point_id>

return the position of id in the universe.

-----
