#include <am.h>
#include <x86.h>

#define RTC_PORT 0x48   // Note that this is not standard
static unsigned long boot_time;

void _ioe_init() {
  boot_time = inl(RTC_PORT);
}

unsigned long _uptime() {
  // return 0;
  return inl(RTC_PORT) - boot_time;
}

uint32_t* const fb = (uint32_t *)0x40000;

_Screen _screen = {
  .width  = 400,
  .height = 300,
};

extern void* memcpy(void *, const void *, int);

void _draw_rect(const uint32_t *pixels, int x, int y, int w, int h) {
  // int i;
  // for (i = 0; i < _screen.width * _screen.height; i++) {
  //   fb[i] = i;
  // }
  if (w <= 0 || h <= 0) return;
  if (x < 0) { pixels -= x; w += x; x = 0; }
  if (y < 0) { pixels -= y * w; h += y; y = 0; }
  if (x + w > _screen.width)  w = _screen.width - x;
  if (y + h > _screen.height) h = _screen.height - y;
  if (w <= 0 || h <= 0) return;

  int row;
  for (row = 0; row < h; row++) {
    uint32_t *dst = fb + (y + row) * _screen.width + x;
    const uint32_t *src = pixels + row * w;
    int col;
    for (col = 0; col < w; col++) {
      dst[col] = src[col] | 0xff000000u;
    }
  }
}

void _draw_sync() {
}

int _read_key() {
  // return _KEY_NONE;
  uint8_t status = inb(0x64);
  if ((status & 0x1) == 0) return _KEY_NONE;
  return inl(0x60);
}
