#ifndef __REDISHACKERY_H__
#define __REDISHACKERY_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* redis commands
 * (from: https://redis.io/commands/command)
 *
 * flags:
 * write - command may result in modifications
 * readonly - command will never modify keys
 * denyoom - reject command if currently OOM
 * admin - server admin command
 * pubsub - pubsub-related command
 * noscript - deny this command from scripts
 * random - command has random results, dangerous for scripts
 * sort_for_script - if called from script, sort output
 * loading - allow command while database is loading
 * stale - allow command while replica has stale data
 * skip_monitor - do not show this command in MONITOR
 * asking - cluster related - accept even if importing
 * fast - command operates in constant or log(N) time. Used for latency monitoring.
 * movablekeys - keys have no pre-determined position. You must discover keys yourself.
 *
 * arity:
 * positive if command has fixed number of required arguments.
 * negative if command has minimum number of required arguments, but may have more.
 *
 * key position: (necessary for redis cluster)
 * first key: 1 (usually)
 * last key: 1 (usually, but can be negative for unlimited number of keys)
 * key skip: 2 (the interval of arguments to find another key between the first and last key)
 *
 */

#include "redismodule.h"

// copy-n-paste from server.h
typedef void *(*moduleTypeLoadFunc)(struct RedisModuleIO *io, int encver);
typedef void (*moduleTypeSaveFunc)(struct RedisModuleIO *io, void *value);
typedef void (*moduleTypeRewriteFunc)(struct RedisModuleIO *io, struct redisObject *key, void *value);
typedef void (*moduleTypeDigestFunc)(struct RedisModuleDigest *digest, void *value);
typedef size_t (*moduleTypeMemUsageFunc)(const void *value);
typedef void (*moduleTypeFreeFunc)(void *value);

typedef struct RedisModuleType {
    uint64_t id; /* Higher 54 bits of type ID + 10 lower bits of encoding ver. */
    struct RedisModule *module;
    moduleTypeLoadFunc rdb_load;
    moduleTypeSaveFunc rdb_save;
    moduleTypeRewriteFunc aof_rewrite;
    moduleTypeMemUsageFunc mem_usage;
    moduleTypeDigestFunc digest;
    moduleTypeFreeFunc free;
    char name[10]; /* 9 bytes name + null term. Charset: A-Z a-z 0-9 _- */
} moduleType;

typedef struct moduleValue {
    moduleType *type;
    void *value;
} moduleValue;

#define LRU_BITS 24
typedef struct RedisModuleString {
    unsigned type:4;
    unsigned encoding:4;
    unsigned lru:LRU_BITS; /* LRU time (relative to global lru_clock) or
                            * LFU data (least significant 8 bits frequency
                            * and most significant 16 bits access time). */
    int refcount;
    void *ptr;
} RedisModuleString;

int ReplyWithError (RedisModuleCtx *ctx, const char* format, ...);

int ReplyWithError (RedisModuleCtx *ctx, const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start (args, format);
  vsnprintf (buffer, 255, format, args);

  RedisModule_ReplyWithError(ctx, &buffer[0]);

  va_end (args);
  return REDISMODULE_OK;
}

#define OBJ_PROP(o, p) o->##p

#define BEGIN_ARRAY_REPLY() \
    size_t arrc = 0; \
    RedisModule_ReplyWithArray(ctx, REDISMODULE_POSTPONED_ARRAY_LEN);

#define END_ARRAY_REPLY() RedisModule_ReplySetArrayLength(ctx, arrc)

#define ARRAY_REPLY_STR_INT(k, v) do { \
    RedisModule_ReplyWithSimpleString(ctx, k); \
    RedisModule_ReplyWithLongLong(ctx, v); \
    arrc += 2; \
} while (0)

#define ARRAY_REPLY_STR_DOUBLE(k, v) do { \
    RedisModule_ReplyWithSimpleString(ctx, k); \
    RedisModule_ReplyWithDouble(ctx, v); \
    arrc += 2; \
} while (0)

#define ARRAY_REPLY_OBJ_INT(o, key) do { \
    RedisModule_ReplyWithSimpleString(ctx, #key); \
    RedisModule_ReplyWithLongLong(ctx, o->key); \
    arrc += 2; \
} while (0)


// RedisModule_ReplyWithLongLong(ctx, OBJ_PROP(o, key));


#endif // __REDIS_HACKERY__
