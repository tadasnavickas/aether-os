#include "shell.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "drivers/timer.h"
#include "drivers/rtc.h"
#include "fs/vfs.h"
#include "fs/aetherfs.h"
#include "gui/desktop.h"
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

static void print_2digits(uint8_t val) {
    if (val < 10) kputchar('0');
    kprint_dec(val);
}

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

static size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

void shell_init(void) {
    cmd_len = 0;
    cmd_buffer[0] = '\0';
    kprint("\nAetherOS Shell v0.4\nType 'help' for commands.\n> ");
}

static void shell_execute(void) {
    if (cmd_len == 0) {
        kprint("\n> ");
        return;
    }

    kprint("\n");

    if (strcmp(cmd_buffer, "help") == 0) {
        kprint("Commands:\n");
        kprint("  gui          - Launch Graphical Desktop UI\n");
        kprint("  dir          - List files on AetherFS disk\n");
        kprint("  touch <file> - Create an empty file\n");
        kprint("  write <f> <t>- Write text into file\n");
        kprint("  type <file>  - Read file contents\n");
        kprint("  rm <file>    - Delete file from AetherFS\n");
        kprint("  format       - Reformat AetherFS disk\n");
        kprint("  ls           - List files on TarFS (initrd)\n");
        kprint("  cat <file>   - Read file from TarFS\n");
        kprint("  date         - Show RTC date/time\n");
        kprint("  uptime       - Show system uptime\n");
        kprint("  sleep        - Sleep 2 seconds\n");
        kprint("  mem / heap   - Memory status\n");
        kprint("  clear        - Clear screen\n");
    } else if (strcmp(cmd_buffer, "gui") == 0) {
        desktop_render();
        cmd_len = 0;
        cmd_buffer[0] = '\0';
        return;
    } else if (strcmp(cmd_buffer, "clear") == 0) {
        clear_screen(0x000F172A);
    } else if (strcmp(cmd_buffer, "dir") == 0) {
        kprint("AetherFS Native Drive Files:\n");
        aetherfs_list();
    } else if (strncmp(cmd_buffer, "touch ", 6) == 0) {
        char *name = cmd_buffer + 6;
        if (aetherfs_create_file(name) == 0) {
            kprint("File created: ");
            kprint(name);
            kprint("\n");
        } else {
            kprint("Error creating file!\n");
        }
    } else if (strncmp(cmd_buffer, "write ", 6) == 0) {
        char *args = cmd_buffer + 6;
        char filename[32] = {0};
        int i = 0;
        while (args[i] && args[i] != ' ' && i < 31) {
            filename[i] = args[i];
            i++;
        }
        filename[i] = '\0';
        if (args[i] == ' ') {
            char *text = args + i + 1;
            aetherfs_create_file(filename);
            if (aetherfs_write_file(filename, text, strlen(text)) == 0) {
                kprint("Data written to ");
                kprint(filename);
                kprint("\n");
            } else {
                kprint("Error writing to file!\n");
            }
        } else {
            kprint("Usage: write <filename> <text>\n");
        }
    } else if (strncmp(cmd_buffer, "type ", 5) == 0) {
        char *name = cmd_buffer + 5;
        char buf[512];
        if (aetherfs_read_file(name, buf, sizeof(buf)) == 0) {
            kprint(buf);
            kprint("\n");
        } else {
            kprint("File not found on AetherFS: ");
            kprint(name);
            kprint("\n");
        }
    } else if (strncmp(cmd_buffer, "rm ", 3) == 0) {
        char *name = cmd_buffer + 3;
        if (aetherfs_delete_file(name) == 0) {
            kprint("File deleted: ");
            kprint(name);
            kprint("\n");
        } else {
            kprint("File not found: ");
            kprint(name);
            kprint("\n");
        }
    } else if (strcmp(cmd_buffer, "format") == 0) {
        aetherfs_format();
    } else if (strcmp(cmd_buffer, "ls") == 0) {
        kprint("TarFS (Read-only Ramdisk):\n");
        vfs_list_files();
    } else if (strncmp(cmd_buffer, "cat ", 4) == 0) {
        char *filename = cmd_buffer + 4;
        char file_content[512];
        if (vfs_read_file(filename, file_content, sizeof(file_content)) == 0) {
            kprint(file_content);
            kprint("\n");
        } else {
            kprint("File not found on TarFS: ");
            kprint(filename);
            kprint("\n");
        }
    } else if (strcmp(cmd_buffer, "date") == 0) {
        struct rtc_time t;
        rtc_get_time(&t);
        kprint("RTC Time (UTC): ");
        kprint_dec(t.year);
        kputchar('-');
        print_2digits(t.month);
        kputchar('-');
        print_2digits(t.day);
        kprint(" ");
        print_2digits(t.hour);
        kputchar(':');
        print_2digits(t.minute);
        kputchar(':');
        print_2digits(t.second);
        kprint("\n");
    } else if (strcmp(cmd_buffer, "uptime") == 0) {
        kprint("System Uptime: ");
        kprint_dec(timer_get_uptime_seconds());
        kprint(" s\n");
    } else if (strcmp(cmd_buffer, "sleep") == 0) {
        kprint("Sleeping for 2000 ms...\n");
        sleep_ms(2000);
        kprint("Awake!\n");
    } else if (strcmp(cmd_buffer, "mem") == 0) {
        kprint("Physical RAM: ");
        kprint_dec(pmm_get_free_memory() / (1024 * 1024));
        kprint(" MB free\n");
    } else if (strcmp(cmd_buffer, "heap") == 0) {
        kprint("Kernel Heap: ");
        kprint_dec(heap_get_used());
        kprint(" Bytes used\n");
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