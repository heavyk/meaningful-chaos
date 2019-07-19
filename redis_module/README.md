by default, a universe is created in the default database. (TODO) the database can be selected by passing the desired number when loading the module. the universe data structure is created when the module loads at the key: `__UV__` (so don't delete that key, hehe).
---

### MC.STATS

returns an array of statistics the universe has, like x points, x sectors, etc.

### (TODO) MC.CONFIG.GET [field]

returns the universe configuration for this database, or just the value of the field requested.

defaults:
- dimensions 128
- sector_num_elements 1024

### MC.CONFIG.SET <field> <value>

returns the universe configuration for this database.

| key                 | type   | default | description |
|---------------------|:------:|:-------:|-------------|
| dimensions          | uint32 | 128     | number of dimensions new sectors will be created with |
| sector_points | uint32 | 1024    | max number of elements that a new sector will be able to hold (can be resized) |

> mc.config.set dimensions 256

### (TODO) MC.NEAR <radius> <results> [position ...]

query for nearest <results> elements within <radius> of [position]. position are not required, as any positions not given are considered as zeros (which doesn't add any distance to the inner product calculation).

ex: query for up to ten (nearest) results located within 5.2 units of coordinates 1.4, 2.5, ...

> mc.near 5.2 10 1.4 2.5 ...

### MC.POINT.CREATE <point_id> <position:float(dimensions)>

create a point with id at position, (any missing dimensions will become random values)

### MC.POINT.POS <point_id>

return the position of id in the universe.

-----
