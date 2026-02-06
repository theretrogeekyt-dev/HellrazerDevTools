/* 3D_MAZE - Phase 2 (C89-safe): 16.16 fixed-point raycaster (OpenWatcom 16-bit DOS)
   - INT 9 key-down table
   - Doom-ish 35Hz tic movement
   - Mode 13h backbuffer + v-retrace blit
   - 80 rays (4px columns)
*/

#include <dos.h>
#include <conio.h>
#include <math.h>
#include <string.h>

/* ========= Video / world ========= */
#define W 320
#define H 200
#define MW 16
#define MH 16

static const unsigned char world[MH][MW] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,2,0,2,0,2,0,2,0,2,0,2,0,0,1},
  {1,0,2,0,0,0,0,0,0,0,0,0,2,0,0,1},
  {1,0,0,0,0,0,0,3,3,0,0,0,2,0,0,1},
  {1,0,2,0,2,0,0,3,3,0,2,0,2,0,0,1},
  {1,0,2,0,0,0,0,0,0,0,2,0,2,0,0,1},
  {1,0,2,2,2,2,0,2,2,0,2,0,2,0,0,1},
  {1,0,0,0,0,2,0,0,2,0,0,0,0,0,0,1},
  {1,0,2,2,0,2,2,0,2,2,2,2,2,2,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,2,0,1},
  {1,0,2,2,2,2,2,2,2,2,2,2,0,2,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,2,0,0,0,1},
  {1,0,2,2,2,2,2,2,2,2,0,2,2,2,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static unsigned char far* vga = (unsigned char far*)MK_FP(0xA000, 0);
static unsigned char far backbuf[64000];

/* ========= Fixed-point (16.16) ========= */
typedef long fixed;
#define FIX_SHIFT 16
#define FIX_ONE   ((fixed)1L << FIX_SHIFT)

static fixed f_from_int(int v) { return ((fixed)v) << FIX_SHIFT; }
static int   f_to_int(fixed v) { return (int)(v >> FIX_SHIFT); }

static fixed fmul(fixed a, fixed b) {
  /* OpenWatcom supports long long in 16-bit builds */
  return (fixed)(((long long)a * (long long)b) >> FIX_SHIFT);
}

static fixed f_abs(fixed v) { return (v < 0) ? -v : v; }

/* ========= Keyboard (key-down table via INT 9) ========= */
static volatile unsigned char keyDown[256];
static volatile unsigned char sawE0 = 0;
static void (interrupt far *oldInt9)(void) = 0;

static void interrupt far kb_isr(void) {
  unsigned char sc;
  unsigned char isBreak;
  unsigned char code;
  unsigned char idx;

  sc = (unsigned char)inp(0x60);

  if (sc == 0xE0) {
    sawE0 = 1;
    outp(0x20, 0x20);
    return;
  }

  isBreak = (unsigned char)(sc & 0x80);
  code    = (unsigned char)(sc & 0x7F);
  idx     = code;

  if (sawE0) {
    idx = (unsigned char)(0x80 | code);
    sawE0 = 0;
  }

  keyDown[idx] = isBreak ? 0 : 1;

  outp(0x20, 0x20);
}

static void kb_install(void) {
  int i;
  for (i = 0; i < 256; i++) keyDown[i] = 0;
  sawE0 = 0;

  _disable();
  oldInt9 = _dos_getvect(9);
  _dos_setvect(9, kb_isr);
  _enable();
}

static void kb_uninstall(void) {
  _disable();
  if (oldInt9) _dos_setvect(9, oldInt9);
  _enable();
}

/* Scancodes */
#define K_ESC        0x01
#define K_LSHIFT     0x2A
#define K_RSHIFT     0x36
#define K_LALT       0x38

#define K_W          0x11
#define K_A          0x1E
#define K_S          0x1F
#define K_D          0x20

#define K_UP_E0      (0x80 | 0x48)
#define K_LEFT_E0    (0x80 | 0x4B)
#define K_RIGHT_E0   (0x80 | 0x4D)
#define K_DOWN_E0    (0x80 | 0x50)

/* ========= Timing ========= */
#define TICRATE 35

static unsigned long bios_ticks(void) {
  unsigned long far *p;
  p = (unsigned long far*)MK_FP(0x0040, 0x006C);
  return *p;
}

static double ticks_to_seconds(unsigned long t) {
  return (double)t / 18.20648193;
}

/* ========= LUTs ========= */
#define ANGLE_MAX 1024
#define ANG_MASK (ANGLE_MAX - 1)

static fixed cosLUT[ANGLE_MAX];
static fixed sinLUT[ANGLE_MAX];
static fixed camX[80];

#define RCP_N 2048
static fixed rcpLUT[RCP_N];

static fixed fast_rcp_dir(fixed absDir) {
  long idx;

  if (absDir <= 1) return (fixed)(1000L << FIX_SHIFT);

  idx = (long)(((long long)absDir * (RCP_N - 1)) / (2LL * FIX_ONE));
  if (idx < 0) idx = 0;
  if (idx >= RCP_N) idx = RCP_N - 1;
  return rcpLUT[(int)idx];
}

static void build_luts(void) {
  int i;
  double a;
  fixed t;
  double x;

  for (i = 0; i < ANGLE_MAX; i++) {
    a = (double)i * (2.0 * 3.141592653589793) / (double)ANGLE_MAX;
    cosLUT[i] = (fixed)(cos(a) * (double)FIX_ONE);
    sinLUT[i] = (fixed)(sin(a) * (double)FIX_ONE);
  }

  for (i = 0; i < 80; i++) {
    int sx;
    sx = i * 4;
    /* cameraX = 2*(sx+2)/W - 1 */
    t = (fixed)((2L * (long)(sx + 2)) << FIX_SHIFT);
    camX[i] = (fixed)(t / W) - FIX_ONE;
  }

  for (i = 0; i < RCP_N; i++) {
    x = (2.0 * (double)i) / (double)(RCP_N - 1);
    if (x < 0.0005) rcpLUT[i] = (fixed)(1000.0 * (double)FIX_ONE);
    else            rcpLUT[i] = (fixed)((1.0 / x) * (double)FIX_ONE);
  }
}

/* ========= Video helpers ========= */
static void bios_set_mode(unsigned char mode) {
  union REGS r;
  r.h.ah = 0x00;
  r.h.al = mode;
  int86(0x10, &r, &r);
}

static void wait_vretrace(void) {
  while (inp(0x3DA) & 0x08) { }
  while (!(inp(0x3DA) & 0x08)) { }
}

static int is_wall(int mx, int my) {
  if (mx < 0 || my < 0 || mx >= MW || my >= MH) return 1;
  return world[my][mx] != 0;
}

static unsigned char wall_color(unsigned char tile) {
  switch (tile) {
    case 1: return 200;
    case 2: return 50;
    case 3: return 100;
    default: return 180;
  }
}

static void clear_sky_floor(unsigned char sky, unsigned char floor) {
  int y;
  unsigned int off;
  for (y = 0; y < H/2; y++) {
    off = (unsigned int)(y * W);
    _fmemset((void far*)(backbuf + off), sky, W);
  }
  for (y = H/2; y < H; y++) {
    off = (unsigned int)(y * W);
    _fmemset((void far*)(backbuf + off), floor, W);
  }
}

static void vline4_bb(int x, int yTop, int yBot, unsigned char c) {
  int y;
  unsigned int off;

  if (yTop < 0) yTop = 0;
  if (yBot >= H) yBot = H - 1;
  if (yTop > yBot) return;

  for (y = yTop; y <= yBot; y++) {
    off = (unsigned int)(y * W + x);
    backbuf[off]     = c;
    backbuf[off + 1] = c;
    backbuf[off + 2] = c;
    backbuf[off + 3] = c;
  }
}

static void blit_bb(void) {
  movedata(FP_SEG(backbuf), FP_OFF(backbuf), FP_SEG(vga), FP_OFF(vga), (unsigned)64000);
}

/* ========= Main ========= */
int main(void) {
  /* Declarations first (C89) */
  fixed px, py;
  int ang;
  fixed dirX, dirY;
  fixed planeX, planeY;
  fixed PLANE_SCALE;

  int WALK_TPS, RUN_TPS;
  fixed WALK_PER_TIC, RUN_PER_TIC;
  int TURN_DPS, RTURN_DPS;

  unsigned long lastTick, now;
  double acc, dt;

  /* Setup */
  build_luts();

  px = (fixed)(3.5 * (double)FIX_ONE);
  py = (fixed)(3.5 * (double)FIX_ONE);

  ang = 0;
  dirX = cosLUT[ang];
  dirY = sinLUT[ang];

  PLANE_SCALE = (fixed)(0.66 * (double)FIX_ONE);
  planeX = fmul(-dirY, PLANE_SCALE);
  planeY = fmul( dirX, PLANE_SCALE);

  WALK_TPS = 4;
  RUN_TPS  = 7;
  WALK_PER_TIC = (fixed)(((long long)WALK_TPS * FIX_ONE) / TICRATE);
  RUN_PER_TIC  = (fixed)(((long long)RUN_TPS  * FIX_ONE) / TICRATE);

  TURN_DPS  = 120;
  RTURN_DPS = 180;

  kb_install();
  bios_set_mode(0x13);

  lastTick = bios_ticks();
  acc = 0.0;

  /* Main loop */
  while (1) {
    now = bios_ticks();
    dt = ticks_to_seconds(now - lastTick);
    lastTick = now;

    if (dt > 0.25) dt = 0.25;
    acc += dt;

    if (keyDown[K_ESC]) break;

    /* Fixed 35Hz tics */
    while (acc >= (1.0 / (double)TICRATE)) {
      int forward, back, left, right, run, strafe;
      fixed moveSpd;

      forward = (keyDown[K_UP_E0] || keyDown[K_W]) ? 1 : 0;
      back    = (keyDown[K_DOWN_E0] || keyDown[K_S]) ? 1 : 0;
      left    = (keyDown[K_LEFT_E0] || keyDown[K_A]) ? 1 : 0;
      right   = (keyDown[K_RIGHT_E0] || keyDown[K_D]) ? 1 : 0;
      run     = (keyDown[K_LSHIFT] || keyDown[K_RSHIFT]) ? 1 : 0;
      strafe  = keyDown[K_LALT] ? 1 : 0;

      moveSpd = run ? RUN_PER_TIC : WALK_PER_TIC;

      /* Turn (unless strafing) */
      if (!strafe && (left || right)) {
        int dps, step;

        dps = run ? RTURN_DPS : TURN_DPS;
        step = (int)((((long)dps * ANGLE_MAX) / 360L) / TICRATE);
        if (step < 1) step = 1;

        if (left)  ang = (ang - step) & ANG_MASK;
        else       ang = (ang + step) & ANG_MASK;

        dirX = cosLUT[ang];
        dirY = sinLUT[ang];
        planeX = fmul(-dirY, PLANE_SCALE);
        planeY = fmul( dirX, PLANE_SCALE);
      }

      /* Forward/back */
      if (forward || back) {
        fixed stepFB, nx, ny;

        stepFB = forward ? moveSpd : -moveSpd;
        nx = px + fmul(dirX, stepFB);
        ny = py + fmul(dirY, stepFB);

        if (!is_wall(f_to_int(nx), f_to_int(py))) px = nx;
        if (!is_wall(f_to_int(px), f_to_int(ny))) py = ny;
      }

      /* Strafe with Alt + Left/Right */
      if (strafe && (left || right)) {
        fixed sxv, syv, stepS, nx2, ny2;

        sxv = -dirY;
        syv =  dirX;
        stepS = left ? -moveSpd : moveSpd;

        nx2 = px + fmul(sxv, stepS);
        ny2 = py + fmul(syv, stepS);

        if (!is_wall(f_to_int(nx2), f_to_int(py))) px = nx2;
        if (!is_wall(f_to_int(px),  f_to_int(ny2))) py = ny2;
      }

      acc -= (1.0 / (double)TICRATE);
    }

    /* Render */
    clear_sky_floor(29, 17);

    {
      int r;
      for (r = 0; r < 80; r++) {
        int sx;
        fixed cameraX, rayDirX, rayDirY;
        int mapX, mapY;
        fixed deltaDistX, deltaDistY;
        fixed sideDistX, sideDistY;
        int stepX, stepY, side;

        sx = r * 4;

        cameraX = camX[r];
        rayDirX = dirX + fmul(planeX, cameraX);
        rayDirY = dirY + fmul(planeY, cameraX);

        mapX = f_to_int(px);
        mapY = f_to_int(py);

        deltaDistX = fast_rcp_dir(f_abs(rayDirX));
        deltaDistY = fast_rcp_dir(f_abs(rayDirY));

        if (rayDirX < 0) {
          stepX = -1;
          sideDistX = fmul(px - f_from_int(mapX), deltaDistX);
        } else {
          stepX = 1;
          sideDistX = fmul(f_from_int(mapX + 1) - px, deltaDistX);
        }

        if (rayDirY < 0) {
          stepY = -1;
          sideDistY = fmul(py - f_from_int(mapY), deltaDistY);
        } else {
          stepY = 1;
          sideDistY = fmul(f_from_int(mapY + 1) - py, deltaDistY);
        }

        side = 0;
        while (!is_wall(mapX, mapY)) {
          if (sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
          } else {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
          }
        }

        {
          fixed perpDist;
          int lineHeight;
          int yTop, yBot;
          unsigned char tile, col;

          perpDist = (side == 0) ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);
          if (perpDist < 1) perpDist = 1;

          lineHeight = (int)(((long long)H << FIX_SHIFT) / (long long)perpDist);

          yTop = (H / 2) - (lineHeight / 2);
          yBot = yTop + lineHeight;

          tile = world[mapY][mapX];
          col  = wall_color(tile);
          if (side == 1 && col > 20) col -= 20;

          vline4_bb(sx, yTop, yBot, col);
        }
      }
    }

    wait_vretrace();
    blit_bb();
  }

  bios_set_mode(0x03);
  kb_uninstall();
  return 0;
}