/**
 * KillZone Constants - Atari 8-bit
 * 
 * Centralized configuration and constants.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

/* Game Info */
#define GAME_TITLE "KillZone"
#define CLIENT_VERSION "1.2.0"

/* Display Dimensions */
#define DISPLAY_WIDTH 40
#define DISPLAY_HEIGHT 20
#define STATUS_BAR_HEIGHT 4
#define STATUS_BAR_START (DISPLAY_HEIGHT)

/* Display Characters */
#define CHAR_EMPTY '.'
#define CHAR_PLAYER '@'
#define CHAR_ENEMY '*'
#define CHAR_HUNTER '+'
#define CHAR_WALL '#'

/* Player Limits */
#define MAX_OTHER_PLAYERS 10
#define PLAYER_NAME_MAX 32

/* Server Configuration */
#define SERVER_HOST "fujinet.diller.org"
#define SERVER_PORT 3000
#define SERVER_PROTO "http"
#define SERVER_TCP_PORT 6809

/* Network Configuration */
#define DEVICE_SPEC_SIZE 256

#endif /* CONSTANTS_H */
