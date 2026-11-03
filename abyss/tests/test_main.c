#include <stdio.h>
#include <string.h>
#include "game.h"
#include "upgrades.h"
#include "abilities.h"
#include "animation.h"
static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
static Input NONE;

static void run_until_ground(Player *p, LevelState *l) { for (int i = 0; i < 200 && !p->ground; i++) player_step(p, l, NONE); }

static void test_physics(void) {
  puts("physics");
  LevelState l; level_load(&l, 0, 0); Player p; player_spawn(&p, l.def);
  run_until_ground(&p, &l);
  CHECK(p.ground);
  CHECK(floor_div(p.y + PH * SUBPX, TILE_SUB) == 13);          /* resting exactly on floor top (row 13) */
  /* acceleration + max speed */
  Input r = {.right = true};
  for (int i = 0; i < 30; i++) player_step(&p, &l, r);
  CHECK(p.vx == MAXV);
  for (int i = 0; i < 20; i++) player_step(&p, &l, NONE);
  CHECK(p.vx == 0);                                              /* deceleration */
  /* variable jump: tap vs hold */
  Player a = p, b = p; int ya = a.y, yb = b.y, topa = a.y, topb = b.y;
  Input jh = {.jump = true};
  player_step(&a, &l, jh); player_step(&b, &l, jh);
  for (int i = 0; i < 60; i++) { player_step(&a, &l, NONE); player_step(&b, &l, jh); if (a.y < topa) topa = a.y; if (b.y < topb) topb = b.y; }
  CHECK(ya - topa < yb - topb);                                  /* held jump goes higher */
  CHECK((yb - topb) / SUBPX >= 36 && (yb - topb) / SUBPX <= 56); /* ~3 tiles, matches reachability rule */
  /* coyote time: walk off ledge then jump within window */
  LevelState l2; level_load(&l2, 0, 0); Player c; player_spawn(&c, l2.def);
  c.x = 14 * TILE_SUB - 4 * SUBPX; run_until_ground(&c, &l2);   /* near pit edge (cols 15-17 are the pit) */
  CHECK(c.ground);
  for (int i = 0; i < 40 && c.ground; i++) player_step(&c, &l2, (Input){.right = true});
  CHECK(!c.ground);
  int vy_before = c.vy;
  player_step(&c, &l2, (Input){.right = true, .jump = true});
  CHECK(c.vy < vy_before);                                       /* jumped while airborne: coyote works */
  /* jump buffering: press jump just before landing */
  Player d; player_spawn(&d, l.def); d.y -= TILE_SUB; d.vy = 40;
  bool jumped = false;
  for (int i = 0; i < 40; i++) { player_step(&d, &l, (Input){.jump = (i == 1)}); if (d.vy < -50) jumped = true; }
  CHECK(jumped);
  /* wall: left boundary */
  Player w; player_spawn(&w, l.def); run_until_ground(&w, &l);
  for (int i = 0; i < 200; i++) player_step(&w, &l, (Input){.left = true});
  CHECK(w.x >= TILE_SUB);                                        /* never inside the border wall */
  /* no tunnelling at terminal velocity */
  Player t; player_spawn(&t, l.def); t.x = 5 * TILE_SUB; t.y = TILE_SUB + 1; t.vy = MAXFALL;
  for (int i = 0; i < 200; i++) player_step(&t, &l, NONE);
  CHECK(t.ground && t.y < 14 * TILE_SUB);
  /* one-way platform: land from above, pass from below, drop through */
  LevelState l3; level_load(&l3, 1, 0); Player o; player_spawn(&o, l3.def);
  o.x = 17 * TILE_SUB; o.y = 6 * TILE_SUB; o.vy = 0;           /* above the row-10 platform (cols 16-19) */
  run_until_ground(&o, &l3);
  CHECK(o.ground && floor_div(o.y + PH * SUBPX, TILE_SUB) == 10);
  player_step(&o, &l3, (Input){.down = true});
  for (int i = 0; i < 15; i++) player_step(&o, &l3, NONE);
  CHECK(floor_div(o.y + PH * SUBPX, TILE_SUB) > 10);           /* dropped through */
  Player u; player_spawn(&u, l3.def); u.x = 17 * TILE_SUB; u.y = 12 * TILE_SUB - 3 * SUBPX; u.vy = JUMPV;
  int mn = u.y; for (int i = 0; i < 20; i++) { player_step(&u, &l3, NONE); if (u.y < mn) mn = u.y; }
  CHECK(mn < 10 * TILE_SUB);                                     /* rose through platform from below */
  /* ladder */
  Player k; player_spawn(&k, l.def); k.x = 30 * TILE_SUB + 3 * SUBPX; k.y = 12 * TILE_SUB - 4; run_until_ground(&k, &l);
  for (int i = 0; i < 20; i++) player_step(&k, &l, (Input){.up = true});
  CHECK(k.ladder == 1 && anim_player_frame(&k, 0) == FR_JUMP);   /* on ladder: climbing frame */
  for (int i = 0; i < 100; i++) player_step(&k, &l, (Input){.up = true});
  CHECK(k.y < 8 * TILE_SUB);                                     /* climbed up to ledge at row 6 */
}

