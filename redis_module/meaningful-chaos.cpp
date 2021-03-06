#include "redismodule.h"
#include "redishackery.h"

#include "meaningful-chaos.hpp"
#include <assert.h>
#include <limits.h>

// #include <Eigen/Dense>
// using namespace Eigen;
// typedef Matrix<number_t, Dynamic, 1, ColMajor> Vec;
// typedef Matrix<double, Dynamic, 1, ColMajor> d_Vec;

float random_float (float a, float b) {
    double random = ((double) rand()) / (double) RAND_MAX;
    double diff = b - a;
    double r = random * diff;
    return (float)(a + r);
}


static RedisModuleType *Universe_Data;
// static RedisModuleType *Point_Data; // not yet used


Grid* Grid_create (RedisModuleString* id, uint16_t width, uint16_t height) {
    Grid* g = (Grid*) RedisModule_Alloc(sizeof(*g));
    if (!g) return NULL;

    g->id = id;
    g->width = width;
    g->height = height;
    g->data = (char*) RedisModule_Calloc(width * height, 1);
    if (!g->data) {
        RedisModule_Free(g);
        return NULL;
    }

    return g;
}


Sector* Sector_create (uint32_t dimensions, uint32_t max_points) {
    Sector* s = (Sector*) RedisModule_Alloc(sizeof(*s));
    if (!s) return NULL;

    s->num_points = 0;
    s->max_points = max_points;
    s->dimensions = dimensions;
    s->flags = 0;

    s->vectors = (number_t*) RedisModule_Calloc(max_points, dimensions * sizeof(number_t));
    s->_points = (Point**) RedisModule_Calloc(max_points, sizeof(Point*));

    return s;
}

Sector* Sector_create (Universe* u) {
    Sector* s = Sector_create(u->dimensions, u->max_points);
    if (!s) return NULL;
    s->universe = u;
    return s;
}


Point* Point_create (Sector* sector, RedisModuleString* id, number_t* vector) {
    Point* p = (Point*) RedisModule_Alloc(sizeof(Point));
    if (!p) return NULL;
    p->id = id;
    p->sector = sector;
    p->vector = vector;

    sector->_points[sector->num_points] = p;
    sector->num_points++;
    sector->universe->num_points++;
    return p;
}

Universe* Universe_create (uint16_t dimensions = 128, uint32_t sector_points = 1024, uint16_t flags = 0) {
    Universe* u = (Universe*) RedisModule_Alloc(sizeof(*u));
    if (!u) return NULL;

    u->num_points = 0;
    u->max_points = 0;
    u->num_sectors = 0;

    u->flags = flags;
    u->dimensions = dimensions;
    u->sector_points = sector_points;

    u->sectors_head = u->sectors_tail = NULL;
    u->points = RedisModule_CreateDict(NULL);

    return u;
}

// returns a sector with dimensions >= the universe dimensionality
Sector* Universe_ensure_space (Universe* u, uint64_t more_points) {
    Sector* last = u->sectors_head;
    uint64_t needed_points = u->num_points + more_points;
    while (u->max_points < needed_points || last->dimensions < u->dimensions) {
        Sector* s = Sector_create(u);

        if (last == NULL) {
            // universe does not have any sectors yet
            u->sectors_head = last = s;
        } else {
            last = s;
            // traverse all of the sectors and set the next to the newly created one.
            // simplify this by storing the prev/tail??
            while ((last = last->next)) {}
            last->next = s;
        }

        u->sectors_tail = s;
        u->max_points += u->sector_points;
    }

    return last;
}

