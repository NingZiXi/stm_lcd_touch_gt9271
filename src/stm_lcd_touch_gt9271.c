/** @file stm_lcd_touch_gt9271.c @brief 触点解码、错误传递和实例管理。 */
#include "stm_lcd_touch_gt9271.h"
#include <stdlib.h>
#include <string.h>
struct lcd_touch_gt9271_context {
    lcd_touch_gt9271_config_t config;
    lcd_touch_gt9271_point_t points[LCD_TOUCH_GT9271_MAX_POINTS];
    uint8_t count;
};
stm_err_t lcd_touch_gt9271_create(const lcd_touch_gt9271_config_t *config, lcd_touch_gt9271_handle_t *out)
{
    if (!config || !out) return STM_ERR_INVALID_ARG;
    if (*out) return STM_ERR_INVALID_STATE;
    if (!config->read_reg || !config->write_reg || !config->x_max || !config->y_max ||
        config->swap_xy > 1u || config->mirror_x > 1u || config->mirror_y > 1u ||
        (config->reset && !config->delay_ms)) return STM_ERR_INVALID_CONFIG;
    lcd_touch_gt9271_handle_t touch = calloc(1, sizeof(*touch));
    if (!touch) return STM_ERR_NO_MEM;
    touch->config = *config;
    *out = touch;
    return STM_OK;
}
stm_err_t lcd_touch_gt9271_delete(lcd_touch_gt9271_handle_t *handle)
{
    if (!handle) return STM_ERR_INVALID_ARG;
    free(*handle);
    *handle = NULL;
    return STM_OK;
}
stm_err_t lcd_touch_gt9271_reset(lcd_touch_gt9271_handle_t touch)
{
    if (!touch) return STM_ERR_INVALID_ARG;
    touch->count = 0;
    if (!touch->config.reset) return STM_OK;
    stm_err_t err = touch->config.reset(touch->config.io, 0);
    if (err != STM_OK) return err;
    touch->config.delay_ms(touch->config.io, 30);
    err = touch->config.reset(touch->config.io, 1);
    if (err != STM_OK) return err;
    touch->config.delay_ms(touch->config.io, 50);
    return STM_OK;
}
#define GT_ID 0x8140u
#define GT_STATUS 0x814eu
#define GT_POINTS 0x814fu
stm_err_t lcd_touch_gt9271_read_id(lcd_touch_gt9271_handle_t touch, char id[5])
{
    if (id) memset(id, 0, 5);
    if (!touch || !id) return STM_ERR_INVALID_ARG;
    char local[5] = {0};
    stm_err_t err = touch->config.read_reg(touch->config.io, GT_ID, (uint8_t *)local, 4);
    if (err != STM_OK) return err;
    memcpy(id, local, 5);
    return memcmp(id, "9271", 4) == 0 ? STM_OK : STM_ERR_NOT_SUPPORTED;
}
stm_err_t lcd_touch_gt9271_read_data(lcd_touch_gt9271_handle_t touch)
{
    if (!touch) return STM_ERR_INVALID_ARG;
    uint8_t status, raw[LCD_TOUCH_GT9271_MAX_POINTS * 8u], ack = 0;
    stm_err_t err = touch->config.read_reg(touch->config.io, GT_STATUS, &status, 1);
    if (err != STM_OK) { touch->count = 0; return err; }
    // 未就绪没有新状态，不能将持续拖动误报成松开。
    if (!(status & 0x80u)) return STM_OK;
    touch->count = 0;
    uint8_t n = status & 0x0fu;
    if (n > LCD_TOUCH_GT9271_MAX_POINTS) err = STM_ERR_VERIFY;
    else if (n) err = touch->config.read_reg(touch->config.io, GT_POINTS, raw, (size_t)n * 8u);
    stm_err_t ack_err = touch->config.write_reg(touch->config.io, GT_STATUS, &ack, 1);
    if (err != STM_OK) return err;
    if (ack_err != STM_OK) return ack_err;
    for (uint8_t i = 0; i < n; ++i) {
        const uint8_t *p = raw + (size_t)i * 8u;
        uint16_t x = (uint16_t)(p[1] | ((uint16_t)p[2] << 8));
        uint16_t y = (uint16_t)(p[3] | ((uint16_t)p[4] << 8));
        if (touch->config.swap_xy) { uint16_t t = x; x = y; y = t; }
        if (x >= touch->config.x_max || y >= touch->config.y_max) continue;
        if (touch->config.mirror_x) x = (uint16_t)(touch->config.x_max - 1u - x);
        if (touch->config.mirror_y) y = (uint16_t)(touch->config.y_max - 1u - y);
        touch->points[touch->count].x = x;
        touch->points[touch->count].y = y;
        touch->points[touch->count].id = p[0] & 0x0fu;
        ++touch->count;
    }
    return STM_OK;
}
stm_err_t lcd_touch_gt9271_get_data(lcd_touch_gt9271_handle_t touch, lcd_touch_gt9271_point_t *points, size_t capacity, size_t *count)
{
    if (count) *count = 0;
    if (!touch || !count || (capacity && !points)) return STM_ERR_INVALID_ARG;
    *count = touch->count < capacity ? touch->count : capacity;
    if (*count) memcpy(points, touch->points, *count * sizeof(*points));
    return STM_OK;
}