static void test_collision_hazards(void) {
  puts("collision/hazards");
  LevelState l; level_load(&l, 0, 0);
  CHECK(level_tile(&l, -1, 5) == T_SOLID && level_tile(&l, 5, -1) == T_SOLID && level_tile(&l, 5, 99) == T_AIR);
  Game g; SaveData s; save_default(&s); game_start_level(&g, &s, 0);
  for (int i = 0; i < 20; i++) game_update(&g, NONE);
  g.p.x = 26 * TILE_SUB; g.p.y = 12 * TILE_SUB - 2 * SUBPX; g.p.vy = 0;
  uint8_t ev = 0; for (int i = 0; i < 5; i++) ev |= game_update(&g, NONE);
  CHECK(ev & EV_HURT); CHECK(g.hp == 80);                       /* 20 dmg, 0 armor */
  int hp = g.hp; g.invuln = 30; g.p.x = 26 * TILE_SUB; g.p.y = 12 * TILE_SUB; game_update(&g, NONE);
  CHECK(g.hp == hp);                                             /* damage cooldown */
  /* regen interrupted then resumes */
  g.invuln = 0; g.p.x = 3 * TILE_SUB; g.p.y = 12 * TILE_SUB; g.regen_wait = 120; hp = g.hp;
  for (int i = 0; i < 100; i++) game_update(&g, NONE);
  CHECK(g.hp == hp);
  for (int i = 0; i < 200; i++) game_update(&g, NONE);
  CHECK(g.hp > hp);
  /* armour reduces damage */
  save_default(&s); s.upg[UPG_ARMOR] = 3; game_start_level(&g, &s, 0);
  g.p.x = 26 * TILE_SUB; g.p.y = 12 * TILE_SUB - 2 * SUBPX; for (int i = 0; i < 5; i++) game_update(&g, NONE);
  CHECK(g.hp == 100 - (20 - 6));
}

static void test_progression_treasure(void) {
  puts("progression/treasure/death");
  SaveData s; save_default(&s); Game g; game_start_level(&g, &s, 0);
  CHECK(s.unlocked == 0);
  /* collect first coin */
  g.p.x = 8 * TILE_SUB + 3 * SUBPX; g.p.y = 12 * TILE_SUB; game_update(&g, NONE);
  CHECK(s.coins == COIN_VALUE); CHECK(s.treasure[0] != 0);
  /* same coin cannot be re-collected after re-entry */
  uint32_t c = s.coins; game_start_level(&g, &s, 0);
  g.p.x = 8 * TILE_SUB + 3 * SUBPX; g.p.y = 12 * TILE_SUB; game_update(&g, NONE);
  CHECK(s.coins == c);
  /* coins + treasure persist through death; death restarts level */
  g.hp = 0; uint8_t ev = game_update(&g, NONE);
  CHECK(ev & EV_DEAD); CHECK(s.deaths == 1); CHECK(s.coins == c); CHECK(g.hp == 100);
  CHECK(level_tile(&g.ls, 8, 12) == T_AIR);
  /* falling in a pit kills */
  g.p.x = 16 * TILE_SUB; g.p.y = 14 * TILE_SUB; ev = 0; for (int i = 0; i < 60 && !(ev & EV_DEAD); i++) ev |= game_update(&g, NONE);
  CHECK(ev & EV_DEAD);
  /* exit completes + unlocks next */
  g.p.x = 57 * TILE_SUB; g.p.y = 12 * TILE_SUB; ev = game_update(&g, NONE);
  CHECK(ev & EV_WIN); CHECK(s.completed & 1); CHECK(s.unlocked == 1);
}

static void test_upgrades(void) {
  puts("upgrades");
  SaveData s; save_default(&s); s.coins = 49;
  CHECK(upgrade_buy(&s, UPG_HEALTH) == BUY_NO_COINS && s.coins == 49 && s.upg[UPG_HEALTH] == 0);
  s.coins = 50; CHECK(upgrade_buy(&s, UPG_HEALTH) == BUY_OK); CHECK(s.coins == 0 && upgrade_max_hp(&s) == 120);
  CHECK(upgrade_cost(UPG_HEALTH, 1) > upgrade_cost(UPG_HEALTH, 0));          /* rising prices */
  s.coins = 100000;
  for (int c = 0; c < UPG_COUNT; c++) { for (int i = 0; i < 20; i++) upgrade_buy(&s, c); CHECK(s.upg[c] == UPG_MAX_LEVEL); CHECK(upgrade_buy(&s, c) == BUY_MAXED); }
  CHECK(s.coins <= 100000);                                                   /* never negative (unsigned) / never grows */
  CHECK(upgrade_cost(UPG_ARMOR, UPG_MAX_LEVEL) == -1);
  CHECK(upgrade_max_hp(&s) == 200 && upgrade_max_energy(&s) == 200);
}

