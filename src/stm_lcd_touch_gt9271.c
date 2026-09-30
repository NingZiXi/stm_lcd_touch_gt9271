#include "stm_lcd_touch_gt9271.h"
#include <string.h>
#define GT_ID 0x8140u
#define GT_STATUS 0x814eu
#define GT_POINTS 0x814fu
int stm_lcd_touch_gt9271_new_i2c(stm_lcd_touch_gt9271_t *touch, const stm_lcd_touch_gt9271_config_t *cfg) {
    if (!touch || !cfg || !cfg->read_reg || !cfg->write_reg || !cfg->x_max || !cfg->y_max) return -1;
    memset(touch, 0, sizeof(*touch)); touch->config = *cfg; touch->created = 1; return 0;
}
int stm_lcd_touch_gt9271_reset(stm_lcd_touch_gt9271_t *touch) {
    if (!touch || !touch->created) return -1;
    touch->count = 0;
    if (!touch->config.reset) return 0;
    if (!touch->config.delay_ms) return -1;
    touch->config.reset(touch->config.io, 0); touch->config.delay_ms(touch->config.io, 30);
    touch->config.reset(touch->config.io, 1); touch->config.delay_ms(touch->config.io, 50);
    return 0;
}
int stm_lcd_touch_gt9271_read_id(stm_lcd_touch_gt9271_t *touch, char id[5]) {
    if (!touch || !touch->created || !id) return -1;
    memset(id, 0, 5);
    if (touch->config.read_reg(touch->config.io, GT_ID, (uint8_t *)id, 4)) return -2;
    id[4] = '\0';
    return memcmp(id, "9271", 4) == 0 ? 0 : -3;
}
int stm_lcd_touch_gt9271_read_data(stm_lcd_touch_gt9271_t *touch) {
    uint8_t status, raw[STM_LCD_TOUCH_GT9271_MAX_POINTS * 8u], ack = 0;
    uint8_t n, i; int result = 0;
    if (!touch || !touch->created) return -1;
    if (touch->config.read_reg(touch->config.io, GT_STATUS, &status, 1)) {
        touch->count = 0;
        return -2;
    }
    /* Not-ready means no update, not finger-up. Hold the last state between
     * controller reports; only a ready zero-point frame indicates release. */
    if (!(status & 0x80u)) return 0;
    touch->count = 0;
    n = status & 0x0fu;
    if (n > STM_LCD_TOUCH_GT9271_MAX_POINTS) result = -3;
    else if (n && touch->config.read_reg(touch->config.io, GT_POINTS, raw, (size_t)n * 8u)) result = -2;
    /* Acknowledge even malformed frames so the controller can publish new ones. */
    if (touch->config.write_reg(touch->config.io, GT_STATUS, &ack, 1)) return -2;
    if (result) return result;
    for (i = 0; i < n; ++i) {
        const uint8_t *p = raw + (size_t)i * 8u;
        uint16_t x = (uint16_t)(p[1] | ((uint16_t)p[2] << 8));
        uint16_t y = (uint16_t)(p[3] | ((uint16_t)p[4] << 8));
        uint16_t t;
        if (touch->config.swap_xy) { t = x; x = y; y = t; }
        if (x >= touch->config.x_max || y >= touch->config.y_max) continue;
        if (touch->config.mirror_x) x = (uint16_t)(touch->config.x_max - 1u - x);
        if (touch->config.mirror_y) y = (uint16_t)(touch->config.y_max - 1u - y);
        touch->points[touch->count].x = x; touch->points[touch->count].y = y;
        touch->points[touch->count].id = p[0] & 0x0fu; ++touch->count;
    }
    return 0;
}
int stm_lcd_touch_gt9271_get_data(const stm_lcd_touch_gt9271_t *touch, stm_lcd_touch_gt9271_point_t *points, size_t capacity, size_t *count) {
    if (!touch || !touch->created || !count || (capacity && !points)) return -1;
    *count = touch->count < capacity ? touch->count : capacity;
    if (*count) memcpy(points, touch->points, *count * sizeof(*points));
    return 0;
}
