#include "redismodule.h"
// #include "falconn/lsh_nn_table.h"
// #include "falconn/falconn.pch"
// #include "hnswlib/hnswlib.h"

#include <string.h>
#include <stdlib.h>

#include <Eigen/Dense>
#include <random>

using namespace Eigen;


std::uniform_real_distribution<double> udist(-1000, 1000);
std::default_random_engine rande;

#define RDB_VERSION 1

// #define USE_DOUBLE
#ifdef USE_DOUBLE
typedef double number_t;
#else
typedef float number_t;
#endif


#define AUTO_MEMORY() ctx->flags |= REDISMODULE_CTX_AUTO_MEMORY


static RedisModuleType *Universe_Data;
// static RedisModuleType *Point_Data; // not yet used
// uint64_t Universe_next_id = 1; // next universe unique id

typedef Matrix<number_t, Dynamic, 1, ColMajor> Point;
typedef Matrix<double, Dynamic, 1, ColMajor> d_Point;

// for now, unused...
// typedef struct {
//     RedisModuleString* id; // textual key of the concept
//     uint16_t flags;
// } Concept;

typedef struct Sector {
    uint32_t count;
    uint32_t max;
    uint32_t dimensions;
    uint32_t flags;

    number_t* points;
    RedisModuleString** ids;
    RedisModuleDict* id2point;

    Sector* next;
    // Sector* prev;
} Sector;

Sector* Sector_create (uint32_t dimensions, uint32_t num_points) {
    Sector* s = (Sector*) RedisModule_Calloc(1, sizeof(*s));

    s->dimensions = dimensions;
    s->max = num_points;
    s->points = (number_t*) RedisModule_Calloc(num_points, dimensions * sizeof(number_t));
    s->ids = (RedisModuleString**) RedisModule_Calloc(num_points, sizeof(RedisModuleString*));
    s->id2point = RedisModule_CreateDict(NULL);
    return s;
}

// should these just become classes?
typedef struct Universe {

    uint64_t num_points; // number of points in the universe
    uint64_t max_points; // the maximum number of points the universe can hold

    uint32_t sector_points; // the number of points that are stored in each sector
                            // each time the universe has reached its limit, a new sector with this many points are created
    uint32_t num_sectors; // number of sectors currently allocated

    Sector* sectors_head; // pointer to a linked list of sectors of array of vectors
    Sector* sectors_tail; // pointer to a linked list of sectors of array of vectors

    uint16_t dimensions; // number of dimensions (individual sectors can have a different number of dimensions, because distance function is inner product)
    uint16_t flags; // any flags that need to be set on it

    RedisModuleDict* id2sector; // dict mapping point id to its sector

} Universe;

