#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

static paddr_t page_translate(vaddr_t addr, bool is_write) {
  if (!cpu.cr0.protect_enable || !cpu.cr0.paging) {
    return addr;
  }

  uint32_t dir = (addr >> 22) & 0x3ff;
  uint32_t tab = (addr >> 12) & 0x3ff;
  uint32_t off = addr & PAGE_MASK;

  paddr_t pde_addr = (cpu.cr3.page_directory_base << 12) + dir * sizeof(PDE);
  PDE pde;
  pde.val = paddr_read(pde_addr, 4);
  Assert(pde.present, "page directory entry is not present, vaddr = 0x%08x", addr);
  if (!pde.accessed) {
    pde.accessed = 1;
    paddr_write(pde_addr, 4, pde.val);
  }

  paddr_t pte_addr = (pde.page_frame << 12) + tab * sizeof(PTE);
  PTE pte;
  pte.val = paddr_read(pte_addr, 4);
  Assert(pte.present, "page table entry is not present, vaddr = 0x%08x", addr);
  if (!pte.accessed || (is_write && !pte.dirty)) {
    pte.accessed = 1;
    if (is_write) {
      pte.dirty = 1;
    }
    paddr_write(pte_addr, 4, pte.val);
  }

  return (pte.page_frame << 12) | off;
}

uint32_t paddr_read(paddr_t addr, int len) {
  int map_NO = is_mmio(addr);
  if (map_NO >= 0) {
    return mmio_read(addr, len, map_NO);
  }
  return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int map_NO = is_mmio(addr);
  if (map_NO >= 0) {
    mmio_write(addr, len, data, map_NO);
    return;
  }
  memcpy(guest_to_host(addr), &data, len);
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  // return paddr_read(addr, len);
  assert(len == 1 || len == 2 || len == 4);
  int first_len = PAGE_SIZE - (addr & PAGE_MASK);
  if (first_len < len) {
    uint32_t low = vaddr_read(addr, first_len);
    uint32_t high = vaddr_read(addr + first_len, len - first_len);
    return low | (high << (first_len * 8));
  }
  return paddr_read(page_translate(addr, false), len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  // paddr_write(addr, len, data);
  assert(len == 1 || len == 2 || len == 4);
  int first_len = PAGE_SIZE - (addr & PAGE_MASK);
  if (first_len < len) {
    vaddr_write(addr, first_len, data);
    vaddr_write(addr + first_len, len - first_len, data >> (first_len * 8));
    return;
  }
  paddr_write(page_translate(addr, true), len, data);
}