void MC_UniverseRdbSave (RedisModuleIO *rdb, void *value) {
    Universe* u = (Universe*) value;
    RedisModule_SaveUnsigned(rdb, u->dimensions);
    RedisModule_SaveUnsigned(rdb, u->flags);
    // num_points is calculatable
    // max_points is calculatable
    RedisModule_SaveUnsigned(rdb, u->sector_points);
    RedisModule_SaveUnsigned(rdb, u->num_sectors);
    Sector* s = u->sectors_head;
    uint32_t num_sectors = 0;
    while (s) {
        // @Efficiency: save num_points/max_points together
        RedisModule_SaveUnsigned(rdb, s->num_points);
        RedisModule_SaveUnsigned(rdb, s->max_points);
        // @Efficiency: save dimensions/flags together
        RedisModule_SaveUnsigned(rdb, s->dimensions);
        RedisModule_SaveUnsigned(rdb, s->flags);
        auto num_points = s->num_points;
        auto dimensions = s->dimensions;
        for (uint32_t j = 0; j < num_points; j++) {
            Point* p = s->_points[j];
            assert(p);
            RedisModule_SaveString(rdb, p->id);
            assert(&s->vectors[j*dimensions] == p->vector);
            for (uint32_t k = 0; k < dimensions; k++) {
#ifdef USE_DOUBLE
                RedisModule_SaveDouble(rdb, s->vectors[j*dimensions+k]);
#else
                RedisModule_SaveFloat(rdb, s->vectors[j*dimensions+k]);
#endif
            }
        }

        s = s->next;
        num_sectors++;
    }

#ifndef DEBUG
    if (num_sectors == u->num_sectors) {
        RedisModule_LogIOError(rdb,"warning","%d sectors were saved, but the universe only has %d sectors", num_sectors, u->num_sectors);
    }
#else
    assert(num_sectors == u->num_sectors);
#endif
}

void* MC_UniverseRdbLoad (RedisModuleIO *rdb, int encver) {
    if (encver != 1) {
        RedisModule_LogIOError(rdb,"warning","Can't load data with version %d", encver);
        return NULL;
    }

    uint64_t dimensions = RedisModule_LoadUnsigned(rdb);
    uint64_t flags = RedisModule_LoadUnsigned(rdb);
    uint64_t sector_points = RedisModule_LoadUnsigned(rdb);
    uint64_t num_sectors = RedisModule_LoadUnsigned(rdb);
    uint64_t num_points = 0;
    uint64_t max_points = 0;

    Universe* u = Universe_create(dimensions, sector_points, flags);

    Sector* prev = NULL;
    Sector* s = NULL;
    for (uint32_t i = 0; i < num_sectors; i++) {
        uint64_t num_points = RedisModule_LoadUnsigned(rdb);
        uint64_t max_points = RedisModule_LoadUnsigned(rdb);
        uint64_t dimensions = RedisModule_LoadUnsigned(rdb);
        uint64_t flags = RedisModule_LoadUnsigned(rdb);
        s = Sector_create(dimensions, max_points);
        if (!s) {
            RedisModule_LogIOError(rdb,"warning","Can't create sector with %d dimesionns and %d max_points", dimensions, max_points);
            return NULL;
        }
        if (prev) prev->next = s;
        else u->sectors_head = s;
        s->flags = flags;
        s->universe = u; // when creating the sector with non-universe sizes, set the universe ptr

        for (uint32_t j = 0; j < num_points; j++) {
            size_t offset = j * dimensions;
            auto id = RedisModule_LoadString(rdb);
            Point* p = Point_create(s, id, &s->vectors[offset]);
            assert(p->vector == &s->vectors[offset]);
            RedisModule_DictReplace(u->points, id, (void*) p);
            for (uint32_t k = 0; k < dimensions; k++) {
#ifdef USE_DOUBLE
                s->vectors[offset+k] = RedisModule_LoadDouble(rdb);
#else
                s->vectors[offset+k] = RedisModule_LoadFloat(rdb);
#endif
            }
        }

#ifndef DEBUG
        if (s->num_points != num_points) {
            RedisModule_LogIOError(rdb,"warning","sector was saved with %d points, yet only %d were loaded", num_points, s->num_points);
        }
#else
        assert(s->num_points != num_points);
#endif
    }

    u->sectors_tail = s;
    u->num_sectors = num_sectors;
    u->num_points = num_points;
    u->max_points = max_points;

    return u;
}

void MC_UniverseAofRewrite (RedisModuleIO *aof, RedisModuleString *key, void *value) {
    REDISMODULE_NOT_USED(aof);
    REDISMODULE_NOT_USED(key);
    REDISMODULE_NOT_USED(value);
    // NOT USED
#if 0
    Universe* u = (Universe*) value;
    Sector* s = u->sectors_head;
    // RedisModule_EmitAOF(aof, "MC.UNIVERSE.CREATE", "sl", key, u->dimensions);
    while (s) {
        // RedisModule_EmitAOF(aof, "MC.POINT.CREATE", "sl", key, s->value);
        RedisModule_LogIOError(aof, "warning", "UHOH! trying to rewrite a universe with initialised sectors");
        s = s->next;
    }
#endif
}

