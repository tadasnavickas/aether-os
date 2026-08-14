#include "keyboard.h"
#include "../arch/pic.h"
#include "../arch/idt.h"
#include <stdint.h>

void kputchar(char c);

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static const char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',   0, ' '
};

__attribute__((interrupt))
void keyboard_interrupt_handler(struct interrupt_frame *frame) {
    (void)frame;
    uint8_t scancode = inb(0x60);

    if (!(scancode & 0x80)) {
        char c = kbd_us[scancode];
        if (c != 0) {
            kputchar(c);
        }
    }

    pic_send_eoi(1);
}

void keyboard_init(void) {
    idt_set_descriptor(33, (void *)keyboard_interrupt_handler, 0x8E);
    pic_unmask_irq(1);
}