#include "gfx.h"
#include "../font/font.h"

static struct limine_framebuffer *fb = NULL;

void gfx_init(struct limine_framebuffer *framebuffer) {
    fb = framebuffer;
}

size_t gfx_get_width(void) {
    return fb ? fb->width : 0;
}

size_t gfx_get_height(void) {
    return fb ? fb->height : 0;
}

void gfx_put_pixel(size_t x, size_t y, uint32_t color) {
    if (!fb || x >= fb->width || y >= fb->height) return;
    volatile uint32_t *dest = (volatile uint32_t *)((uint8_t *)fb->address + y * fb->pitch + x * 4);
    *dest = color;
}

void gfx_fill_rect(size_t x, size_t y, size_t width, size_t height, uint32_t color) {
    if (!fb) return;
    for (size_t dy = 0; dy < height; dy++) {
        for (size_t dx = 0; dx < width; dx++) {
            gfx_put_pixel(x + dx, y + dy, color);
        }
    }
}

void gfx_draw_rect(size_t x, size_t y, size_t width, size_t height, uint32_t color) {
    for (size_t dx = 0; dx < width; dx++) {
        gfx_put_pixel(x + dx, y, color);
        gfx_put_pixel(x + dx, y + height - 1, color);
    }
    for (size_t dy = 0; dy < height; dy++) {
        gfx_put_pixel(x, y + dy, color);
        gfx_put_pixel(x + width - 1, y + dy, color);
    }
}

void gfx_draw_char(char c, size_t x, size_t y, uint32_t fg, uint32_t bg) {
    if ((unsigned char)c >= 128) return;
    const uint8_t *glyph = font8x16[(unsigned char)c];

    for (int cy = 0; cy < 16; cy++) {
        uint8_t row = glyph[cy];
        for (int cx = 0; cx < 8; cx++) {
            if ((row >> (7 - cx)) & 1) {
                gfx_put_pixel(x + cx, y + cy, fg);
            } else if (bg != 0xFF000000) {
                gfx_put_pixel(x + cx, y + cy, bg);
            }
        }
    }
}

void gfx_draw_string(const char *str, size_t x, size_t y, uint32_t fg, uint32_t bg) {
    size_t cur_x = x;
    while (*str) {
        gfx_draw_char(*str, cur_x, y, fg, bg);
        cur_x += 8;
        str++;
    }
}

void gfx_draw_window(size_t x, size_t y, size_t width, size_t height, const char *title) {
    gfx_fill_rect(x + 6, y + 6, width, height, 0x00050B14);

    gfx_fill_rect(x, y, width, height, 0x000F172A);

    gfx_fill_rect(x, y, width, 28, 0x001E293B);
    gfx_draw_rect(x, y, width, height, 0x00334155);

    gfx_fill_rect(x + 10, y + 8, 12, 12, 0x00EF4444); // Red
    gfx_fill_rect(x + 28, y + 8, 12, 12, 0x00F59E0B); // Yellow
    gfx_fill_rect(x + 46, y + 8, 12, 12, 0x0010B981); // Green

    gfx_draw_string(title, x + 70, y + 6, 0x00F8FAFC, 0x001E293B);
}