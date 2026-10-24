#include "game_spec.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        exit(1); \
    } \
} while (0)

static uint32_t resolve_mode(uint8_t mode)
{
    if (mode == 1u)
        return 0x123456u;
    if (mode == 0xFFu)
        return GAME_RESUME_PC_UNSUPPORTED;
    return 0;
}

int main(void)
{
    const GameSpec spec = {
        .resume_main_loop_pc = 0x654320u,
        .save_resume_pc = resolve_mode,
    };
    uint32_t pc = 0;

    CHECK(game_spec_resolve_save_resume_pc(&spec, true, 1u, &pc));
    CHECK(pc == 0x123456u);
    CHECK(game_spec_resolve_save_resume_pc(&spec, true, 2u, &pc));
    CHECK(pc == 0x654320u);
    CHECK(game_spec_resolve_save_resume_pc(&spec, false, 1u, &pc));
    CHECK(pc == 0x654320u);
    CHECK(!game_spec_resolve_save_resume_pc(&spec, true, 0xFFu, &pc));
    CHECK(game_spec_resolve_save_resume_pc(&spec, true, 1u, NULL));
    return 0;
}
