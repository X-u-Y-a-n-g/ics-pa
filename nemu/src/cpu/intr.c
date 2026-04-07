#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  // TODO();
  rtlreg_t esp = reg_l(R_ESP);
  esp -= 4; vaddr_write(esp, 4, cpu.eflags);
  esp -= 4; vaddr_write(esp, 4, cpu.cs);
  esp -= 4; vaddr_write(esp, 4, ret_addr);
  reg_l(R_ESP) = esp;

  vaddr_t gate_addr = cpu.idtr.base + NO * 8;
  uint32_t low = vaddr_read(gate_addr, 4);
  uint32_t high = vaddr_read(gate_addr + 4, 4);
  assert((high >> 15) & 0x1);
  cpu.eip = (high & 0xFFFF0000) | (low & 0xFFFF);
}

void dev_raise_intr() {
}