Universe* Universe_create (uint16_t dimensions = 128, uint32_t sector_points = 1024, uint16_t flags = 0, uint64_t id = 0) {
    Universe *u;
    u = (Universe*) RedisModule_Calloc(1,sizeof(*u));
    u->id2sector = RedisModule_CreateDict(NULL);
    // u->id = id || Universe_next_id++;
    u->flags = flags;
    u->dimensions = dimensions;

    u->sector_points = sector_points;
    u->num_sectors = 0;

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

void UniverseRdbSave (RedisModuleIO *rdb, void *value) {
    Universe* u = (Universe*) value;
    // RedisModule_SaveUnsigned(rdb, u->id);
    RedisModule_SaveUnsigned(rdb, u->dimensions);
    RedisModule_SaveUnsigned(rdb, u->flags);
    // num_points is calculatable
    // max_points is calculatable
    RedisModule_SaveUnsigned(rdb, u->sector_points);
    RedisModule_SaveUnsigned(rdb, u->num_sectors);
    Sector* s = (Sector*) u->sectors_head;
    while (s) {
        // @Efficiency: save count/max together
        RedisModule_SaveUnsigned(rdb, s->count);
        RedisModule_SaveUnsigned(rdb, s->max);
        // @Efficiency: save dimensions/flags together
        RedisModule_SaveUnsigned(rdb, s->dimensions);
        RedisModule_SaveUnsigned(rdb, s->flags);
        auto count = s->count;
        auto dimensions = s->dimensions;
        for (uint32_t j = 0; j < count; j++) {
            RedisModule_SaveString(rdb, s->ids[j]);
            for (uint32_t k = 0; k < dimensions; k++) {
#ifdef USE_DOUBLE
                RedisModule_SaveDouble(rdb, s->points[j*dimensions+k]);
#else
                RedisModule_SaveFloat(rdb, s->points[j*dimensions+k]);
#endif
            }
        }

        s = s->next;
    }

}

void* UniverseRdbLoad (RedisModuleIO *rdb, int encver) {
    if (encver != 1) {
        RedisModule_LogIOError(rdb,"warning","Can't load data with version %d", encver);
        return NULL;
    }

    uint64_t id = RedisModule_LoadUnsigned(rdb);
    uint64_t dimensions = RedisModule_LoadUnsigned(rdb);
    uint64_t flags = RedisModule_LoadUnsigned(rdb);
    uint64_t sector_points = RedisModule_LoadUnsigned(rdb);
    uint64_t num_sectors = RedisModule_LoadUnsigned(rdb);
    uint64_t num_points = 0;
    uint64_t max_points = 0;

    Universe* u = Universe_create(dimensions, sector_points, flags, id);

    Sector* prev = NULL;
    Sector* s = NULL;
    for (uint32_t i = 0; i < num_sectors; i++) {
        // for each sector, load them:
        int64_t count = RedisModule_LoadSigned(rdb);
        int64_t max = RedisModule_LoadSigned(rdb);
        int64_t dimensions = RedisModule_LoadUnsigned(rdb);
        int64_t flags = RedisModule_LoadUnsigned(rdb);
        s = Sector_create(dimensions, max);
        if (prev) prev->next = s;
        else u->sectors_head = s;
        s->flags = flags;
        s->count = count;

        for (uint32_t j = 0; j < count; j++) {
            size_t offset = j * dimensions;
            auto id = RedisModule_LoadString(rdb);
            s->ids[j] = id;
            RedisModule_DictSet(u->id2sector, id, (void*) s);
            RedisModule_DictSet(s->id2point, id, &s->points[offset]);
            for (uint32_t k = 0; k < dimensions; k++) {
#ifdef USE_DOUBLE
                s->points[offset+k] = RedisModule_LoadDouble(rdb);
#else
                s->points[offset+k] = RedisModule_LoadFloat(rdb);
#endif
            }
        }
    }

    u->sectors_tail = s;
    u->num_sectors = num_sectors;
    u->num_points = num_points;
    u->max_points = max_points;

    return u;
}

void UniverseAofRewrite (RedisModuleIO *aof, RedisModuleString *key, void *value) {
    REDISMODULE_NOT_USED(aof);
    REDISMODULE_NOT_USED(key);
    REDISMODULE_NOT_USED(value);
#if 0
    Universe* u = (Universe*) value;
    Sector* s = u->sectors_head;
    // dimensions
    RedisModule_EmitAOF(aof, "MC.UNIVERSE.CREATE", "sl", key, u->dimensions);
    while (s) {
        // RedisModule_EmitAOF(aof, "MC.POINT.CREATE", "sl", key, s->value);
        RedisModule_LogIOError(aof, "warning", "UHOH! trying to rewrite a universe with initialised sectors");
        s = s->next;
    }
#endif
}

// *(void* custom_malloc(size_t bytes))
// void* (custom_malloc)(size_t) = &malloc;
// #define malloc *(custom_malloc)

void ReleaseUniverseObject (Universe *u) {
    Sector *s = u->sectors_head;
    while (s) {
        RedisModule_FreeDict(NULL, s->id2point);
        RedisModule_Free(s->points);
        RedisModule_Free(s = s->next);
    }

    RedisModule_FreeDict(NULL, u->id2sector);
    RedisModule_Free(u);
}

void UniverseFree (void *value) {
    ReleaseUniverseObject((Universe*) value);
}

size_t UniverseMemUsage (const void *value) {
    Universe* u = (Universe*) value;
    Sector* s = u->sectors_head;
    size_t bytes = sizeof(*u);
    while (s) {
        bytes += sizeof(*s) + s->max * sizeof(number_t);
        s = s->next;
    }

    return bytes;
}

// MC.UNIVERSE.CREATE <universe_id> [dimensions=128]
int Cmd_Universe_Create (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);

    if (argc > 3 || argc < 2) {
        return RedisModule_WrongArity(ctx);
    }

    // get universe key
    RedisModuleKey *key = (RedisModuleKey*) RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ|REDISMODULE_WRITE);
    auto type = RedisModule_ModuleTypeGetType(key);

    if (type != REDISMODULE_KEYTYPE_EMPTY) {
        return RedisModule_ReplyWithError(ctx, type != Universe_Data ? REDISMODULE_ERRORMSG_WRONGTYPE
            : "universe is already created at this key");
    }

    int64_t sector_points = 1024;
    int64_t dimensions = 128;
    int64_t flags = 0;

    if (argc > 2) {
        if (RedisModule_StringToLongLong(argv[2], &dimensions) == REDISMODULE_ERR || dimensions < 0)
            return RedisModule_ReplyWithError(ctx, "ERR: dimensions should be a positive integer");
    }

    if (dimensions <= 0) dimensions = 128;
    if ((unsigned) dimensions > ULONG_MAX) dimensions = ULONG_MAX;
    if (sector_points <= 0) sector_points = 1024;
    if ((unsigned) sector_points > ULONG_MAX) sector_points = ULONG_MAX;

    RedisModule_Log(ctx,"debug","creating universe: %d %d", dimensions, sector_points);

    Universe *u = Universe_create(dimensions, sector_points, flags);
    RedisModule_ModuleTypeSetValue(key, Universe_Data, u);

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    RedisModule_ReplicateVerbatim(ctx);
    return REDISMODULE_OK;
}

