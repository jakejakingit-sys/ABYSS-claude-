#include "animation.h"
int anim_player_frame(const Player *p, uint16_t tick) {
  if (p->ladder) return FR_JUMP;
  if (!p->ground) return p->vy < 0 ? FR_JUMP : FR_FALL;
  int v = p->vx < 0 ? -p->vx : p->vx;
  return v > 4 ? FR_RUN1 + ((tick / 6) & 1) : FR_IDLE;
}
