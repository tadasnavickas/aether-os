#include "shell.h"
#include "mm/pmm.h"
#include <stddef.h>
#include <stdbool.h>

#define CMD_BUFFER_SIZE 256

static char cmd_buffer[CMD_BUFFER_SIZE];
static size_t cmd_len = 0;

void kprint(const char *str);
void kputchar(char c);
void kprint_dec(uint64_t val);
void kprint_hex(uint64_t val);
void clear_screen(uint32_t color);

static int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(const unsigned char *)a - *(const unsigned char *)b;
}

static int strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i] || a[i] == '\0') {
            return (unsigned char)a[i] - (unsigned char)b[i];
        }
    }
    return 0;
}

void shell_init(void) {
    cmd_len = 0;
    cmd_buffer[0] = '\0';
    kprint("\nAetherOS Shell v0.1\nType 'help' for a list of commands.\n> ");
}

static void shell_execute(void) {
    if (cmd_len == 0) {
        kprint("\n> ");
        return;
    }

    kprint("\n");

    if (strcmp(cmd_buffer, "help") == 0) {
        kprint("Available commands:\n");
        kprint("  help   - Show this help menu\n");
        kprint("  clear  - Clear the screen\n");
        kprint("  mem    - Display RAM statistics\n");
        kprint("  echo   - Print arguments to screen\n");
        kprint("  panic  - Trigger kernel panic exception\n");
    } else if (strcmp(cmd_buffer, "clear") == 0) {
        clear_screen(0x000F172A);
    } else if (strcmp(cmd_buffer, "mem") == 0) {
        kprint("Memory Status:\n");
        kprint("  Total RAM: ");
        kprint_dec(pmm_get_total_memory() / (1024 * 1024));
        kprint(" MB\n  Free RAM:  ");
        kprint_dec(pmm_get_free_memory() / (1024 * 1024));
        kprint(" MB\n");
    } else if (strncmp(cmd_buffer, "echo ", 5) == 0) {
        kprint(cmd_buffer + 5);
        kprint("\n");
    } else if (strcmp(cmd_buffer, "panic") == 0) {
        kprint("Triggering division by zero...\n");
        int a = 1 / 0;
        (void)a;
    } else {
        kprint("Unknown command: ");
        kprint(cmd_buffer);
        kprint("\n");
    }

    cmd_len = 0;
    cmd_buffer[0] = '\0';
    kprint("> ");
}

void shell_handle_key(char c) {
    if (c == '\n') {
        cmd_buffer[cmd_len] = '\0';
        shell_execute();
    } else if (c == '\b') {
        if (cmd_len > 0) {
            cmd_len--;
            cmd_buffer[cmd_len] = '\0';
            kputchar('\b');
        }
    } else {
        if (cmd_len < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_len++] = c;
            kputchar(c);
        }
    }
}