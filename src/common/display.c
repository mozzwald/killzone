/**
 * KillZone Display Module Implementation
 * 
 * Text-based display rendering.
 * Uses text mode with last 4 lines for status bar.
 */

#include "display.h"
#include "network.h"
#ifdef __ATARI__
#include "atari_visuals.h"
#endif
#ifdef _CMOC_VERSION_
#include <cmoc.h>
#include <coco.h>
#include "conio_wrapper.h"
#include "snprintf.h"
#else
#include <stdio.h>
#include <string.h>
#include <conio.h>
#endif

/* Direct drawing to screen, no buffer needed */

static uint8_t status_needs_redraw = 1;

static void display_clear_line(uint8_t y, uint8_t width) {
    uint8_t x;

    for (x = 0; x < width; x++) {
        cputcxy(x, y, ' ');
    }
}

static void display_puts_limited(uint8_t x, uint8_t y, const char *text, uint8_t max_len) {
    uint8_t i;

    if (!text) {
        return;
    }

    for (i = 0; i < max_len && text[i] != '\0'; i++) {
        cputcxy((uint8_t)(x + i), y, text[i]);
    }
}

/**
 * Initialize display system
 */
void display_init(void) {
#ifdef _CMOC_VERSION_
    hirestxt_init();    
#endif
    clrscr();  /* Clear screen using conio */
#ifdef __ATARI__
    atari_visuals_init();
#endif
}

/**
 * Close display system
 */
void display_close(void) {
#ifdef __ATARI__
    atari_visuals_shutdown();
#endif
    clrscr();
#ifdef _CMOC_VERSION_
    hirestxt_close();
#endif
}

/**
 * Show welcome screen
 */
void display_show_welcome(const char *server_name) {
    static char url_buf[60];
    (void)server_name; /* Suppress unused warning */
    
    clrscr();
    gotoxy(0, 4);
    printf("  *** KILLZONE ***\n");
    gotoxy(0, 5);
    printf("  Version %s\n", CLIENT_VERSION);
    gotoxy(0, 6);
    printf("  @2025 DillerNet Studios\n");
    gotoxy(0, 8);
    printf("  Connecting to server:\n");
    gotoxy(0, 9);
    
    /* Build full URL */
    snprintf(url_buf, sizeof(url_buf), "N:TCP://%s:%d", SERVER_HOST, SERVER_TCP_PORT);
    
    printf("%s\n", url_buf);
    
    gotoxy(0, 11);
    printf("  Waiting for game world...\n");
}


/**
 * Draw status bar (last 4 lines of screen)
 * 
 * Shows: player name, player count, connection status, world ticks
 * Uses cputsxy for direct character placement without scrolling
 */
void display_draw_status_bar(const char *player_name, uint8_t player_count, 
                             const char *connection_status, uint16_t world_ticks) {
    static char line_buf[41];
    static char ticks_buf[11];
    static char ver_buf[16];
    static char last_info_buf[30] = "";
    static char last_ticks_buf[11] = "";
    static char last_ver_buf[16] = "";
    static uint8_t static_status_drawn = 0;
    const char *server_ver;
    uint8_t ver_len;
    uint8_t x;
    
    if (!player_name || !connection_status) {
        return;
    }
    
    /* Line 20: Player info on left, ticks on right. */
    snprintf(line_buf, sizeof(line_buf), "%s P:%d %s", player_name, player_count, connection_status);
    if (status_needs_redraw || strcmp(line_buf, last_info_buf) != 0) {
        for (x = 0; x < 30; x++) {
            cputcxy(x, 20, ' ');
        }
        display_puts_limited(0, 20, line_buf, 29);
        strncpy(last_info_buf, line_buf, sizeof(last_info_buf) - 1);
        last_info_buf[sizeof(last_info_buf) - 1] = '\0';
    }
    
    snprintf(ticks_buf, sizeof(ticks_buf), "T:%d", world_ticks);
    if (status_needs_redraw || strcmp(ticks_buf, last_ticks_buf) != 0) {
        for (x = 30; x < 39; x++) {
            cputcxy(x, 20, ' ');
        }
        display_puts_limited(30, 20, ticks_buf, 9);
        strncpy(last_ticks_buf, ticks_buf, sizeof(last_ticks_buf) - 1);
        last_ticks_buf[sizeof(last_ticks_buf) - 1] = '\0';
    }
    
    if (status_needs_redraw || !static_status_drawn) {
        /* Line 21: reserved for combat messages. */
        for (x = 0; x < 39; x++) {
            cputcxy(x, 21, ' ');
        }

        /* Line 22: Separator */
        display_puts_limited(0, 22, "---------------------------------------", 39);
        
        /*
         * Keep row 23 column 39 untouched. Writing the bottom-right cell can
         * advance the Atari text cursor and scroll the display.
         */
        for (x = 0; x < 39; x++) {
            cputcxy(x, 23, ' ');
        }
        display_puts_limited(0, 23, "WASD=Move R=Refresh Q=Quit", 27);
        static_status_drawn = 1;
    }
    
    /* Version display at far right: C1.1.0|S1.1.0 */
    server_ver = state_get_server_version();
    snprintf(ver_buf, sizeof(ver_buf), "C%s|S%s", CLIENT_VERSION, server_ver);
    if (status_needs_redraw || strcmp(ver_buf, last_ver_buf) != 0) {
        ver_len = (uint8_t)strlen(ver_buf);
        if (ver_len > 38) {
            ver_len = 38;
        }
        for (x = 27; x < 39; x++) {
            cputcxy(x, 23, ' ');
        }
        display_puts_limited((uint8_t)(39 - ver_len), 23, ver_buf, ver_len);
        strncpy(last_ver_buf, ver_buf, sizeof(last_ver_buf) - 1);
        last_ver_buf[sizeof(last_ver_buf) - 1] = '\0';
    }

    status_needs_redraw = 0;
}

