#include "oled.h"
#include "i2c.h"
#include "stm32f1xx.h"
#include <stdint.h>

extern void Delay_ms(uint32_t ms);

static uint8_t g_oled_addr = 0x3C;

// ----------- low-level SSD1306 -----------
static int ssd1306_cmd(uint8_t c)
{
    uint8_t pkt[2] = {0x00, c};
    return i2c1_write(g_oled_addr, pkt, 2);
}

static int ssd1306_data(const uint8_t *p, uint16_t n)
{
    uint8_t buf[17];
    buf[0] = 0x40;

    while (n) {
        uint16_t chunk = (n > 16) ? 16 : n;
        for (uint16_t i = 0; i < chunk; i++) buf[1+i] = p[i];

        int ret = i2c1_write(g_oled_addr, buf, 1 + chunk);
        if (ret < 0) return ret;

        p += chunk;
        n -= chunk;
    }
    return 0;
}

void oled_set_addr(uint8_t addr7)
{
    g_oled_addr = addr7;
}

int oled_probe(uint8_t *found_addr7)
{
    if (i2c1_ping(0x3C) == 0) { *found_addr7 = 0x3C; return 0; }
    if (i2c1_ping(0x3D) == 0) { *found_addr7 = 0x3D; return 0; }
    return -1;
}

void oled_set_cursor(uint8_t x, uint8_t y)
{
    uint8_t page = (uint8_t)(y / 8);
    ssd1306_cmd(0xB0 | (page & 0x07));
    ssd1306_cmd(0x00 | (x & 0x0F));
    ssd1306_cmd(0x10 | ((x >> 4) & 0x0F));
}

void oled_clear_pages(uint8_t p0, uint8_t p1)
{
    uint8_t z[16] = {0};

    for (uint8_t p = p0; p <= p1; p++) {
        ssd1306_cmd(0xB0 | p);
        ssd1306_cmd(0x00);
        ssd1306_cmd(0x10);
        for (uint8_t i = 0; i < 8; i++) (void)ssd1306_data(z, 16); // 128 bytes
    }
}

void oled_clear(void)
{
    oled_clear_pages(0, 7);
}

void oled_inverse(uint8_t on)
{
    ssd1306_cmd(on ? 0xA7 : 0xA6);
}

// ----------- tiny 5x7 font (필요 문자만) -----------
static void font5x7_get(char c, uint8_t col[5])
{
    for (int i = 0; i < 5; i++) col[i] = 0x00;

    switch (c) {
        case 'A': col[0]=0x7E; col[1]=0x11; col[2]=0x11; col[3]=0x11; col[4]=0x7E; break;
        case 'U': col[0]=0x3F; col[1]=0x40; col[2]=0x40; col[3]=0x40; col[4]=0x3F; break;
        case 'T': col[0]=0x01; col[1]=0x01; col[2]=0x7F; col[3]=0x01; col[4]=0x01; break;
        case 'O': col[0]=0x3E; col[1]=0x41; col[2]=0x41; col[3]=0x41; col[4]=0x3E; break;
        case 'M': col[0]=0x7F; col[1]=0x02; col[2]=0x0C; col[3]=0x02; col[4]=0x7F; break;
        case 'N': col[0]=0x7F; col[1]=0x04; col[2]=0x08; col[3]=0x10; col[4]=0x7F; break;
        case 'L': col[0]=0x7F; col[1]=0x40; col[2]=0x40; col[3]=0x40; col[4]=0x40; break;
        case 'E': col[0]=0x7F; col[1]=0x49; col[2]=0x49; col[3]=0x49; col[4]=0x41; break;
        case 'S': col[0]=0x46; col[1]=0x49; col[2]=0x49; col[3]=0x49; col[4]=0x31; break;
        case 'P': col[0]=0x7F; col[1]=0x09; col[2]=0x09; col[3]=0x09; col[4]=0x06; break;
        case ' ': default: break;
    }
}

static void ssd1306_write_char(char c)
{
    uint8_t g[5];
    font5x7_get(c, g);

    uint8_t out[6] = { g[0], g[1], g[2], g[3], g[4], 0x00 };
    (void)ssd1306_data(out, 6);
}

static void ssd1306_write_string(const char *s)
{
    while (*s) ssd1306_write_char(*s++);
}

static uint8_t str_len(const char *s)
{
    uint8_t n = 0;
    while (*s++) n++;
    return n;
}

void oled_write_centered(const char *s, uint8_t y)
{
    uint8_t len = str_len(s);
    uint8_t w = (uint8_t)(len * 6);
    uint8_t x = (w >= 128) ? 0 : (uint8_t)((128 - w) / 2);

    oled_set_cursor(x, y);
    ssd1306_write_string(s);
}

// ----------- 2배 확대 출력 -----------
void oled_write_centered_2x(const char *s, uint8_t y)
{
    // y는 0,8,16,... 권장 (2페이지 사용)
    uint8_t len = str_len(s);
    uint8_t char_w = 12;            // (5col*2) + (space 2col)
    uint16_t w = (uint16_t)len * char_w;
    uint8_t x = (w >= 128) ? 0 : (uint8_t)((128 - w) / 2);

    uint8_t top[128];
    uint8_t bot[128];
    uint16_t idx = 0;

    while (*s && (idx + char_w) <= 128) {
        uint8_t g[5];
        font5x7_get(*s++, g);

        for (int c = 0; c < 5; c++) {
            // 7bit -> 14bit(세로 2배)
            uint16_t col14 = 0;
            for (int b = 0; b < 7; b++) {
                if (g[c] & (1U << b)) {
                    col14 |= (1U << (2*b));
                    col14 |= (1U << (2*b + 1));
                }
            }

            uint8_t lo = (uint8_t)(col14 & 0xFF);
            uint8_t hi = (uint8_t)((col14 >> 8) & 0xFF);

            // 가로 2배
            top[idx] = lo; bot[idx] = hi; idx++;
            top[idx] = lo; bot[idx] = hi; idx++;
        }

        // 글자 사이 공백 2컬럼
        top[idx] = 0; bot[idx] = 0; idx++;
        top[idx] = 0; bot[idx] = 0; idx++;
    }

    oled_set_cursor(x, y);
    (void)ssd1306_data(top, (uint16_t)idx);

    oled_set_cursor(x, (uint8_t)(y + 8));
    (void)ssd1306_data(bot, (uint16_t)idx);
}

void oled_init(void)
{
    Delay_ms(50);

    ssd1306_cmd(0xAE);
    ssd1306_cmd(0xD5); ssd1306_cmd(0x80);
    ssd1306_cmd(0xA8); ssd1306_cmd(0x3F);
    ssd1306_cmd(0xD3); ssd1306_cmd(0x00);
    ssd1306_cmd(0x40);
    ssd1306_cmd(0x8D); ssd1306_cmd(0x14);
    ssd1306_cmd(0x20); ssd1306_cmd(0x02); // page addressing
    ssd1306_cmd(0xA1);
    ssd1306_cmd(0xC8);
    ssd1306_cmd(0xDA); ssd1306_cmd(0x12);
    ssd1306_cmd(0x81); ssd1306_cmd(0xCF);
    ssd1306_cmd(0xD9); ssd1306_cmd(0xF1);
    ssd1306_cmd(0xDB); ssd1306_cmd(0x40);
    ssd1306_cmd(0xA4);
    ssd1306_cmd(0xA6);
    ssd1306_cmd(0xAF);

    oled_clear();
}