void Universe_free (Universe *u) {
    Sector *s = u->sectors_head;
    while (s) {
        for (uint32_t i = 0; i < s->num_points; i++) {
            RedisModule_Free(s->_points[i]);
        }

        RedisModule_Free(s->_points);
        RedisModule_Free(s->vectors);
        RedisModule_Free(s = s->next);
    }

    // @Incomplete: implement the raxFreeWithCallback, then also free all of the Points
    RedisModule_FreeDict(NULL, u->points);
    RedisModule_Free(u);
}

void MC_UniverseFree (void *value) {
    Universe_free((Universe*) value);
}

size_t MC_UniverseMemUsage (const void *value) {
    Universe* u = (Universe*) value;
    Sector* s = u->sectors_head;
    // MISSING: u->points dictionary size
    // I implemented it in rax. it's not available in redis though.
    // figure out a hack.
    size_t bytes = sizeof(*u);
    while (s) {
        bytes += sizeof(*s) + s->max_points * sizeof(number_t);
        s = s->next;
    }

    return bytes;
}

// unused
// void MC_UniverseDigest (RedisModuleDigest *md, void *value) {
//     Universe* u = (Universe*) value;
//     Sector* s = u->sectors_head;
//     while (s) {
//         // sum the pieces
//
//         s = s->next;
//     }
// }

Universe* GetUniverse (RedisModuleCtx* ctx) {
    // get universe key
    UV = RedisModule_CreateString(ctx, UV_STR, strlen(UV_STR));
    RedisModuleKey *key = (RedisModuleKey*) RedisModule_OpenKey(ctx, UV, REDISMODULE_READ|REDISMODULE_WRITE);
    auto type = RedisModule_ModuleTypeGetType(key);

    Universe* u;
    if (type == REDISMODULE_KEYTYPE_EMPTY) {
        int64_t sector_points = 1024;
        int64_t dimensions = 128;
        int64_t flags = 0;
        RedisModule_Log(ctx,"debug","creating universe: %d %d", dimensions, sector_points);
        u = Universe_create(dimensions, sector_points, flags);
        RedisModule_ModuleTypeSetValue(key, Universe_Data, u);
        RedisModule_ReplicateVerbatim(ctx);
    } else if (type != Universe_Data) {
        ReplyWithError(ctx, "universe key `%s` is busy. its type is: '%s'", UV_STR, (moduleType*) type->name);
        return NULL;
    } else {
        u = (Universe*) RedisModule_ModuleTypeGetValue(key);
    }

    return u;
}

// MC.STATS [field]
int Cmd_Stats (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc < 1 || argc > 2) RedisModule_WrongArity(ctx);

    Universe* u = GetUniverse(ctx);
    if (!u) return REDISMODULE_OK;

    size_t len = 0;
    const char* key = "*";
    if (argc > 1) key = RedisModule_StringPtrLen(argv[1], &len);

    BEGIN_ARRAY_REPLY();

    if (!len || strncmp(key, "num_points", len) == 0)
        ARRAY_REPLY_STR_INT("num_points", u->num_points);

    if (!len || strncmp(key, "num_sectors", len) == 0)
        ARRAY_REPLY_STR_INT("num_sectors", u->num_sectors);

    END_ARRAY_REPLY();

    return REDISMODULE_OK;
}

// MC.CONFIG.GET [field]
int Cmd_Config_Get (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc > 2) RedisModule_WrongArity(ctx);

    Universe* u = GetUniverse(ctx);
    if (!u) return REDISMODULE_OK;

    size_t len = 0;
    const char* key = argc == 2 ? RedisModule_StringPtrLen(argv[1], &len) : "*";
    if (len == 0 || strncmp(key, "*", len) == 0) {
        BEGIN_ARRAY_REPLY();
        ARRAY_REPLY_OBJ_INT(u, dimensions);
        ARRAY_REPLY_OBJ_INT(u, sector_points);
        END_ARRAY_REPLY();
    } else {
        if (strncmp(key, "dimensions", len) == 0) {
            RedisModule_ReplyWithLongLong(ctx, u->dimensions);
        } else if (strncmp(key, "sector_points", len) == 0) {
            RedisModule_ReplyWithLongLong(ctx, u->sector_points);
        }
    }

    return REDISMODULE_OK;
}


