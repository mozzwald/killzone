#include <string.h>
#include <peekpoke.h>

#include "atari_visuals.h"

#define ROM_CHARSET_ADDR 0xE000
#define CHARSET_SIZE 1024
#define CHARSET_ALIGN_MASK 0xFC00

#define CHBAS 756
#define COLOR0 708
#define COLOR1 709
#define COLOR2 710
#define COLOR3 711
#define COLOR4 712

#define COLOR_BLACK 0x00
#define COLOR_LIGHT_GREEN 0xCA

#define SCREEN_CODE_HASH 3
#define SCREEN_CODE_STAR 10
#define SCREEN_CODE_PLUS 11
#define SCREEN_CODE_DOT 14
#define SCREEN_CODE_AT 32

static unsigned char charset_storage[CHARSET_SIZE + 1024];
static unsigned char *game_charset;
static unsigned char old_chbas;
static unsigned char old_colors[5];
static unsigned char initialized;

static void patch_glyph(unsigned char screen_code, const unsigned char *glyph)
{
    memcpy(game_charset + ((unsigned int)screen_code * 8), glyph, 8);
}

void atari_visuals_init(void)
{
    unsigned int addr;

    static const unsigned char grass_glyph[8] = {
        0x00, 0x02, 0x00, 0x20, 0x00, 0x04, 0x00, 0x40
    };
    /* Local player: person with head, shoulders, body, arms, and legs. */
    static const unsigned char local_player_glyph[8] = {
        0x18, 0x3C, 0x3C, 0x7E, 0x99, 0x18, 0x24, 0x66
    };
    /* Remote player: slimmer person silhouette to read differently at a glance. */
    static const unsigned char remote_player_glyph[8] = {
        0x18, 0x3C, 0x18, 0x7E, 0x5A, 0x18, 0x24, 0x42
    };
    /* Goblin: squat monster with ears, eyes, claws, and short legs. */
    static const unsigned char goblin_glyph[8] = {
        0x42, 0xA5, 0x7E, 0xDB, 0xFF, 0x7E, 0x24, 0x42
    };
    /* Hunter: horned monster, wider eyes, raised claws, heavier body. */
    static const unsigned char hunter_glyph[8] = {
        0xA5, 0xDB, 0x7E, 0xFF, 0xDB, 0x7E, 0x66, 0xC3
    };

    if (initialized) {
        return;
    }

    old_chbas = PEEK(CHBAS);
    old_colors[0] = PEEK(COLOR0);
    old_colors[1] = PEEK(COLOR1);
    old_colors[2] = PEEK(COLOR2);
    old_colors[3] = PEEK(COLOR3);
    old_colors[4] = PEEK(COLOR4);

    addr = (unsigned int)charset_storage;
    addr = (addr + 1023) & CHARSET_ALIGN_MASK;
    game_charset = (unsigned char *)addr;

    memcpy(game_charset, (const void *)ROM_CHARSET_ADDR, CHARSET_SIZE);
    patch_glyph(SCREEN_CODE_DOT, grass_glyph);
    patch_glyph(SCREEN_CODE_AT, local_player_glyph);
    patch_glyph(SCREEN_CODE_HASH, remote_player_glyph);
    patch_glyph(SCREEN_CODE_STAR, goblin_glyph);
    patch_glyph(SCREEN_CODE_PLUS, hunter_glyph);

    POKE(CHBAS, (unsigned char)(addr >> 8));
    POKE(COLOR0, COLOR_LIGHT_GREEN);
    POKE(COLOR1, COLOR_LIGHT_GREEN);
    POKE(COLOR2, COLOR_BLACK);
    POKE(COLOR3, COLOR_LIGHT_GREEN);
    POKE(COLOR4, COLOR_BLACK);

    initialized = 1;
}

void atari_visuals_shutdown(void)
{
    if (!initialized) {
        return;
    }

    POKE(CHBAS, old_chbas);
    POKE(COLOR0, old_colors[0]);
    POKE(COLOR1, old_colors[1]);
    POKE(COLOR2, old_colors[2]);
    POKE(COLOR3, old_colors[3]);
    POKE(COLOR4, old_colors[4]);

    initialized = 0;
}
