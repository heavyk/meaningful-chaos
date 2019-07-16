#include "redismodule.h"
#include "redishackery.h"

#include "meaningful-chaos.hpp"

#include <Eigen/Dense>
#include <random>

using namespace Eigen;




std::uniform_real_distribution<double> udist(-1000, 1000);
std::default_random_engine rande;


static RedisModuleType *Universe_Data;
// static RedisModuleType *Point_Data; // not yet used

typedef Matrix<number_t, Dynamic, 1, ColMajor> Vec;
typedef Matrix<double, Dynamic, 1, ColMajor> d_Vec;

// for now, unused...
// typedef struct {
//     RedisModuleString* id; // textual key of the concept
//     uint16_t flags;
// } Concept;


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


Point* Point_create (RedisModuleString* id, Sector* sector, number_t* vector) {
    Point* p = (Point*) RedisModule_Alloc(sizeof(Point));
    if (!p) return NULL;
    p->id = id;
    p->sector = sector;
    p->vector = vector;

    sector->_points[++sector->num_points] = p;
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

    u->points = RedisModule_CreateDict(NULL);

    return u;
}

// Point* Point_get (Universe* u, RedisModuleString* key) {
//     // if (RedisModule_HashGet(u->points_hash, 0, key) == REDISMODULE_ERR)
//     // RedisModuleKey *p_key = (RedisModuleKey*) RedisModule_OpenKey(ctx, u->points_hash, REDISMODULE_READ);
//     // if (RedisModule_ModuleTypeGetType(key) != Universe_Data)
//     //     return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);
// }

// returns a sector with dimensions >= the universe dimensionality
Sector* Universe_ensure_space (Universe* u, uint64_t more_points) {
    Sector* last = u->sectors_head;
    uint64_t needed_points = u->num_points + more_points;
    while (u->max_points < needed_points || last->dimensions < u->dimensions) {
        Sector* s = Sector_create(u->dimensions, u->sector_points);

        if (!last) {
            // universe does not have any sectors yet
            u->sectors_head = last = s;
        } else {
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
    Sector* s = (Sector*) u->sectors_head;
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
    }

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
        // for each sector, load them:
        int64_t num_points = RedisModule_LoadSigned(rdb);
        int64_t max_points = RedisModule_LoadSigned(rdb);
        int64_t dimensions = RedisModule_LoadUnsigned(rdb);
        int64_t flags = RedisModule_LoadUnsigned(rdb);
        s = Sector_create(dimensions, max_points);
        if (prev) prev->next = s;
        else u->sectors_head = s;
        s->flags = flags;

        for (uint32_t j = 0; j < num_points; j++) {
            size_t offset = j * dimensions;
            auto id = RedisModule_LoadString(rdb);
            Point* p = Point_create(id, s, &s->vectors[offset]);
            assert(p->vector == &s->vectors[offset]);
            RedisModule_DictSet(u->points, id, (void*) p);
            for (uint32_t k = 0; k < dimensions; k++) {
#ifdef USE_DOUBLE
                s->vectors[offset+k] = RedisModule_LoadDouble(rdb);
#else
                s->vectors[offset+k] = RedisModule_LoadFloat(rdb);
#endif
            }
        }

        if (s->num_points != num_points) {
            RedisModule_LogIOError(rdb,"warning","sector was saved with %d points, yet only %d were loaded", num_points, s->num_points);
        }
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

    // RedisModule_DictIteratorStart()
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

        // if (argc > 2) {
        //     if (RedisModule_StringToLongLong(argv[2], &dimensions) == REDISMODULE_ERR || dimensions < 0)
        //         return RedisModule_ReplyWithError(ctx, "ERR: dimensions should be a positive integer");
        // }
        //
        // if (dimensions <= 0) dimensions = 128;
        // if ((unsigned) dimensions > ULONG_MAX) dimensions = ULONG_MAX;
        // if (sector_points <= 0) sector_points = 1024;
        // if ((unsigned) sector_points > ULONG_MAX) sector_points = ULONG_MAX;

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

// MC.NEAR <radius> <results> [positions ...]
int Cmd_Near (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc < 2) RedisModule_WrongArity(ctx);
    RedisModule_AutoMemory(ctx);

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

    VectorXd dquery(dims);
    double* query_ptr = dquery.data();

    for (int i = 0; i < dims; i++) {
        RedisModule_StringToDouble(argv[i + 2], &query_ptr[i]);
    }

    Vec query(dquery.cast<number_t>());

    // in the future, what I could have to have instead of a priority queue, I can store
    // the largest and smallest values. if a new value is greater than the top or
    // greater than the bottom, do something with the value.
    //
    // the elements between the top and bottom would always be unsorted, so sort the vector
    // before returning it, but it would improve insertion times on a large dataset (sometimes).


    return REDISMODULE_ERR; // @Incomplete
}

// MC.POINT.CREATE <point_id> [pos: float(dimensions)]
int Cmd_Point_Create (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc < 2) return RedisModule_WrongArity(ctx);
    RedisModule_AutoMemory(ctx);

    Universe* u = GetUniverse(ctx);
    if (!u) return REDISMODULE_OK;

    auto dims = u->dimensions;
    if (argc > dims + 2) {
        auto err = RedisModule_CreateStringPrintf(ctx, "ERR arity must be <= %d: (given: %d)", dims + 2, argc);
        return RedisModule_ReplyWithError(ctx, RedisModule_StringPtrLen(err, NULL));
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
            vector[i] = udist(rande);
        }
    }

    RedisModuleString* id = argv[1];

    RedisModule_RetainString(ctx, id);
    Point* point = Point_create(id, s, vector);
    RedisModule_DictReplace(u->points, id, point);

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    RedisModule_ReplicateVerbatim(ctx);

    return REDISMODULE_OK;
}

// MC.POINT.POS <point_id>
int Cmd_Point_Pos (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc < 2) return RedisModule_WrongArity(ctx);
    RedisModule_AutoMemory(ctx);

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

#ifdef USE_DOUBLE
    RedisModule_Log(ctx,"warning","number_t is double");
#else
    RedisModule_Log(ctx,"warning","number_t is float");
#endif

    RedisModuleTypeMethods tm = {
        .version = REDISMODULE_TYPE_METHOD_VERSION,
        .rdb_load = MC_UniverseRdbLoad,
        .rdb_save = MC_UniverseRdbSave,
        // .aof_rewrite = MC_UniverseAofRewrite, // disabled. it's not used.
        .free = MC_UniverseFree
    };

    Universe_Data = RedisModule_CreateDataType(ctx, "mc-uverse", RDB_VERSION, &tm);
    if (Universe_Data == NULL) return REDISMODULE_ERR;

    // @Incomplete: select database from argv

    if (RedisModule_CreateCommand(ctx,"mc.near",
        Cmd_Near,"readonly",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.point.create",
        Cmd_Point_Create,"write deny-oom",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.point.pos",
        Cmd_Point_Pos,"readonly",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;


#ifdef TEST
    ADD_TEST_COMMANDS(ctx);
#endif

    return REDISMODULE_OK;
}

#ifdef __cplusplus
}
#endif
