#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine.h"
#include "font/font.h"
#include "arch/idt.h"
#include "arch/pic.h"
#include "mm/pmm.h"
#include "drivers/keyboard.h"

__attribute__((used, section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

static struct limine_framebuffer *fb = NULL;
static size_t cursor_x = 0;
static size_t cursor_y = 0;
static uint32_t fg_color = 0xFFFFFFFF;
static uint32_t bg_color = 0x000F172A;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void serial_init(void) {
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}

static int is_transmit_empty(void) {
    return inb(0x3F8 + 5) & 0x20;
}

static void serial_putchar(char c) {
    while (is_transmit_empty() == 0);
    outb(0x3F8, c);
}

static void serial_print(const char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') serial_putchar('\r');
        serial_putchar(str[i]);
    }
}

static void putpixel(size_t x, size_t y, uint32_t color) {
    if (!fb || x >= fb->width || y >= fb->height) return;
    volatile uint32_t *pixel_addr = (volatile uint32_t *)((uint8_t *)fb->address + y * fb->pitch + x * 4);
    *pixel_addr = color;
}

static void clear_screen(uint32_t color) {
    if (!fb) return;
    bg_color = color;
    for (size_t y = 0; y < fb->height; y++) {
        for (size_t x = 0; x < fb->width; x++) {
            putpixel(x, y, color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

static void draw_char(char c, size_t x, size_t y, uint32_t fg, uint32_t bg) {
    uint8_t uc = (uint8_t)c;
    if (uc > 127) uc = '?';

    for (int cy = 0; cy < 16; cy++) {
        uint8_t row = font8x16[uc][cy];
        for (int cx = 0; cx < 8; cx++) {
            if ((row >> (7 - cx)) & 1) {
                putpixel(x + cx, y + cy, fg);
            } else {
                putpixel(x + cx, y + cy, bg);
            }
        }
    }
}

void kputchar(char c) {
    if (c == '\n') serial_putchar('\r');
    serial_putchar(c);

    if (c == '\n') {
        cursor_x = 0;
        cursor_y += 16;
        return;
    }

    if (cursor_x + 8 > fb->width) {
        cursor_x = 0;
        cursor_y += 16;
    }

    if (cursor_y + 16 > fb->height) {
        cursor_y = 0;
    }

    draw_char(c, cursor_x, cursor_y, fg_color, bg_color);
    cursor_x += 8;
}

void kprint(const char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        kputchar(str[i]);
    }
}

void kprint_hex(uint64_t val) {
    kprint("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t nibble = (val >> i) & 0xF;
        if (nibble < 10) {
            kputchar('0' + nibble);
        } else {
            kputchar('A' + (nibble - 10));
        }
    }
}

void kprint_dec(uint64_t val) {
    if (val == 0) {
        kputchar('0');
        return;
    }
    char buf[32];
    int i = 0;
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (--i >= 0) {
        kputchar(buf[i]);
    }
}

void kmain(void) {
    serial_init();
    serial_print("\n[AetherOS] Serial COM1 Initialized\n");

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        serial_print("[ERROR] Framebuffer not found!\n");
        for (;;) __asm__ volatile ("hlt");
    }

    fb = framebuffer_request.response->framebuffers[0];
    clear_screen(0x000F172A);

    fg_color = 0x0038BDF8;
    kprint("==================================================\n");
    kprint("           AetherOS Kernel x86_64 v0.1            \n");
    kprint("==================================================\n\n");

    fg_color = 0x004ADE80;
    kprint("[OK] Resolution: ");
    kprint_dec(fb->width);
    kprint(" x ");
    kprint_dec(fb->height);
    kprint(" (");
    kprint_dec(fb->bpp);
    kprint(" bpp)\n");

    kprint("[OK] Video Buffer: ");
    kprint_hex((uint64_t)fb->address);
    kprint("\n");

    fg_color = 0x00FACC15;
    kprint("[OK] Serial UART COM1: Active (38400 baud)\n");
    kprint("[OK] Console Font Engine: 8x16 Bitmap Loaded\n");

    idt_init();
    fg_color = 0x0038BDF8;
    kprint("[OK] IDT (Interrupt Descriptor Table): Loaded (256 vectors)\n");

    pmm_init();
    fg_color = 0x00A78BFA;
    kprint("[OK] PMM (Physical Memory Manager): Active (Bitmap Allocator)\n");
    kprint("     Total RAM: ");
    kprint_dec(pmm_get_total_memory() / (1024 * 1024));
    kprint(" MB | Free RAM: ");
    kprint_dec(pmm_get_free_memory() / (1024 * 1024));
    kprint(" MB\n");

    void *test_page1 = pmm_alloc_page();
    void *test_page2 = pmm_alloc_page();
    kprint("     Allocated Page 1: ");
    kprint_hex((uint64_t)test_page1);
    kprint(" | Page 2: ");
    kprint_hex((uint64_t)test_page2);
    kprint("\n\n");

    pic_remap();
    keyboard_init();
    __asm__ volatile ("sti");

    fg_color = 0x00F472B6;
    kprint("[OK] PIC & PS/2 Keyboard Driver: Ready\n\n");

    fg_color = 0xFFFFFFFF;
    kprint("AetherOS Shell (type anything): \n> ");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}