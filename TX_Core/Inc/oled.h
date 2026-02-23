#ifndef INC_OLED_H_
#define INC_OLED_H_

#include <stdint.h>

int  oled_probe(uint8_t *found_addr7);     // 0x3C/0x3D 탐색
void oled_set_addr(uint8_t addr7);

void oled_init(void);
void oled_inverse(uint8_t on);

void oled_clear(void);
void oled_clear_pages(uint8_t p0, uint8_t p1);

void oled_set_cursor(uint8_t x, uint8_t y);
void oled_write_centered(const char *s, uint8_t y);
void oled_write_centered_2x(const char *s, uint8_t y);

#endif
