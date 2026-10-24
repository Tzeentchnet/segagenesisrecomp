#include "resume_dispatch.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        exit(1); \
    } \
} while (0)

static void check_next(
    GenesisResumeDispatch *dispatch, uint32_t expected_pc,
    GenesisResumeDispatchStage expected_stage)
{
    uint32_t pc = 0;
    GenesisResumeDispatchStage stage = GENESIS_RESUME_DISPATCH_DONE;
    CHECK(genesis_resume_dispatch_next(dispatch, &pc, &stage));
    CHECK(pc == expected_pc);
    CHECK(stage == expected_stage);
}

static void check_done(GenesisResumeDispatch *dispatch)
{
    uint32_t pc = 0;
    GenesisResumeDispatchStage stage = GENESIS_RESUME_DISPATCH_DONE;
    CHECK(!genesis_resume_dispatch_next(dispatch, &pc, &stage));
}

int main(void)
{
    GenesisResumeDispatch dispatch;
    GameSpec spec = {
        .dispatch_main_loop_pc = 0x123456u,
    };

    genesis_resume_dispatch_init(&dispatch, &spec);
    check_next(&dispatch, 0x123456u, GENESIS_RESUME_DISPATCH_INITIAL);
    check_done(&dispatch);

    spec.resume_dispatch_setup_pc = 0xABCDEFu;
    genesis_resume_dispatch_init(&dispatch, &spec);
    check_next(&dispatch, 0xABCDEFu, GENESIS_RESUME_DISPATCH_SETUP);
    check_next(&dispatch, 0x123456u, GENESIS_RESUME_DISPATCH_INITIAL);
    check_done(&dispatch);

    spec.redispatch_main_loop_pc = 0x654321u;
    genesis_resume_dispatch_init(&dispatch, &spec);
    check_next(&dispatch, 0xABCDEFu, GENESIS_RESUME_DISPATCH_SETUP);
    check_next(&dispatch, 0x123456u, GENESIS_RESUME_DISPATCH_INITIAL);
    check_next(&dispatch, 0x654321u, GENESIS_RESUME_DISPATCH_STEADY);
    check_next(&dispatch, 0x654321u, GENESIS_RESUME_DISPATCH_STEADY);

    spec.resume_dispatch_setup_pc = 0;
    genesis_resume_dispatch_init(&dispatch, &spec);
    check_next(&dispatch, 0x123456u, GENESIS_RESUME_DISPATCH_INITIAL);
    check_next(&dispatch, 0x654321u, GENESIS_RESUME_DISPATCH_STEADY);

    spec.resume_dispatch_setup_pc = 0x1ABCDEFu;
    spec.dispatch_main_loop_pc = 0x1123456u;
    spec.redispatch_main_loop_pc = 0x1654321u;
    genesis_resume_dispatch_init(&dispatch, &spec);
    check_next(&dispatch, 0xABCDEFu, GENESIS_RESUME_DISPATCH_SETUP);
    check_next(&dispatch, 0x123456u, GENESIS_RESUME_DISPATCH_INITIAL);
    check_next(&dispatch, 0x654321u, GENESIS_RESUME_DISPATCH_STEADY);

    spec.dispatch_main_loop_pc = 0;
    genesis_resume_dispatch_init(&dispatch, &spec);
    check_done(&dispatch);
    return 0;
}
