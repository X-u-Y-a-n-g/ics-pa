#include "common.h"
#include "fs.h"

#define DEFAULT_ENTRY ((void *)0x4000000)

// void ramdisk_read(void *buf, off_t offset, size_t len);
// size_t get_ramdisk_size();

uintptr_t loader(_Protect *as, const char *filename) {
  // TODO();
  // size_t size = get_ramdisk_size();
  // ramdisk_read(DEFAULT_ENTRY, 0, size);
  // return (uintptr_t)DEFAULT_ENTRY;
  (void)as;
  const char *fname = (filename == NULL ? "/bin/bmptest" : filename);
  int fd = fs_open(fname, 0, 0);
  size_t size = fs_filesz(fd);
  fs_read(fd, DEFAULT_ENTRY, size);
  fs_close(fd);
  return (uintptr_t)DEFAULT_ENTRY;
}
