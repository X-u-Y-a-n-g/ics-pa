#include "common.h"
#include "fs.h"
#include "memory.h"

// #define DEFAULT_ENTRY ((void *)0x4000000)
#define DEFAULT_ENTRY ((void *)0x8048000)

// void ramdisk_read(void *buf, off_t offset, size_t len);
// size_t get_ramdisk_size();
uintptr_t loader_brk = 0;

uintptr_t loader(_Protect *as, const char *filename) {
  // TODO();
  // size_t size = get_ramdisk_size();
  // ramdisk_read(DEFAULT_ENTRY, 0, size);
  // return (uintptr_t)DEFAULT_ENTRY;
  // (void)as;
  const char *fname = (filename == NULL ? "/bin/pal" : filename);
  int fd = fs_open(fname, 0, 0);
  size_t size = fs_filesz(fd);
  // fs_read(fd, DEFAULT_ENTRY, size);
  
  if (as == NULL) {
    fs_read(fd, DEFAULT_ENTRY, size);
  }
  else {
    uintptr_t va = (uintptr_t)DEFAULT_ENTRY;
    size_t off = 0;
    while (off < size) {
      void *pa = new_page();
      memset(pa, 0, PGSIZE);
      _map(as, (void *)(va + off), pa);

      size_t len = size - off;
      if (len > PGSIZE) {
        len = PGSIZE;
      }
      fs_read(fd, pa, len);
      off += len;
    }
  }

  loader_brk = PGROUNDUP((uintptr_t)DEFAULT_ENTRY + size);
  fs_close(fd);
  return (uintptr_t)DEFAULT_ENTRY;
}
