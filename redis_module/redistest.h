#ifndef __REDISTEST_H__
#define __REDISTEST_H__

#include <string.h>

#define TEST(name,...) \
    do { \
        RedisModule_Log(ctx,"warning","Testing %s", name); \
        reply = RedisModule_Call(ctx,name,__VA_ARGS__); \
    } while (0);

// Return true if the reply and the C null term string matches.
static int TestMatchReply(RedisModuleCallReply *reply, const char *str) {
    RedisModuleString *mystr;
    mystr = RedisModule_CreateStringFromCallReply(reply);
    if (!mystr) return 0;
    const char *ptr = RedisModule_StringPtrLen(mystr,NULL);
    return strcmp(ptr,str) == 0;
}

// ----------------------------- Test framework -----------------------------

// Return 1 if the reply matches the specified string, otherwise log errors
// in the server log and return 0.
static int TestAssertStringReply(RedisModuleCtx *ctx, RedisModuleCallReply *reply, const char *str, size_t len) {
    RedisModuleString *mystr, *expected;

    if (RedisModule_CallReplyType(reply) != REDISMODULE_REPLY_STRING) {
        RedisModule_Log(ctx,"warning","Unexpected reply type %d",
            RedisModule_CallReplyType(reply));
        return 0;
    }
    mystr = RedisModule_CreateStringFromCallReply(reply);
    expected = RedisModule_CreateString(ctx,str,len);
    if (RedisModule_StringCompare(mystr,expected) != 0) {
        const char *mystr_ptr = RedisModule_StringPtrLen(mystr,NULL);
        const char *expected_ptr = RedisModule_StringPtrLen(expected,NULL);
        RedisModule_Log(ctx,"warning",
            "Unexpected string reply '%s' (instead of '%s')",
            mystr_ptr, expected_ptr);
        return 0;
    }
    return 1;
}

// Return 1 if the reply matches the specified integer, otherwise log errors
// in the server log and return 0.
static int TestAssertIntegerReply(RedisModuleCtx *ctx, RedisModuleCallReply *reply, long long expected) {
    if (RedisModule_CallReplyType(reply) != REDISMODULE_REPLY_INTEGER) {
        RedisModule_Log(ctx,"warning","Unexpected reply type %d",
            RedisModule_CallReplyType(reply));
        return 0;
    }
    long long val = RedisModule_CallReplyInteger(reply);
    if (val != expected) {
        RedisModule_Log(ctx,"warning",
            "Unexpected integer reply '%lld' (instead of '%lld')",
            val, expected);
        return 0;
    }
    return 1;
}

// define this function
typedef struct RedisModuleCtx RedisModuleCtx;
// int ADD_TEST_COMMANDS (RedisModuleCtx *ctx) __attribute__((unused));
int ADD_TEST_COMMANDS (RedisModuleCtx *ctx);

#endif // __REDISTEST_H__
