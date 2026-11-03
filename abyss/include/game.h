#ifndef GAME_H
#define GAME_H
#include "physics.h"
#include "save.h"
#define EV_COIN 1
#define EV_HURT 2
#define EV_DEAD 4
#define EV_WIN 8
typedef struct {
  Player p; LevelState ls; SaveData *save;
  int level, hp, energy; uint8_t invuln, regen_wait; uint16_t regen_tick, tick;
} Game;
void game_start_level(Game *g, SaveData *s, int level);
uint8_t game_update(Game *g, Input in);
#endif
