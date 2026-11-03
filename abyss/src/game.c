#include "game.h"
#include "abilities.h"
#include "upgrades.h"
#define BASE_SPIKE_DMG 20
static void respawn(Game *g) { player_spawn(&g->p, g->ls.def); g->hp = upgrade_max_hp(g->save); g->invuln = 0; g->regen_wait = 0; }
void game_start_level(Game *g, SaveData *s, int level) {
  g->save = s; g->level = level; g->tick = 0; g->regen_tick = 0;
  level_load(&g->ls, level, s->treasure[level]);
  s->abilities |= abilities_for_level(level);
  g->energy = upgrade_max_energy(s);
  respawn(g);
}
static void hurt(Game *g, uint8_t *ev) {
  int dmg = BASE_SPIKE_DMG - upgrade_armor(g->save); if (dmg < 1) dmg = 1;
  g->hp -= dmg; g->invuln = 45; g->regen_wait = 120;
  g->p.vy = -60; g->p.vx = g->p.face ? -MAXV : MAXV; g->p.ladder = 0;
  *ev |= EV_HURT;
}
uint8_t game_update(Game *g, Input in) {
  uint8_t ev = 0, pe = player_step(&g->p, &g->ls, in);
  g->tick++;
  if (g->invuln) g->invuln--;
  if (g->regen_wait) g->regen_wait--;
  else if (g->hp > 0 && g->hp < upgrade_max_hp(g->save) && ++g->regen_tick >= 30) { g->hp++; g->regen_tick = 0; }
  if ((pe & EV_SPIKE) && !g->invuln) hurt(g, &ev);
  if (pe & EV_PIT) g->hp = 0;
  /* pickups: any treasure tile touched by the hitbox */
  int x0 = floor_div(g->p.x, TILE_SUB), x1 = floor_div(g->p.x + PW * SUBPX - 1, TILE_SUB);
  int y0 = floor_div(g->p.y, TILE_SUB), y1 = floor_div(g->p.y + PH * SUBPX - 1, TILE_SUB);
  for (int ty = y0; ty <= y1; ty++) for (int tx = x0; tx <= x1; tx++) {
    int v = level_take(&g->ls, tx, ty);
    if (v) {
      g->save->coins += v; if (g->save->coins > COINS_CAP) g->save->coins = COINS_CAP;
      g->save->treasure[g->level] = level_treasure_bits(&g->ls); ev |= EV_COIN;
    }
  }
  if (g->hp <= 0) {                      /* death: level restarts, coins + treasure records persist */
    g->save->deaths++; ev |= EV_DEAD; respawn(g); return ev;
  }
  if (player_overlaps(&g->p, &g->ls, T_EXIT)) {
    g->save->completed |= 1UL << g->level;
    if (g->level + 1 < LEVEL_COUNT && g->save->unlocked < g->level + 1) g->save->unlocked = g->level + 1;
    ev |= EV_WIN;
  }
  return ev;
}
