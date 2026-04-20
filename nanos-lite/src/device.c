#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  static uint32_t last_time = 0;
  static char evbuf[64];
  static size_t evlen = 0;
  static size_t evoff = 0;
  if (len == 0) return 0;

  // If there is no pending event text, try to generate one first.
  if (evoff == evlen) {
    evlen = 0;
    evoff = 0;

    int key = _read_key();
    if (key != _KEY_NONE) {
      bool down = (key & 0x8000) != 0;
      key &= 0x7fff;
      const char *name = (key >= 0 && key < 256 && keyname[key] != NULL) ? keyname[key] : "NONE";
      snprintf(evbuf, sizeof(evbuf), "%s %s\n", down ? "kd" : "ku", name);
      evlen = strlen(evbuf);
    } else {
      uint32_t now = _uptime();
      if (now - last_time >= 1000 / 30) {
        last_time = now;
        snprintf(evbuf, sizeof(evbuf), "t %u\n", now);
        evlen = strlen(evbuf);
      }
    }
  }

  if (evoff == evlen) {
    return 0;
  }

  size_t n = len;
  size_t remain = evlen - evoff;
  if (n > remain) n = remain;
  memcpy(buf, evbuf + evoff, n);
  evoff += n;
  if (evoff == evlen) {
    evoff = 0;
    evlen = 0;
  }
  return n;
}

static char dispinfo[128] __attribute__((used));

size_t dispinfo_size() {
  return strlen(dispinfo);
}

void dispinfo_read(void *buf, off_t offset, size_t len) {
  size_t disp_len = strlen(dispinfo);
  if (offset >= (off_t)disp_len) return;
  if (offset + (off_t)len > (off_t)disp_len) {
    len = disp_len - offset;
  }
  memcpy(buf, dispinfo + offset, len);
}

void fb_write(const void *buf, off_t offset, size_t len) {
  size_t start_pixel = offset / sizeof(uint32_t);
  size_t pixels = len / sizeof(uint32_t);
  const uint32_t *src = (const uint32_t *)buf;
  size_t fb_pixels = (size_t)_screen.width * _screen.height;

  while (pixels > 0 && start_pixel < fb_pixels) {
    int x = start_pixel % _screen.width;
    int y = start_pixel / _screen.width;
    size_t row_pixels = _screen.width - x;
    if (row_pixels > pixels) row_pixels = pixels;
    _draw_rect(src, x, y, row_pixels, 1);

    src += row_pixels;
    start_pixel += row_pixels;
    pixels -= row_pixels;
  }
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  snprintf(dispinfo, sizeof(dispinfo), "WIDTH:%d\nHEIGHT:%d\n",
           _screen.width, _screen.height);
}
