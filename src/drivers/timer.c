#include "timer.h"
#include "../arch/pic.h"
#include "../arch/idt.h"

#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND_PORT  0x43
#define PIT_BASE_FREQUENCY 1193182

static volatile uint64_t ticks = 0;
static uint32_t current_freq = 1000;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

__attribute__((interrupt))
void timer_interrupt_handler(struct interrupt_frame *frame) {
    (void)frame;
    ticks++;
    pic_send_eoi(0);
}

void timer_init(uint32_t frequency) {
    current_freq = frequency;
    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;

    outb(PIT_COMMAND_PORT, 0x36);
    outb(PIT_CHANNEL0_DATA, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0_DATA, (uint8_t)((divisor >> 8) & 0xFF));

    idt_set_descriptor(32, (void *)timer_interrupt_handler, 0x8E);
    pic_unmask_irq(0);
}

uint64_t timer_get_ticks(void) {
    return ticks;
}

uint64_t timer_get_uptime_seconds(void) {
    return ticks / current_freq;
}

void sleep_ms(uint64_t milliseconds) {
    uint64_t target_ticks = ticks + milliseconds;
    while (ticks < target_ticks) {
        __asm__ volatile ("sti; hlt");
    }
}