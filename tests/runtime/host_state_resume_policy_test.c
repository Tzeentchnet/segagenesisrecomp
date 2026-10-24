#include "game_layout.h"
#include "game_spec.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t Uint32;
typedef int32_t Sint32;

#define RECOMP_LAUNCHER 0
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        exit(1); \
    } \
} while (0)

uint8_t g_ram[65536];
uint8_t g_rom[0x400000];

static uint32_t s_cpu_state;
static uint32_t s_machine_state;
static uint32_t s_ym_state;
static uint32_t s_psg_state;
static uint8_t s_adapter_state;
static int s_adapter_preflight_calls;
static int s_adapter_apply_calls;
static int s_audio_discards;
static int s_restart_calls;

static uint64_t SDL_GetPerformanceCounter(void)
{
    static uint64_t counter;
    return ++counter;
}

static uint32_t rom_crc32(const uint8_t *data, size_t len)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++)
        hash = (hash ^ data[i]) * 16777619u;
    return hash;
}

void glue_save_state(FILE *f)
{
    CHECK(fwrite(&s_cpu_state, sizeof(s_cpu_state), 1, f) == 1);
}

void glue_load_state(FILE *f)
{
    CHECK(fread(&s_cpu_state, sizeof(s_cpu_state), 1, f) == 1);
}

int machine_save_state(FILE *f)
{
    return fwrite(&s_machine_state, sizeof(s_machine_state), 1, f) == 1;
}

int machine_load_state(FILE *f)
{
    return fread(&s_machine_state, sizeof(s_machine_state), 1, f) == 1;
}

int ym2612_save_state(FILE *f)
{
    const uint32_t size = sizeof(s_ym_state);
    return fwrite(&size, sizeof(size), 1, f) == 1
        && fwrite(&s_ym_state, sizeof(s_ym_state), 1, f) == 1;
}

int ym2612_load_state(FILE *f)
{
    uint32_t size = 0;
    return fread(&size, sizeof(size), 1, f) == 1
        && size == sizeof(s_ym_state)
        && fread(&s_ym_state, sizeof(s_ym_state), 1, f) == 1;
}

int psg_save_state(FILE *f)
{
    return fwrite(&s_psg_state, sizeof(s_psg_state), 1, f) == 1;
}

int psg_load_state(FILE *f)
{
    return fread(&s_psg_state, sizeof(s_psg_state), 1, f) == 1;
}

void audio_discard_playback(void)
{
    s_audio_discards++;
}

void glue_restart_game_fiber(uint32_t resume_pc)
{
    (void)resume_pc;
    s_restart_calls++;
}

void glue_state_boundary_request(int requested)
{
    (void)requested;
}

int glue_state_boundary_ready(void)
{
    return 1;
}

uint8_t glue_peek8(uint32_t address)
{
    return g_ram[(uint16_t)address];
}

static size_t adapter_state_size(void)
{
    return sizeof(s_adapter_state);
}

static int adapter_state_save(void *data, size_t size)
{
    if (size != sizeof(s_adapter_state))
        return 0;
    memcpy(data, &s_adapter_state, size);
    return 1;
}

static int adapter_state_load(const void *data, size_t size, int apply)
{
    if (size != sizeof(s_adapter_state))
        return 0;
    if (!apply) {
        s_adapter_preflight_calls++;
        return 1;
    }
    s_adapter_apply_calls++;
    memcpy(&s_adapter_state, data, size);
    return 1;
}

static uint32_t resolve_mode(uint8_t mode)
{
    return mode == 0xFFu ? GAME_RESUME_PC_UNSUPPORTED : 0x123456u;
}

const GameRamLayout g_game_layout = {
    .game_mode_addr = 0xFF0010u,
};

const GameSpec g_game_spec = {
    .display_name = "Host state test",
    .expected_rom_size = 1u,
    .state_build_id = "host-state-test",
    .state_size = adapter_state_size,
    .state_save = adapter_state_save,
    .state_load = adapter_state_load,
    .save_resume_pc = resolve_mode,
};

#include "host_state.h"

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    const char *path = argv[1];
    static uint8_t expected_ram[sizeof(g_ram)];

    memset(g_ram, 0x11, sizeof(g_ram));
    g_ram[0x0010] = 0xFFu;
    s_cpu_state = 0x11111111u;
    s_machine_state = 0x22222222u;
    s_ym_state = 0x33333333u;
    s_psg_state = 0x44444444u;
    s_adapter_state = 0x55u;
    CHECK(host_state_save_now(path));

    memset(g_ram, 0x63, sizeof(g_ram));
    g_ram[0x0010] = 1u;
    memcpy(expected_ram, g_ram, sizeof(g_ram));
    s_cpu_state = 0xA1A1A1A1u;
    s_machine_state = 0xB2B2B2B2u;
    s_ym_state = 0xC3C3C3C3u;
    s_psg_state = 0xD4D4D4D4u;
    s_adapter_state = 0xE5u;

    CHECK(!host_state_load(path));
    CHECK(memcmp(g_ram, expected_ram, sizeof(g_ram)) == 0);
    CHECK(s_cpu_state == 0xA1A1A1A1u);
    CHECK(s_machine_state == 0xB2B2B2B2u);
    CHECK(s_ym_state == 0xC3C3C3C3u);
    CHECK(s_psg_state == 0xD4D4D4D4u);
    CHECK(s_adapter_state == 0xE5u);
    CHECK(s_adapter_preflight_calls == 1);
    CHECK(s_adapter_apply_calls == 0);
    CHECK(s_audio_discards == 0);
    CHECK(s_restart_calls == 0);
    CHECK(remove(path) == 0);
    return 0;
}
