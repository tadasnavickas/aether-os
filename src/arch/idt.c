#include "idt.h"

void kprint(const char *str);
void kprint_dec(uint64_t val);
void kprint_hex(uint64_t val);

__attribute__((aligned(0x10)))
static struct idt_entry idt[256];
static struct idtr idtr_descriptor;

static const char *exception_names[] = {
    "Division By Zero (#DE)",
    "Debug (#DB)",
    "Non-Maskable Interrupt (#NMI)",
    "Breakpoint (#BP)",
    "Overflow (#OF)",
    "Bound Range Exceeded (#BR)",
    "Invalid Opcode (#UD)",
    "Device Not Available (#NM)",
    "Double Fault (#DF)",
    "Coprocessor Segment Overrun",
    "Invalid TSS (#TS)",
    "Segment Not Present (#NP)",
    "Stack-Segment Fault (#SS)",
    "General Protection Fault (#GP)",
    "Page Fault (#PF)",
    "Reserved",
    "x87 Floating-Point Exception (#MF)",
    "Alignment Check (#AC)",
    "Machine Check (#MC)",
    "SIMD Floating-Point Exception (#XM)",
    "Virtualization Exception (#VE)",
    "Control Protection Exception (#CP)"
};

__attribute__((interrupt))
void default_exception_handler(struct interrupt_frame *frame) {
    kprint("\n\n==================================================\n");
    kprint("             KERNEL PANIC: EXCEPTION              \n");
    kprint("==================================================\n");
    kprint("Fault Reason:              ");
    kprint(exception_names[0]);
    kprint("\nInstruction Pointer (RIP): ");
    kprint_hex(frame->rip);
    kprint("\nCode Segment (CS):         ");
    kprint_hex(frame->cs);
    kprint("\nCPU Flags (RFLAGS):        ");
    kprint_hex(frame->rflags);
    kprint("\nStack Pointer (RSP):       ");
    kprint_hex(frame->rsp);
    kprint("\n\nSystem halted to prevent data corruption.\n");

    for (;;) {
        asm volatile ("hlt");
    }
}

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
    struct idt_entry *entry = &idt[vector];
    uint64_t addr = (uint64_t)isr;

    entry->isr_low    = (uint16_t)addr;
    entry->kernel_cs  = 0x28;
    entry->ist        = 0;
    entry->attributes = flags;
    entry->isr_mid    = (uint16_t)(addr >> 16);
    entry->isr_high   = (uint32_t)(addr >> 32);
    entry->reserved   = 0;
}

void idt_init(void) {
    idtr_descriptor.base = (uint64_t)&idt[0];
    idtr_descriptor.limit = (uint16_t)(sizeof(struct idt_entry) * 256 - 1);

    for (uint8_t vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, (void *)default_exception_handler, 0x8E);
    }

    asm volatile ("lidt %0" : : "m"(idtr_descriptor));
}