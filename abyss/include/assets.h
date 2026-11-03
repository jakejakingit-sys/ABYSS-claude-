#ifndef ASSETS_H
#define ASSETS_H
#include <stdint.h>
/* Each frame: [width, height, indexed pixels...]; index 0 = transparent. Layout matches graphx gfx_sprite_t. */
extern const uint8_t PLAYER_FRAMES[5][2 + 10 * 14];
#endif
