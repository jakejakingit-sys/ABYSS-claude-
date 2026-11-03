#ifndef LEVEL_H
#define LEVEL_H
#include "abyss.h"
typedef struct { uint8_t world, w, h, spawn_tx, spawn_ty; const uint8_t *tiles; const char *name; } LevelDef;
typedef struct { const LevelDef *def; uint8_t taken[(MAX_LVL_W * MAX_LVL_H + 7) / 8]; } LevelState;
extern const LevelDef LEVELS[LEVELS_IMPLEMENTED];
uint8_t level_raw(const LevelDef *d, int tx, int ty);
uint8_t level_tile(const LevelState *l, int tx, int ty);
void level_load(LevelState *l, int idx, uint32_t treasure_bits);
uint32_t level_treasure_bits(const LevelState *l);
int level_treasure_count(const LevelDef *d);
int level_take(LevelState *l, int tx, int ty);   /* returns coin value, 0 if nothing */
#endif
