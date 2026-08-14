#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include <stddef.h>
#include "../limine.h"

void gfx_init(struct limine_framebuffer *framebuffer);
void gfx_put_pixel(size_t x, size_t y, uint32_t color);
void gfx_fill_rect(size_t x, size_t y, size_t width, size_t height, uint32_t color);
void gfx_draw_rect(size_t x, size_t y, size_t width, size_t height, uint32_t color);
void gfx_draw_char(char c, size_t x, size_t y, uint32_t fg, uint32_t bg);
void gfx_draw_string(const char *str, size_t x, size_t y, uint32_t fg, uint32_t bg);
void gfx_draw_window(size_t x, size_t y, size_t width, size_t height, const char *title);

size_t gfx_get_width(void);
size_t gfx_get_height(void);

#endif