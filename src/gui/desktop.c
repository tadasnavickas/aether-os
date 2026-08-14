#include "desktop.h"
#include "gfx.h"
#include "../drivers/rtc.h"
#include "../mm/pmm.h"

static void format_time_str(char *buf) {
    struct rtc_time t;
    rtc_get_time(&t);

    buf[0] = '0' + (t.hour / 10);
    buf[1] = '0' + (t.hour % 10);
    buf[2] = ':';
    buf[3] = '0' + (t.minute / 10);
    buf[4] = '0' + (t.minute % 10);
    buf[5] = ':';
    buf[6] = '0' + (t.second / 10);
    buf[7] = '0' + (t.second % 10);
    buf[8] = '\0';
}

void desktop_render(void) {
    size_t screen_w = gfx_get_width();
    size_t screen_h = gfx_get_height();

    gfx_fill_rect(0, 0, screen_w, screen_h, 0x000A0F1D);

    gfx_fill_rect(0, 0, screen_w, 32, 0x000F172A);
    gfx_draw_rect(0, 0, screen_w, 32, 0x001E293B);

    gfx_draw_string("AetherOS Desktop", 16, 8, 0x0038BDF8, 0x000F172A);
    gfx_draw_string("Files", 180, 8, 0x0094A3B8, 0x000F172A);
    gfx_draw_string("Terminal", 240, 8, 0x0094A3B8, 0x000F172A);
    gfx_draw_string("System", 325, 8, 0x0094A3B8, 0x000F172A);

    char time_str[16];
    format_time_str(time_str);
    gfx_draw_string(time_str, screen_w - 90, 8, 0x00F8FAFC, 0x000F172A);

    gfx_draw_window(40, 60, 320, 220, "System Info");
    gfx_draw_string("OS: AetherOS x86_64", 56, 105, 0x0038BDF8, 0x000F172A);
    gfx_draw_string("Arch: 64-bit Kernel", 56, 130, 0x0094A3B8, 0x000F172A);
    gfx_draw_string("FS: Native AetherFS", 56, 155, 0x00F43F5E, 0x000F172A);
    gfx_draw_string("Status: Running smoothly", 56, 180, 0x0010B981, 0x000F172A);
    gfx_draw_string("GUI Engine: v0.1 GFX", 56, 205, 0x00FACC15, 0x000F172A);

    gfx_draw_window(400, 60, 480, 320, "Interactive Shell");
    gfx_draw_string("AetherOS Shell v0.3", 416, 105, 0x004ADE80, 0x000F172A);
    gfx_draw_string("Type 'help' for command list.", 416, 130, 0x0094A3B8, 0x000F172A);
    gfx_draw_string("> dir", 416, 160, 0x00F8FAFC, 0x000F172A);
    gfx_draw_string("  readme.txt  [50 bytes]", 416, 185, 0x0038BDF8, 0x000F172A);
    gfx_draw_string("> _", 416, 215, 0x0038BDF8, 0x000F172A);

    size_t dock_w = 260;
    size_t dock_x = (screen_w - dock_w) / 2;
    size_t dock_y = screen_h - 50;

    gfx_fill_rect(dock_x, dock_y, dock_w, 40, 0x001E293B);
    gfx_draw_rect(dock_x, dock_y, dock_w, 40, 0x00334155);

    // Icons in Dock
    gfx_fill_rect(dock_x + 15, dock_y + 8, 24, 24, 0x0038BDF8); // Terminal
    gfx_fill_rect(dock_x + 65, dock_y + 8, 24, 24, 0x00F43F5E); // Files
    gfx_fill_rect(dock_x + 115, dock_y + 8, 24, 24, 0x0010B981); // Settings
    gfx_fill_rect(dock_x + 165, dock_y + 8, 24, 24, 0x00F59E0B); // Browser
    gfx_fill_rect(dock_x + 215, dock_y + 8, 24, 24, 0x00A855F7); // Editor
}