static void test_abilities(void) {
  puts("abilities");
  CHECK(abilities_for_level(0) == 1);
  CHECK(abilities_for_level(1) == 1);
  CHECK(abilities_for_level(2) == 3);       /* Heavy Landing at level 3 */
  CHECK(abilities_for_level(19) == 127);
  SaveData s; save_default(&s); Game g; game_start_level(&g, &s, 1);
  CHECK(s.abilities == 1);
}

static void test_save(void) {
  puts("save/load/corruption");
  SaveData a; save_default(&a); a.coins = 1234; a.unlocked = 3; a.completed = 0x7; a.upg[0] = 2; a.treasure[1] = 0xA5; a.deaths = 9; a.abilities = 7;
  uint8_t buf[SAVE_BYTES]; save_serialize(&a, buf);
  SaveData b; CHECK(save_parse(&b, buf, SAVE_BYTES) == SAVE_OK); CHECK(memcmp(&a, &b, sizeof a) == 0);
  for (int i = 0; i < SAVE_BYTES; i++) {                          /* every single-bit flip must be rejected */
    for (int bit = 0; bit < 8; bit++) {
      uint8_t c[SAVE_BYTES]; memcpy(c, buf, sizeof c); c[i] ^= 1 << bit;
      SaveData t; if (save_parse(&t, c, SAVE_BYTES) == SAVE_OK) { fails++; checks++; printf("  FAIL undetected flip byte %d bit %d\n", i, bit); }
    }
  }
  CHECK(save_parse(&b, buf, SAVE_BYTES - 1) == SAVE_BAD_LEN);
  uint8_t bad[SAVE_BYTES]; memcpy(bad, buf, sizeof bad); bad[4] = 99;
  CHECK(save_parse(&b, bad, SAVE_BYTES) == SAVE_BAD_VERSION);
  SaveData r; memset(&r, 0xEE, sizeof r); bad[10] ^= 0xFF;
  CHECK(save_load_or_default(&r, bad, SAVE_BYTES) != SAVE_OK); CHECK(r.coins == 0 && r.unlocked == 0 && r.abilities == 1);  /* recovered */
  CHECK(save_load_or_default(&r, NULL, 0) != SAVE_OK);
  /* valid CRC but out-of-range content is still rejected */
  SaveData evil = a; evil.unlocked = 200; save_serialize(&evil, buf); CHECK(save_parse(&b, buf, SAVE_BYTES) == SAVE_BAD_RANGE);
  CHECK(crc16((const uint8_t *)"123456789", 9) == 0x29B1);          /* CRC-16/CCITT-FALSE check value */
}

