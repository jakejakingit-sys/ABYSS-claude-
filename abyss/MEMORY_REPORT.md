# Memory report

**Source: host x86-64 gcc `sizeof` and generated data sizes. NOT measured from a CEdev link map** (no toolchain here). eZ80 int is 24-bit so real struct sizes differ.

```
memory (HOST sizeof - not a measurement of the calculator build)
  sizeof Game=184 Player=24 LevelState=128 SaveData=100  save file=107 bytes
  level data: 1860 bytes for 2 levels (const, not RAM); worst case 20x960 = 19200

```

## Budget (design intent, unverified on device)
| Item | Bytes (approx) | Where |
|---|---|---|
| Game struct (player, level state, counters) | ~184 | RAM |
| SaveData | ~100 | RAM |
| Save file | 107 | archive (written only on level end / purchase / death) |
| Level data, 2 levels | 1,860 | const (not copied to RAM) |
| Worst case 20 levels at 64x15, 1 byte/tile | 19,200 | const; real levels are smaller |
| Sprites, 5 frames x 142 | 710 | const |
| Framebuffer | 0 | graphx double-buffers in VRAM, no RAM framebuffer |

Not yet done: a real link-map measurement of code size, total RAM, and the compressed `.8xp` size.