/**
 * Draw combat message on line 21 (fixed position, no scrolling)
 */
void display_draw_combat_message(const char *message) {
    static char line_buf[41];
    
    if (!message) {
        return;
    }
    
    /* Clear line first */
    display_clear_line(21, DISPLAY_WIDTH);
    
    /* Draw message */
    snprintf(line_buf, sizeof(line_buf), "%s", message);
    display_puts_limited(0, 21, line_buf, DISPLAY_WIDTH);
}

/* Game Rendering */
void display_render_game(const player_state_t *local, const player_state_t *others, uint8_t count, int force_refresh) {
    static uint8_t last_player_x = 255;
    static uint8_t last_player_y = 255;
    static uint8_t last_other_positions[MAX_OTHER_PLAYERS * 2];  /* x,y pairs */
    static uint8_t last_other_count = 255;
    static int world_rendered = 0; /* Moved declaration to top */
    static int positions_initialized = 0;
    uint8_t x, y, i;
    char entity_char;
    
    /* Initialize position tracking to invalid values on first call */
    if (!positions_initialized) {
        for (i = 0; i < MAX_OTHER_PLAYERS * 2; i++) {
            last_other_positions[i] = 255;
        }
        positions_initialized = 1;
    }
    
    if (!local || local->x >= 255 || local->y >= 255) {
        return;
    }

    /* Full redraw on first render or when player count changes or refresh requested */
    
    if (force_refresh) {
        world_rendered = 0;
    }
    
    /* Only do full redraw on first render or explicit refresh */
    if (!world_rendered) {
        clrscr();
        status_needs_redraw = 1;
        /* Draw world line by line - this fills the play area */
        for (y = 0; y < DISPLAY_HEIGHT; y++) {
            for (x = 0; x < DISPLAY_WIDTH; x++) {
                cputcxy(x, y, CHAR_EMPTY);
            }
        }
        
        /* Draw other entities */
        for (i = 0; i < count; i++) {
            if (others[i].x < DISPLAY_WIDTH && others[i].y < DISPLAY_HEIGHT) {
                if (strcmp(others[i].type, "player") == 0) {
                    entity_char = CHAR_WALL;
                } else if (others[i].isHunter) {
                    entity_char = CHAR_HUNTER;
                } else {
                    entity_char = CHAR_ENEMY;
                }
                cputcxy(others[i].x, others[i].y, entity_char);
            }
        }
        
        /* Draw local player */
        if (local->x < DISPLAY_WIDTH && local->y < DISPLAY_HEIGHT) {
            cputcxy(local->x, local->y, CHAR_PLAYER);
        }
        
        last_player_x = local->x;
        last_player_y = local->y;
        last_other_count = count;
        world_rendered = 1;
        
        /* Initialize tracked positions */
        for (i = 0; i < count && i < MAX_OTHER_PLAYERS; i++) {
            if (others[i].x < DISPLAY_WIDTH && others[i].y < DISPLAY_HEIGHT) {
                last_other_positions[i * 2] = others[i].x;
                last_other_positions[i * 2 + 1] = others[i].y;
            } else {
                last_other_positions[i * 2] = 255;
                last_other_positions[i * 2 + 1] = 255;
            }
        }
        /* Mark remaining slots as invalid */
        for (i = count; i < MAX_OTHER_PLAYERS; i++) {
            last_other_positions[i * 2] = 255;
            last_other_positions[i * 2 + 1] = 255;
        }
        
    } else {
        /* INCREMENTAL UPDATE - no full redraw */
        
        /* If entity count decreased, erase old positions for removed entities */
        if (count < last_other_count) {
            for (i = count; i < last_other_count && i < MAX_OTHER_PLAYERS; i++) {
                uint8_t old_x = last_other_positions[i * 2];
                uint8_t old_y = last_other_positions[i * 2 + 1];
                if (old_x < DISPLAY_WIDTH && old_y < DISPLAY_HEIGHT) {
                    cputcxy(old_x, old_y, CHAR_EMPTY);
                }
                last_other_positions[i * 2] = 255;
                last_other_positions[i * 2 + 1] = 255;
            }
        }
        last_other_count = count;
        
        /* Update player position if changed */
        if (local->x != last_player_x || local->y != last_player_y) {
            /* Erase old player position */
            if (last_player_x < DISPLAY_WIDTH && last_player_y < DISPLAY_HEIGHT) {
                cputcxy(last_player_x, last_player_y, CHAR_EMPTY);
            }
            
            /* Draw new player position */
            if (local->x < DISPLAY_WIDTH && local->y < DISPLAY_HEIGHT) {
                cputcxy(local->x, local->y, CHAR_PLAYER);
            }
            
            last_player_x = local->x;
            last_player_y = local->y;
        }
        
        /* Update other entities incrementally */
        for (i = 0; i < count && i < MAX_OTHER_PLAYERS; i++) {
            uint8_t old_x = last_other_positions[i * 2];
            uint8_t old_y = last_other_positions[i * 2 + 1];
            uint8_t new_x_other = others[i].x;
            uint8_t new_y_other = others[i].y;
            
            /* If position changed, update it */
            if (old_x != new_x_other || old_y != new_y_other) {
                /* Erase old position (if valid) */
                if (old_x < DISPLAY_WIDTH && old_y < DISPLAY_HEIGHT) {
                    cputcxy(old_x, old_y, CHAR_EMPTY);
                }
                
                /* Draw new position */
                if (new_x_other < DISPLAY_WIDTH && new_y_other < DISPLAY_HEIGHT) {
                    if (strcmp(others[i].type, "player") == 0) {
                        entity_char = CHAR_WALL;
                    } else if (others[i].isHunter) {
                        entity_char = CHAR_HUNTER;
                    } else {
                        entity_char = CHAR_ENEMY;
                    }
                    cputcxy(new_x_other, new_y_other, entity_char);
                }
                
                /* Update tracked position */
                last_other_positions[i * 2] = new_x_other;
                last_other_positions[i * 2 + 1] = new_y_other;
            }
        }
    }

#ifdef _CMOC_VERSION_
    gotoxy(DISPLAY_WIDTH -1 , 23); /* Move cursor out of the way */
#endif

}

