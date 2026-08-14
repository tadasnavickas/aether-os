#include "rtc.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static int get_update_in_progress_flag(void) {
    outb(CMOS_ADDRESS, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

static uint8_t get_rtc_register(int reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

static uint8_t bcd_to_bin(uint8_t val) {
    return ((val / 16) * 10) + (val & 0x0F);
}

void rtc_get_time(struct rtc_time *time) {
    while (get_update_in_progress_flag());

    uint8_t second = get_rtc_register(0x00);
    uint8_t minute = get_rtc_register(0x02);
    uint8_t hour   = get_rtc_register(0x04);
    uint8_t day    = get_rtc_register(0x07);
    uint8_t month  = get_rtc_register(0x08);
    uint8_t year   = get_rtc_register(0x09);
    uint8_t registerB = get_rtc_register(0x0B);

    if (!(registerB & 0x04)) {
        second = bcd_to_bin(second);
        minute = bcd_to_bin(minute);
        hour = ((hour & 0x7F) != hour) ? (bcd_to_bin(hour & 0x7F) | 0x80) : bcd_to_bin(hour);
        day = bcd_to_bin(day);
        month = bcd_to_bin(month);
        year = bcd_to_bin(year);
    }

    if (!(registerB & 0x02) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }

    time->second = second;
    time->minute = minute;
    time->hour = hour;
    time->day = day;
    time->month = month;
    time->year = 2000 + year;
}