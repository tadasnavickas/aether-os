#include "keyboard.h"
#include "../arch/pic.h"
#include "../arch/idt.h"
#include "../shell.h"
#include <stdint.h>
#include <stdbool.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static bool shift_pressed = false;

static const char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',   0, ' '
};

static const char kbd_us_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0,
  '*',   0, ' '
};

__attribute__((interrupt))
void keyboard_interrupt_handler(struct interrupt_frame *frame) {
    (void)frame;
    uint8_t scancode = inb(0x60);

    pic_send_eoi(1);

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = false;
        return;
    }

    if (!(scancode & 0x80)) {
        char c = shift_pressed ? kbd_us_shift[scancode] : kbd_us[scancode];
        if (c != 0) {
            __asm__ volatile ("sti");
            shell_handle_key(c);
        }
    }
}

void keyboard_init(void) {
    idt_set_descriptor(33, (void *)keyboard_interrupt_handler, 0x8E);
    pic_unmask_irq(1);
}