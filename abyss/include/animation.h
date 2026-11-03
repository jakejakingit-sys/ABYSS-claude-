#ifndef ANIMATION_H
#define ANIMATION_H
#include "physics.h"
enum { FR_IDLE = 0, FR_RUN1, FR_RUN2, FR_JUMP, FR_FALL, FR_COUNT };
int anim_player_frame(const Player *p, uint16_t tick);
#endif
