#include "redismodule.h"
#include "redistest.h"

// ------------------------------- Test units -------------------------------

// TEST.CALL -- Test Call() API.
int TestCall(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    RedisModule_AutoMemory(ctx);
    RedisModuleCallReply *reply;

    RedisModule_Call(ctx,"DEL","c","mylist");
    RedisModuleString *mystr = RedisModule_CreateString(ctx,"foo",3);
    RedisModule_Call(ctx,"RPUSH","csl","mylist",mystr,(long long)1234);
    reply = RedisModule_Call(ctx,"LRANGE","ccc","mylist","0","-1");
    long long items = RedisModule_CallReplyLength(reply);
    if (items != 2) goto fail;

    RedisModuleCallReply *item0, *item1;

    item0 = RedisModule_CallReplyArrayElement(reply,0);
    item1 = RedisModule_CallReplyArrayElement(reply,1);
    if (!TestMatchReply(item0,"foo")) goto fail;
    if (!TestMatchReply(item1,"1234")) goto fail;

    RedisModule_ReplyWithSimpleString(ctx,"OK");
    return REDISMODULE_OK;

fail:
    RedisModule_ReplyWithSimpleString(ctx,"ERR");
    return REDISMODULE_OK;
}

// TEST.STRING.APPEND -- Test appending to an existing string object.
int TestStringAppend(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    RedisModuleString *s = RedisModule_CreateString(ctx,"foo",3);
    RedisModule_StringAppendBuffer(ctx,s,"bar",3);
    RedisModule_ReplyWithString(ctx,s);
    RedisModule_FreeString(ctx,s);
    return REDISMODULE_OK;
}

// TEST.STRING.APPEND.AM -- Test append with retain when auto memory is on.
int TestStringAppendAM(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    RedisModule_AutoMemory(ctx);
    RedisModuleString *s = RedisModule_CreateString(ctx,"foo",3);
    RedisModule_RetainString(ctx,s);
    RedisModule_StringAppendBuffer(ctx,s,"bar",3);
    RedisModule_ReplyWithString(ctx,s);
    RedisModule_FreeString(ctx,s);
    return REDISMODULE_OK;
}

// TEST.STRING.PRINTF -- Test string formatting.
int TestStringPrintf(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModuleString *s = RedisModule_CreateStringPrintf(ctx,
        "Got %d args. argv[1]: %s, argv[2]: %s",
        argc,
        RedisModule_StringPtrLen(argv[1], NULL),
        RedisModule_StringPtrLen(argv[2], NULL)
    );

    RedisModule_ReplyWithString(ctx,s);

    return REDISMODULE_OK;
}

// TEST.IT -- Run all the tests.
int TestIt(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    RedisModule_AutoMemory(ctx);
    RedisModuleCallReply *reply;

    TEST("ping","");
    if (!TestAssertStringReply(ctx,reply,"PONG",4)) goto fail;

    TEST("test.call","");
    if (!TestAssertStringReply(ctx,reply,"OK",2)) goto fail;

    TEST("test.string.append","");
    if (!TestAssertStringReply(ctx,reply,"foobar",6)) goto fail;

    TEST("test.string.append.am","");
    if (!TestAssertStringReply(ctx,reply,"foobar",6)) goto fail;

    TEST("test.string.printf", "cc", "foo", "bar");
    if (!TestAssertStringReply(ctx,reply,"Got 3 args. argv[1]: foo, argv[2]: bar",38)) goto fail;

    RedisModule_ReplyWithSimpleString(ctx,"ALL TESTS PASSED");
    return REDISMODULE_OK;

fail:
    RedisModule_ReplyWithError(ctx,
        "SOME TEST NOT PASSED! Check server logs");
    return REDISMODULE_ERR;
}

int ADD_TEST_COMMANDS (RedisModuleCtx *ctx) {
    RedisModule_Log(ctx,"warning","test commands enabled");

    if (RedisModule_CreateCommand(ctx, "test.call",
        TestCall,"readonly",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "test.string.append",
        TestStringAppend,"readonly",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "test.string.append.am",
        TestStringAppendAM,"readonly",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "test.string.printf",
        TestStringPrintf,"readonly",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "test.it",
        TestIt,"readonly",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    return REDISMODULE_OK;
}
