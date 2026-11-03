/* ABYSS - CE front end. UNVERIFIED: never compiled, never run. Written against graphx/keypadc from memory. */
#include <graphx.h>
#include <keypadc.h>
#include <tice.h>
#include <stdint.h>
#include "game.h"
#include "upgrades.h"
#include "abilities.h"
#include "animation.h"
#include "assets.h"
#include "platform.h"

enum { C_BG = 1, C_STONE, C_LIGHT, C_PURPLE, C_GREEN, C_SKIN, C_CLOAK, C_RUNE, C_RED, C_GOLD, C_WHITE, C_BROWN, C_OUTLINE };
static void init_palette(void) {
  static uint16_t pal[14];
  const uint8_t rgb[14][3] = {{0,0,0},{16,20,40},{70,74,90},{120,126,140},{90,60,120},{150,200,160},{230,190,150},{60,70,150},{80,240,240},{210,40,40},{250,210,60},{255,255,255},{120,80,40},{10,10,20}};
  for (int i = 0; i < 14; i++) pal[i] = gfx_RGBTo1555(rgb[i][0], rgb[i][1], rgb[i][2]);
  gfx_SetPalette(pal, sizeof pal, 0);
  gfx_SetTransparentColor(0);
}
static Input read_input(bool *clear) {
  kb_Scan();
  Input in = { (kb_Data[7] & kb_Left) != 0, (kb_Data[7] & kb_Right) != 0, (kb_Data[7] & kb_Up) != 0, (kb_Data[7] & kb_Down) != 0, (kb_Data[1] & kb_2nd) != 0 };
  *clear = (kb_Data[6] & kb_Clear) != 0;
  return in;
}
static bool pressed_enter(void) { static bool prev; bool now = kb_Data[6] & kb_Enter; bool r = now && !prev; prev = now; return r; }
static bool edge(bool now, bool *prev) { bool r = now && !*prev; *prev = now; return r; }

static void draw_tile(uint8_t t, int x, int y, int w) {
  switch (t) {
    case T_SOLID: gfx_SetColor(C_STONE); gfx_FillRectangle_NoClip(x, y, 16, 16);
      gfx_SetColor(C_LIGHT); gfx_HorizLine_NoClip(x, y, 16); gfx_HorizLine_NoClip(x, y + 8, 16);
      gfx_VertLine_NoClip(x + ((y / 8 + w) & 1 ? 4 : 12), y, 8); gfx_VertLine_NoClip(x + 8, y + 8, 8); break;
    case T_ONEWAY: gfx_SetColor(C_PURPLE); gfx_FillRectangle_NoClip(x, y, 16, 5); gfx_SetColor(C_LIGHT); gfx_HorizLine_NoClip(x, y, 16); break;
    case T_SPIKE: gfx_SetColor(C_RED); for (int i = 0; i < 16; i += 4) gfx_FillTriangle_NoClip(x + i, y + 15, x + i + 4, y + 15, x + i + 2, y + 6); break;
    case T_LADDER: gfx_SetColor(C_BROWN); gfx_VertLine_NoClip(x + 4, y, 16); gfx_VertLine_NoClip(x + 11, y, 16);
      for (int i = 2; i < 16; i += 5) gfx_HorizLine_NoClip(x + 4, y + i, 8); break;
    case T_COIN: gfx_SetColor(C_GOLD); gfx_FillCircle_NoClip(x + 8, y + 8, 4); break;
    case T_CHEST: gfx_SetColor(C_BROWN); gfx_FillRectangle_NoClip(x + 1, y + 6, 14, 10); gfx_SetColor(C_GOLD); gfx_FillRectangle_NoClip(x + 6, y + 9, 4, 3); break;
    case T_EXIT: gfx_SetColor(C_GREEN); gfx_Rectangle_NoClip(x + 2, y, 12, 16); gfx_SetColor(C_RUNE); gfx_FillRectangle_NoClip(x + 4, y + 2, 8, 14); break;
  }
}
static void render(const Game *g, const SaveData *s) {
  int cam = g->p.x / SUBPX + PW / 2 - 160, maxcam = g->ls.def->w * TILE_PX - 320;
  if (cam > maxcam) cam = maxcam;
  if (cam < 0) cam = 0;
  gfx_FillScreen(C_BG);
  int t0 = cam / TILE_PX;
  for (int ty = 0; ty < g->ls.def->h; ty++)
    for (int tx = t0; tx <= t0 + 20 && tx < g->ls.def->w; tx++) {
      uint8_t t = level_tile(&g->ls, tx, ty);
      if (t) draw_tile(t, tx * TILE_PX - cam, ty * TILE_PX, tx);
    }
  if (!(g->invuln & 4)) {   /* blink while invulnerable */
    gfx_TransparentSprite((gfx_sprite_t *)PLAYER_FRAMES[anim_player_frame(&g->p, g->tick)], g->p.x / SUBPX - cam, g->p.y / SUBPX);
  }
  gfx_SetColor(C_OUTLINE); gfx_FillRectangle_NoClip(4, 4, 102, 10);
  gfx_SetColor(C_RED); gfx_FillRectangle_NoClip(5, 5, g->hp * 100 / upgrade_max_hp(s), 8);
  gfx_SetColor(C_RUNE); gfx_FillRectangle_NoClip(5, 16, g->energy * 100 / upgrade_max_energy(s), 4);
  gfx_SetTextFGColor(C_GOLD); gfx_SetTextXY(240, 5); gfx_PrintString("COINS "); gfx_PrintUInt(s->coins, 1);
}
static void text(const char *s, int x, int y, uint8_t c) { gfx_SetTextFGColor(c); gfx_PrintStringXY(s, x, y); }

