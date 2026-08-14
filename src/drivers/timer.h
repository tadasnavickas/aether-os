#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_init(uint32_t frequency);
uint64_t timer_get_ticks(void);
uint64_t timer_get_uptime_seconds(void);
void sleep_ms(uint64_t milliseconds);

#endif