/* ---- level validation: dimensions, spawn, exit, reachability (heuristic), treasure ---- */
static bool standable(const LevelDef *d, int x, int y) {
  uint8_t here = level_raw(d, x, y), below = level_raw(d, x, y + 1);
  return (here != T_SOLID) && (here != T_SPIKE) && (below == T_SOLID || below == T_ONEWAY || here == T_LADDER);
}
static int reach(const LevelDef *d, uint8_t *seen) {
  memset(seen, 0, MAX_LVL_W * MAX_LVL_H);
  static int q[MAX_LVL_W * MAX_LVL_H]; int h = 0, t = 0;
  int sx = d->spawn_tx, sy = d->spawn_ty;
  /* spawn may stand on nothing special: drop to ground */
  while (sy + 1 < d->h && !standable(d, sx, sy)) sy++;
  q[t++] = sy * d->w + sx; seen[sy * d->w + sx] = 1;
  while (h < t) {
    int cur = q[h++], cx = cur % d->w, cy = cur / d->w;
    for (int ty = 0; ty < d->h; ty++) for (int tx = cx - 4; tx <= cx + 4; tx++) {
      if (tx < 1 || tx >= d->w - 1 || seen[ty * d->w + tx] || !standable(d, tx, ty)) continue;
      if (cy - ty > 3) continue;                                              /* max jump height: 3 tiles */
      bool ok = true;                                                         /* headroom: column above target free */
      if (level_raw(d, tx, ty - 1) == T_SOLID) ok = false;
      if (ok) { seen[ty * d->w + tx] = 1; q[t++] = ty * d->w + tx; }
    }
    if (level_raw(d, cx, cy) == T_LADDER)                                      /* ladders connect vertically */
      for (int dy = -1; dy <= 1; dy += 2) { int ny = cy + dy;
        if (ny > 0 && ny < d->h && !seen[ny * d->w + cx] && level_raw(d, cx, ny) != T_SOLID && (level_raw(d, cx, ny) == T_LADDER || standable(d, cx, ny)))
        { seen[ny * d->w + cx] = 1; q[t++] = ny * d->w + cx; } }
    /* climbing off the ladder top onto a one-way ledge */
    if (level_raw(d, cx, cy) == T_LADDER && cy > 0 && level_raw(d, cx, cy - 1) == T_ONEWAY && !seen[(cy - 2) * d->w + cx])
      { seen[(cy - 2) * d->w + cx] = 1; q[t++] = (cy - 2) * d->w + cx; }
  }
  return t;
}
static void test_levels(void) {
  puts("level validation (2 of 20 implemented)");
  CHECK(LEVELS_IMPLEMENTED <= LEVEL_COUNT);
  static uint8_t seen[MAX_LVL_W * MAX_LVL_H];
  for (int i = 0; i < LEVELS_IMPLEMENTED; i++) {
    const LevelDef *d = &LEVELS[i];
    printf("  L%02d %-16s %dx%d world %d\n", i + 1, d->name, d->w, d->h, d->world);
    CHECK(d->w >= 20 && d->w <= MAX_LVL_W && d->h == MAX_LVL_H);
    CHECK(d->world == i / 5 + 1);
    CHECK(d->spawn_tx > 0 && d->spawn_tx < d->w && level_raw(d, d->spawn_tx, d->spawn_ty) == T_AIR);
    CHECK(level_raw(d, d->spawn_tx, d->spawn_ty + 1) == T_SOLID);
    int exits = 0, tre = 0, bad = 0;
    for (int y = 0; y < d->h; y++) for (int x = 0; x < d->w; x++) {
      uint8_t t = level_raw(d, x, y); if (t == T_EXIT) exits++; if (t == T_COIN || t == T_CHEST) tre++;
      if (t >= 8) bad++;
      if (t == T_SPIKE && level_raw(d, x, y + 1) != T_SOLID && level_raw(d, x, y + 1) != T_SPIKE) bad++;   /* spikes must sit on ground */
      if ((t == T_COIN || t == T_CHEST) && (x == 0 || y == 0 || x == d->w - 1)) bad++;
    }
    CHECK(exits == 1); CHECK(bad == 0); CHECK(tre >= 1 && tre <= MAX_TREASURE);
    CHECK(level_treasure_count(d) == tre);
    reach(d, seen);
    int ex = 0, ey = 0; for (int y = 0; y < d->h; y++) for (int x = 0; x < d->w; x++) if (level_raw(d, x, y) == T_EXIT) { ex = x; ey = y; }
    CHECK(seen[ey * d->w + ex] || seen[ey * d->w + ex - 1] || seen[(ey + 0) * d->w + ex + 0]);
    int chest_ok = 0; for (int y = 0; y < d->h; y++) for (int x = 0; x < d->w; x++) if (level_raw(d, x, y) == T_CHEST) chest_ok += seen[y * d->w + x] || seen[(y + 1) * d->w + x];
    CHECK(chest_ok >= 1);                                                       /* secret chest reachable */
  }
  /* treasure bit round-trip */
  LevelState l; level_load(&l, 0, 0); level_take(&l, 8, 12); level_take(&l, 31, 5);
  uint32_t b = level_treasure_bits(&l); LevelState m; level_load(&m, 0, b); CHECK(level_treasure_bits(&m) == b && level_tile(&m, 8, 12) == T_AIR);
}

static void test_memory(void) {
  puts("memory (HOST sizeof - not a measurement of the calculator build)");
  printf("  sizeof Game=%zu Player=%zu LevelState=%zu SaveData=%zu  save file=%d bytes\n", sizeof(Game), sizeof(Player), sizeof(LevelState), sizeof(SaveData), SAVE_BYTES);
  CHECK(sizeof(Game) < 4096); CHECK(sizeof(LevelState) < 512);
  size_t lvl = 0; for (int i = 0; i < LEVELS_IMPLEMENTED; i++) lvl += LEVELS[i].w * LEVELS[i].h;
  printf("  level data: %zu bytes for %d levels (const, not RAM); worst case 20x960 = %d\n", lvl, LEVELS_IMPLEMENTED, 20 * MAX_LVL_W * MAX_LVL_H);
  CHECK(20 * MAX_LVL_W * MAX_LVL_H < 60000);
}

int main(void) {
  test_physics(); test_collision_hazards(); test_progression_treasure(); test_upgrades();
  test_abilities(); test_save(); test_levels(); test_memory();
  printf("\n%d checks, %d failed\n", checks, fails);
  return fails != 0;
}