static void screen_play(SaveData *s) {   /* returns after a level session */
  int sel = 0; bool pl = false, pr = false, pe = false;
  for (;;) {
    kb_Scan(); if (kb_Data[6] & kb_Clear) return;
    if (edge(kb_Data[7] & kb_Left, &pl) && sel > 0) sel--;
    if (edge(kb_Data[7] & kb_Right, &pr) && sel < LEVEL_COUNT - 1) sel++;
    bool open = sel <= s->unlocked && sel < LEVELS_IMPLEMENTED;
    gfx_FillScreen(C_BG); text("SELECT LEVEL (</> ENTER, CLEAR=back)", 20, 10, C_WHITE);
    for (int i = 0; i < LEVEL_COUNT; i++) {
      int x = 20 + (i % 5) * 56, y = 40 + (i / 5) * 40;
      gfx_SetColor((s->completed >> i) & 1 ? C_GREEN : i <= s->unlocked && i < LEVELS_IMPLEMENTED ? C_LIGHT : C_STONE);
      gfx_FillRectangle(x, y, 44, 28); if (i == sel) { gfx_SetColor(C_GOLD); gfx_Rectangle(x - 2, y - 2, 48, 32); }
      if (i % 5 == 4) text("G", x + 18, y + 10, C_RED); else gfx_SetTextFGColor(C_OUTLINE), gfx_SetTextXY(x + 16, y + 10), gfx_PrintUInt(i + 1, 2);
    }
    text(sel < LEVELS_IMPLEMENTED ? LEVELS[sel].name : "NOT IMPLEMENTED YET", 20, 210, C_GOLD);
    gfx_SwapDraw();
    if (edge(kb_Data[6] & kb_Enter, &pe) && open) {
      Game g; game_start_level(&g, s, sel); bool clr = false;
      while (!clr) {
        Input in = read_input(&clr); uint8_t ev = game_update(&g, in);
        if (ev & EV_WIN) { platform_save_write(s); return; }
        if (ev & EV_DEAD) platform_save_write(s);
        render(&g, s); gfx_SwapDraw();
      }
      platform_save_write(s); return;
    }
  }
}
static void screen_upgrades(SaveData *s) {
  static const char *N[3] = {"ARMOR", "MAX HEALTH", "ENERGY"}; int sel = 0; bool pu = false, pd = false, pe = false;
  for (;;) {
    kb_Scan(); if (kb_Data[6] & kb_Clear) return;
    if (edge(kb_Data[7] & kb_Up, &pu) && sel > 0) sel--;
    if (edge(kb_Data[7] & kb_Down, &pd) && sel < 2) sel++;
    if (edge(kb_Data[6] & kb_Enter, &pe) && upgrade_buy(s, (UpgCat)sel) == BUY_OK) platform_save_write(s);
    gfx_FillScreen(C_BG); text("UPGRADES", 20, 10, C_WHITE); gfx_SetTextXY(20, 28); gfx_SetTextFGColor(C_GOLD); gfx_PrintString("COINS "); gfx_PrintUInt(s->coins, 1);
    for (int i = 0; i < 3; i++) {
      int y = 60 + i * 30; if (i == sel) text(">", 10, y, C_GOLD);
      text(N[i], 24, y, C_WHITE); gfx_SetTextXY(150, y); gfx_PrintString("LV "); gfx_PrintUInt(s->upg[i], 1); gfx_PrintString("/5  COST ");
      int c = upgrade_cost((UpgCat)i, s->upg[i]); if (c < 0) gfx_PrintString("MAX"); else gfx_PrintUInt(c, 1);
    }
    gfx_SwapDraw();
  }
}
static void screen_stats(const SaveData *s, bool collection) {
  int done = 0, tre = 0; for (int i = 0; i < LEVEL_COUNT; i++) { done += (s->completed >> i) & 1; for (int b = 0; b < 32; b++) tre += (s->treasure[i] >> b) & 1; }
  for (;;) {
    kb_Scan(); if (kb_Data[6] & kb_Clear) return;
    gfx_FillScreen(C_BG); text(collection ? "COLLECTION" : "STATS", 20, 10, C_WHITE);
    gfx_SetTextFGColor(C_LIGHT);
    if (!collection) {
      gfx_SetTextXY(20, 40); gfx_PrintString("Levels completed: "); gfx_PrintUInt(done, 1); gfx_PrintString("/20");
      gfx_SetTextXY(20, 56); gfx_PrintString("Guardians defeated: "); gfx_PrintUInt(s->guardians, 1);
      gfx_SetTextXY(20, 72); gfx_PrintString("Deaths: "); gfx_PrintUInt(s->deaths, 1);
      gfx_SetTextXY(20, 88); gfx_PrintString("Treasures found: "); gfx_PrintUInt(tre, 1);
      gfx_SetTextXY(20, 104); gfx_PrintString("Secrets: "); gfx_PrintUInt(s->secrets, 1);
    } else {
      for (int i = 0; i < ABILITY_COUNT; i++) text((s->abilities >> i) & 1 ? ABILITY_NAMES[i] : "???", 20, 40 + i * 16, (s->abilities >> i) & 1 ? C_GOLD : C_STONE);
      text("Relics: not implemented", 20, 170, C_STONE);
    }
    gfx_SwapDraw();
  }
}
int main(void) {
  static SaveData save; platform_save_load(&save);
  gfx_Begin(); gfx_SetDrawBuffer(); init_palette(); gfx_SetTextScale(1, 1);
  static const char *M[4] = {"PLAY", "UPGRADES", "COLLECTION", "STATS"}; int sel = 0; bool pu = false, pd = false, pe = false;
  for (;;) {
    kb_Scan(); if (kb_Data[6] & kb_Clear) break;
    if (edge(kb_Data[7] & kb_Up, &pu) && sel > 0) sel--;
    if (edge(kb_Data[7] & kb_Down, &pd) && sel < 3) sel++;
    if (edge(kb_Data[6] & kb_Enter, &pe)) { if (sel == 0) screen_play(&save); else if (sel == 1) screen_upgrades(&save); else screen_stats(&save, sel == 2); }
    gfx_FillScreen(C_BG); gfx_SetTextScale(3, 3); text("ABYSS", 100, 20, C_RUNE); gfx_SetTextScale(1, 1);
    text("THE WAYSTATION", 110, 60, C_LIGHT);
    for (int i = 0; i < 4; i++) text(M[i], 130, 100 + i * 24, i == sel ? C_GOLD : C_WHITE);
    text(">", 112, 100 + sel * 24, C_GOLD); text("CLEAR = quit", 110, 220, C_STONE);
    gfx_SwapDraw();
  }
  gfx_End(); return 0;
}
