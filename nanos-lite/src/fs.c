#include "fs.h"

void ramdisk_read(void *buf, off_t offset, size_t len);
void ramdisk_write(const void *buf, off_t offset, size_t len);
size_t events_read(void *buf, size_t len);
void dispinfo_read(void *buf, off_t offset, size_t len);
size_t dispinfo_size(void);
void fb_write(const void *buf, off_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 0, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // TODO: initialize the size of /dev/fb
  file_table[FD_FB].size = _screen.width * _screen.height * sizeof(uint32_t);
  file_table[FD_DISPINFO].size = dispinfo_size();
}

int fs_open(const char *pathname, int flags, int mode) {
  (void)flags;
  (void)mode;
  for (int i = 0; i < (int)NR_FILES; i++) {
    if (strcmp(file_table[i].name, pathname) == 0) {
      file_table[i].open_offset = 0;
      return i;
    }
  }
  assert(0);
  return -1;
}

size_t fs_filesz(int fd) {
  assert(fd >= 0 && fd < (int)NR_FILES);
  return file_table[fd].size;
}

ssize_t fs_read(int fd, void *buf, size_t len) {
  assert(fd >= 0 && fd < (int)NR_FILES);
  Finfo *f = &file_table[fd];
  if (fd == FD_STDIN || fd == FD_STDOUT || fd == FD_STDERR) {
    return 0;
  }
  if (fd == FD_EVENTS) {
    return events_read(buf, len);
  }
  if (fd == FD_DISPINFO) {
    if (f->open_offset >= (off_t)f->size) return 0;
    size_t n = len;
    if (f->open_offset + (off_t)n > (off_t)f->size) {
      n = f->size - f->open_offset;
    }
    dispinfo_read(buf, f->open_offset, n);
    f->open_offset += n;
    return n;
  }
  if (f->open_offset >= (off_t)f->size) return 0;
  size_t n = len;
  if (f->open_offset + (off_t)n > (off_t)f->size) {
    n = f->size - f->open_offset;
  }
  ramdisk_read(buf, f->disk_offset + f->open_offset, n);
  f->open_offset += n;
  return n;
}

ssize_t fs_write(int fd, const void *buf, size_t len) {
  assert(fd >= 0 && fd < (int)NR_FILES);
  Finfo *f = &file_table[fd];
  if (fd == FD_STDOUT || fd == FD_STDERR) {
    const char *cbuf = (const char *)buf;
    for (size_t i = 0; i < len; i++) {
      _putc(cbuf[i]);
    }
    return len;
  }
  if (fd == FD_STDIN || fd == FD_EVENTS || fd == FD_DISPINFO) {
    return 0;
  }
  if (fd == FD_FB) {
    if (f->open_offset >= (off_t)f->size) return 0;
    size_t n = len;
    if (f->open_offset + (off_t)n > (off_t)f->size) {
      n = f->size - f->open_offset;
    }
    fb_write(buf, f->open_offset, n);
    f->open_offset += n;
    return n;
  }
  if (f->open_offset >= (off_t)f->size) return 0;
  size_t n = len;
  if (f->open_offset + (off_t)n > (off_t)f->size) {
    n = f->size - f->open_offset;
  }
  ramdisk_write(buf, f->disk_offset + f->open_offset, n);
  f->open_offset += n;
  return n;
}

off_t fs_lseek(int fd, off_t offset, int whence) {
  assert(fd >= 0 && fd < (int)NR_FILES);
  Finfo *f = &file_table[fd];
  off_t new_off = f->open_offset;
  switch (whence) {
    case SEEK_SET: new_off = offset; break;
    case SEEK_CUR: new_off = f->open_offset + offset; break;
    case SEEK_END: new_off = (off_t)f->size + offset; break;
    default: assert(0);
  }
  if (new_off < 0) new_off = 0;
  if (new_off > (off_t)f->size) new_off = (off_t)f->size;
  f->open_offset = new_off;
  return f->open_offset;
}

int fs_close(int fd) {
  assert(fd >= 0 && fd < (int)NR_FILES);
  return 0;
}