// MC.CONFIG.SET <field> <value>
int Cmd_Config_Set (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc > 3 || argc < 3) RedisModule_WrongArity(ctx);

    Universe* u = GetUniverse(ctx);
    if (!u) return REDISMODULE_OK;

    size_t len;
    const char* key = RedisModule_StringPtrLen(argv[1], &len);
    auto value = argv[2];
    if (strncmp(key, "dimensions", len) == 0) {
        int64_t dimensions;
        RedisModule_StringToLongLong(value, &dimensions);
        if (dimensions <= 0) dimensions = 128;
        if ((unsigned) dimensions > ULONG_MAX) dimensions = ULONG_MAX;
        u->dimensions = dimensions;
    } else if (strncmp(key, "sector_points", len) == 0) {
        int64_t sector_points;
        RedisModule_StringToLongLong(value, &sector_points);
        if (sector_points <= 0) sector_points = 1024;
        if ((unsigned) sector_points > ULONG_MAX) sector_points = ULONG_MAX;
        u->sector_points = sector_points;
    }

    return REDISMODULE_OK;
}

// MC.NEAR <radius> <results> [positions ...]
int Cmd_Near (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc < 2) RedisModule_WrongArity(ctx);

    double radius;
    int64_t results;
    RedisModule_StringToDouble(argv[1], &radius);
    RedisModule_StringToLongLong(argv[2], &results);

    Universe* u = GetUniverse(ctx);
    if (!u) return REDISMODULE_OK;

    auto dims = u->dimensions;
    auto arg_dims = argc - 2;
    if (arg_dims < 2) {
        // auto err = RedisModule_CreateStringPrintf(ctx, "ERR at last two dimensions must be given. (given: %d, max: %2)", dims + 2, argc);
        // return RedisModule_ReplyWithError(ctx, RedisModule_StringPtrLen(err, NULL));
        return ReplyWithError(ctx, "ERR at last two dimensions must be given. (given: %d, max: %2)", arg_dims, dims);
    }

    if (arg_dims > dims) {
        return ReplyWithError(ctx, "ERR you passed %d dimensions. the universe is only %d dimensional.", arg_dims, dims);
    }

    // VectorXd dquery(dims);
    // double* query_ptr = dquery.data();
    double* query_ptr = (double*) RedisModule_Calloc(sizeof(double), dims);

    for (int i = 0; i < dims; i++) {
        RedisModule_StringToDouble(argv[i + 2], &query_ptr[i]);
    }

    // Vec query(dquery.cast<number_t>());

    // in the future, what I could have to have instead of a priority queue, I can store
    // the largest and smallest values. if a new value is greater than the top or
    // greater than the bottom, do something with the value.
    //
    // the elements between the top and bottom would always be unsorted, so sort the vector
    // before returning it, but it would improve insertion times on a large dataset (sometimes).

    RedisModule_Free(query_ptr);

    return REDISMODULE_ERR; // @Incomplete
}

// MC.POINT.CREATE <point_id> [pos: float(dimensions)]
int Cmd_Point_Create (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc < 2) return RedisModule_WrongArity(ctx);

    Universe* u = GetUniverse(ctx);
    if (!u) return REDISMODULE_OK;

    auto dims = u->dimensions;
    if (argc > dims + 2) {
        return ReplyWithError(ctx, "ERR arity must be <= %d: (given: %d)", dims + 2, argc);
    }

    Sector* s = Universe_ensure_space(u, 1);
    dims = s->dimensions; // just in case the sector is different dimensionality than the universe

    number_t* vector = &s->vectors[s->num_points * dims];

    for (int i = 0; i < dims; i++) {
        int arg = i + 2;
        if (arg < argc) {
#ifdef USE_DOUBLE
            RedisModule_StringToDouble(argv[arg], &vector[i]);
#else
            double value;
            RedisModule_StringToDouble(argv[arg], &value);
            vector[i] = (number_t) value;
#endif
        } else {
            vector[i] = random_float(-1000, 1000);
        }
    }

    const bool overwrite = false;

    // RedisModule_Log(ctx,"warning","string: type = %d, refcount = %d, enctype = %d\n", argv[1]->type, argv[1]->refcount, argv[1]->encoding);

    // RedisModuleString* id = argv[1]; //RedisModule_CreateStringFromString(NULL, argv[1]);
    RedisModuleString* id = RedisModule_CreateStringFromString(NULL, argv[1]);
    Point* point = (Point*) RedisModule_DictGet(u->points, id, NULL);

    if (point && !overwrite) {
        RedisModule_FreeString(ctx, id);
        RedisModule_ReplyWithError(ctx, "point already exists");
    } else {
        point = Point_create(s, id, vector);
        RedisModule_RetainString(NULL, id);
        RedisModule_DictReplace(u->points, id, point);
        RedisModule_ReplyWithSimpleString(ctx, "OK");
        RedisModule_ReplicateVerbatim(ctx);
    }

    return REDISMODULE_OK;
}

