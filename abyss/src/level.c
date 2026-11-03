#include "level.h"
uint8_t level_raw(const LevelDef *d, int tx, int ty) {
  if (tx < 0 || tx >= d->w || ty < 0) return T_SOLID;   /* walls + ceiling */
  if (ty >= d->h) return T_AIR;                          /* bottomless pit */
  return d->tiles[ty * d->w + tx];
}
static bool is_treasure(uint8_t t) { return t == T_COIN || t == T_CHEST; }
uint8_t level_tile(const LevelState *l, int tx, int ty) {
  uint8_t t = level_raw(l->def, tx, ty);
  if (is_treasure(t)) {
    int i = ty * l->def->w + tx;
    if (l->taken[i >> 3] & (1 << (i & 7))) return T_AIR;
  }
  return t;
}
int level_treasure_count(const LevelDef *d) {
  int n = 0;
  for (int i = 0; i < d->w * d->h; i++) if (is_treasure(d->tiles[i])) n++;
  return n;
}
void level_load(LevelState *l, int idx, uint32_t bits) {
  l->def = &LEVELS[idx];
  for (unsigned i = 0; i < sizeof l->taken; i++) l->taken[i] = 0;
  int k = 0;
  for (int i = 0; i < l->def->w * l->def->h; i++)
    if (is_treasure(l->def->tiles[i])) { if (k < MAX_TREASURE && (bits >> k) & 1) l->taken[i >> 3] |= 1 << (i & 7); k++; }
}
uint32_t level_treasure_bits(const LevelState *l) {
  uint32_t bits = 0; int k = 0;
  for (int i = 0; i < l->def->w * l->def->h; i++)
    if (is_treasure(l->def->tiles[i])) { if (k < MAX_TREASURE && (l->taken[i >> 3] & (1 << (i & 7)))) bits |= 1UL << k; k++; }
  return bits;
}
int level_take(LevelState *l, int tx, int ty) {
  uint8_t t = level_tile(l, tx, ty);
  if (!is_treasure(t)) return 0;
  int i = ty * l->def->w + tx;
  l->taken[i >> 3] |= 1 << (i & 7);
  return t == T_CHEST ? CHEST_VALUE : COIN_VALUE;
}