//

// MC.NEAR <universe_id> <radius> <results> [positions ...]
// TODO

// MC.UNIVERSE.QUERY <universe_id> <positions[dimensions]> [radius=1] [results=10]
int Cmd_Universe_Query (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }

    // get universe key
    RedisModuleKey *key = (RedisModuleKey*) RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);
    if (RedisModule_ModuleTypeGetType(key) != Universe_Data)
        return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);

    Universe *u = (Universe*) RedisModule_ModuleTypeGetValue(key);

    auto dims = u->dimensions;
    if (argc < dims + 2) {
        auto err = RedisModule_CreateStringPrintf(ctx, "ERR arity must be %d: (given: %d)", dims + 2, argc);
        return RedisModule_ReplyWithError(ctx, RedisModule_StringPtrLen(err, NULL));
    }

    // double* query_ = (double*) RedisModule_Alloc(sizeof(double) * dims);
    // d_Point dquery;
    VectorXd dquery(dims);
    double* query_ptr = dquery.data();

    for (int i = 0; i < dims; i++) {
        RedisModule_StringToDouble(argv[i + 2], &query_ptr[i]);
    }

    Point query(dquery.cast<number_t>());

    // for ()


    return REDISMODULE_ERR; // @Incomplete
}

// MC.POINT.CREATE <universe_id> <point_id> [pos: float(dimensions)]
int Cmd_Point_Create (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }

    // get universe key
    RedisModuleKey *u_key = (RedisModuleKey*) RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);
    if (RedisModule_ModuleTypeGetType(u_key) != Universe_Data)
        return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);

    Universe *u = (Universe*) RedisModule_ModuleTypeGetValue(u_key);

    auto dims = u->dimensions;
    if (argc > dims + 2) {
        auto err = RedisModule_CreateStringPrintf(ctx, "ERR arity must be <= %d: (given: %d)", dims + 2, argc);
        return RedisModule_ReplyWithError(ctx, RedisModule_StringPtrLen(err, NULL));
    }

    Sector* s = Universe_ensure_space(u, 1);
    // number_t* point = RedisModule_Malloc(sizeof(number_t) * dims);
    dims = s->dimensions; // just in case the sector is different dimensionality than the universe

    number_t* point = &s->points[s->count * dims];

    for (int i = 0; i < dims; i++) {
        int arg = i + 2;
        if (arg < argc) {
#ifdef USE_DOUBLE
            RedisModule_StringToDouble(argv[arg], &point[i]);
#else
            double value;
            RedisModule_StringToDouble(argv[arg], &value);
            point[i] = (number_t) value;
#endif
        } else {
            point[i] = udist(rande);
        }
    }

    // Point point(dpoint.cast<number_t>());
    // memcpy(&s->points[++s->count * dims], point.data(), sizeof(number_t) * dims);

    RedisModuleString* id = argv[1];

    s->ids[s->count] = id;
    RedisModule_RetainString(ctx, id);

    s->count++;

    // RedisModuleKey *key = RedisModule_OpenKey(ctx,keyname,REDISMODULE_WRITE);
    // struct some_private_struct *data = createMyDataStructure();
    // RedisModule_ModuleTypeSetValue(key,MyType,data);

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    RedisModule_ReplicateVerbatim(ctx);

    return REDISMODULE_OK;
}

// MC.POINT.POS <universe_id> <point_id>
int Cmd_Point_Pos (RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }

    // get universe key
    RedisModuleKey *key = (RedisModuleKey*) RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);
    if (RedisModule_ModuleTypeGetType(key) != Universe_Data)
        return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);

    Universe *u = (Universe*) RedisModule_ModuleTypeGetValue(key);

    RedisModuleString* id = argv[2];

    Sector* s = (Sector*) RedisModule_DictGet(u->id2sector, id, NULL);
    if (!s) return RedisModule_ReplyWithNull(ctx);

    number_t* point = (number_t*) RedisModule_DictGet(s->id2point, id, NULL);
    if (!point) return RedisModule_ReplyWithNull(ctx);

    uint32_t dims = s->dimensions;
    RedisModule_ReplyWithArray(ctx, dims);
    for (uint32_t i = 0; i < dims; i++) {
        RedisModule_ReplyWithDouble(ctx, (double) point[i]);
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
        .rdb_load = UniverseRdbLoad,
        .rdb_save = UniverseRdbSave,
        .aof_rewrite = UniverseAofRewrite,
        .free = UniverseFree
    };

    Universe_Data = RedisModule_CreateDataType(ctx, "mc-uverse", RDB_VERSION, &tm);
    if (Universe_Data == NULL) return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.universe.create",
        Cmd_Universe_Create,"write deny-oom",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.universe.query",
        Cmd_Universe_Query,"readonly",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.point.create",
        Cmd_Point_Create,"readonly",1,1,1) == REDISMODULE_ERR)
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
