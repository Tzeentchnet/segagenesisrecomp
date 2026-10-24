#pragma once

#include "game_spec.h"

typedef enum GenesisResumeDispatchStage {
    GENESIS_RESUME_DISPATCH_SETUP,
    GENESIS_RESUME_DISPATCH_INITIAL,
    GENESIS_RESUME_DISPATCH_STEADY,
    GENESIS_RESUME_DISPATCH_DONE,
} GenesisResumeDispatchStage;

typedef struct GenesisResumeDispatch {
    uint32_t setup_pc;
    uint32_t dispatch_pc;
    uint32_t redispatch_pc;
    GenesisResumeDispatchStage next_stage;
} GenesisResumeDispatch;

static inline void genesis_resume_dispatch_init(
    GenesisResumeDispatch *dispatch, const GameSpec *spec)
{
    dispatch->dispatch_pc = spec->dispatch_main_loop_pc & 0xFFFFFFu;
    if (!dispatch->dispatch_pc) {
        dispatch->setup_pc = 0;
        dispatch->redispatch_pc = 0;
        dispatch->next_stage = GENESIS_RESUME_DISPATCH_DONE;
        return;
    }
    dispatch->setup_pc = spec->resume_dispatch_setup_pc & 0xFFFFFFu;
    dispatch->redispatch_pc = spec->redispatch_main_loop_pc & 0xFFFFFFu;
    dispatch->next_stage = dispatch->setup_pc
        ? GENESIS_RESUME_DISPATCH_SETUP
        : GENESIS_RESUME_DISPATCH_INITIAL;
}

static inline bool genesis_resume_dispatch_next(
    GenesisResumeDispatch *dispatch, uint32_t *pc,
    GenesisResumeDispatchStage *stage)
{
    switch (dispatch->next_stage) {
    case GENESIS_RESUME_DISPATCH_SETUP:
        *pc = dispatch->setup_pc;
        *stage = dispatch->next_stage;
        dispatch->next_stage = GENESIS_RESUME_DISPATCH_INITIAL;
        return true;
    case GENESIS_RESUME_DISPATCH_INITIAL:
        *pc = dispatch->dispatch_pc;
        *stage = dispatch->next_stage;
        dispatch->next_stage = dispatch->redispatch_pc
            ? GENESIS_RESUME_DISPATCH_STEADY
            : GENESIS_RESUME_DISPATCH_DONE;
        return true;
    case GENESIS_RESUME_DISPATCH_STEADY:
        *pc = dispatch->redispatch_pc;
        *stage = dispatch->next_stage;
        return true;
    case GENESIS_RESUME_DISPATCH_DONE:
        return false;
    }
    return false;
}