// MC.POINT.POS <point_id>
int Cmd_Point_Pos (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc < 2) return RedisModule_WrongArity(ctx);

    Universe* u = GetUniverse(ctx);
    if (!u) return REDISMODULE_OK;

    RedisModuleString* id = argv[1];

    Point* p = (Point*) RedisModule_DictGet(u->points, id, NULL);
    RedisModule_Log(ctx,"warning","point: %x", p);
    if (!p) return RedisModule_ReplyWithNull(ctx);

    uint32_t dims = p->sector->dimensions;
    RedisModule_ReplyWithArray(ctx, dims);
    for (uint32_t i = 0; i < dims; i++) {
        RedisModule_ReplyWithDouble(ctx, (double) p->vector[i]);
    }

    return REDISMODULE_OK;
}

// MC.GRID.CREATE <grid_id> <width> <height>
int Cmd_Grid_Create (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc < 3) return RedisModule_WrongArity(ctx);

    RedisModuleString* id = RedisModule_CreateStringFromString(ctx, argv[1]);
    // RedisModule_RetainString(ctx, id);

    int64_t width, height;
    RedisModule_StringToLongLong(argv[2], &width);
    RedisModule_StringToLongLong(argv[3], &height);

    Grid* g = Grid_create(id, width, height);
    if (!g) return REDISMODULE_OK;

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    RedisModule_ReplicateVerbatim(ctx);

    return REDISMODULE_OK;
}

// mapping flat arrays to eigan vectors:
// https://eigen.tuxfamily.org/dox/group__TutorialMapClass.html

// inner product -- eg. if I search for a radius of 10, then I don't need to sqrt all the results if I just get the dot product of the vectors which are less than 100
// https://math.stackexchange.com/questions/1236465/euclidean-distance-and-dot-product

#ifdef __cplusplus
extern "C" {
#endif

int RedisModule_OnLoad (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    if (RedisModule_Init(ctx, "meaningful-chaos", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    RedisModuleTypeMethods tm = {
        .version = REDISMODULE_TYPE_METHOD_VERSION,
        .rdb_load = MC_UniverseRdbLoad,
        .rdb_save = MC_UniverseRdbSave,
        // .aof_rewrite = MC_UniverseAofRewrite, // disabled. it's not used.
        // .digest = MC_UniverseDigest, // digest functions exist in module.c, and there is an example, but they appear not to be exported or something.
        .free = MC_UniverseFree
    };

    Universe_Data = RedisModule_CreateDataType(ctx, "mc-uverse", RDB_VERSION, &tm);
    if (Universe_Data == NULL) return REDISMODULE_ERR;

    if (argc > 1) {
        int64_t db;
        RedisModule_StringToLongLong(argv[1], &db);
        RedisModule_SelectDb(ctx, (int) db);
        RedisModule_Log(ctx,"info","selected database: %d", (int) db);
    }

    if (RedisModule_CreateCommand(ctx,"mc.near",
        Cmd_Near,"readonly",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.point.create",
        Cmd_Point_Create,"write deny-oom",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.point.pos",
        Cmd_Point_Pos,"readonly",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.stats",
        Cmd_Stats,"readonly",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.config.get",
        Cmd_Config_Get,"readonly",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.config.set",
        Cmd_Config_Set,"readonly",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;


#ifdef REDIS_TESTS
    RedisModule_Log(ctx,"warning","add tests");
    return ADD_TEST_COMMANDS(ctx);
#else
    RedisModule_Log(ctx,"warning","no tests");
    return REDISMODULE_OK;
#endif
}

#ifdef __cplusplus
}
#endif
