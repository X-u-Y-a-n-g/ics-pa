#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  static uint32_t last_time = 0;
  int key = _read_key();
  if (key != _KEY_NONE) {
    bool down = (key & 0x8000) != 0;
    key &= 0x7fff;
    const char *name = keyname[key];
    int n = snprintf((char *)buf, len, "%s %s\n", down ? "kd" : "ku", name);
    return (n > 0) ? (size_t)n : 0;
  }

  uint32_t now = _uptime();
  if (now - last_time >= 1000 / 30) {
    last_time = now;
    int n = snprintf((char *)buf, len, "t %u\n", now);
    return (n > 0) ? (size_t)n : 0;
  }
  return 0;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  size_t disp_len = strlen(dispinfo);
  if (offset >= (off_t)disp_len) return;
  if (offset + (off_t)len > (off_t)disp_len) {
    len = disp_len - offset;
  }
  memcpy(buf, dispinfo + offset, len);
}

void fb_write(const void *buf, off_t offset, size_t len) {
  int x = (offset / 4) % _screen.width;
  int y = (offset / 4) / _screen.width;
  int w = len / 4;
  _draw_rect((const uint32_t *)buf, x, y, w, 1);
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  snprintf(dispinfo, sizeof(dispinfo), "WIDTH:%d\nHEIGHT:%d\n",
           _screen.width, _screen.height);
}