/* Dialogs and Prompts */

void display_show_join_prompt(void) {
    clrscr();
    gotoxy(0, 5);
    printf("Enter player name:\n");
    gotoxy(0, 7);
}

void display_show_rejoining(const char *name) {
    clrscr();
    gotoxy(0, 8);
    printf("  Rejoining as: %s\n", name);
    gotoxy(0, 10);
    printf("  Please wait...\n");
}

void display_show_quit_confirmation(void) {
    clrscr();
    gotoxy(0, 8);
    printf("  Are you sure you want to quit?\n");
    gotoxy(0, 10);
    printf("  Y=Quit  N=Continue Playing\n");
    gotoxy(0, 12);
    printf("  Press a key: ");
}

void display_show_connection_lost(void) {
    clrscr();
    gotoxy(0, 8);
    printf("  CONNECTION LOST\n");
    gotoxy(0, 10);
    printf("  You were disconnected from the server.\n");
    gotoxy(0, 12);
    printf("  Y=Quit  N=Rejoin\n");
    gotoxy(0, 14);
    printf("  Press a key: ");
}

void display_show_death_message(void) {
    clrscr();
    gotoxy(0, 8);
    printf("  *** YOU WERE KILLED! ***\n");
    gotoxy(0, 10);
    printf("  You have been eliminated in combat.\n");
    gotoxy(0, 12);
    printf("  Rejoin the game? (Y/N): ");
}

void display_show_error(const char *error) {
    clrscr();
    gotoxy(0, 10);
    printf("ERROR: %s\n", error);
}

void display_toggle_color_scheme(void) {
#ifdef _CMOC_VERSION_
    switch_colorset();
#endif
}
