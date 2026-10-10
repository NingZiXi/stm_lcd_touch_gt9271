/**
 * @file stm_lcd_touch_gt9271.c
 * @brief 保留器件协议并输出单个原始触点。
 */
#include "stm_lcd_touch_gt9271.h"
#include "stm_lcd_impl.h"
#ifndef STM_LCD_FRAMEBUFFER_API
#error                                                                                             \
    "This unpublished migration requires the matching local stm_lcd source with framebuffer/register capabilities"
#endif
#include <stdlib.h>
#include <string.h>

struct lcd_touch_gt9271_context
{
    struct stm_lcd_touch base;        // 首成员，框架拥有变换后快照。
    lcd_touch_gt9271_config_t config; // 借用外部资源。
    stm_lcd_touch_point_t previous;   // GT 无新帧时保留原始点，避免重复变换。
    size_t previous_count;            // 原始快照有效数量。
};

static stm_err_t touch_reset(stm_lcd_touch_handle_t handle)
{
    struct lcd_touch_gt9271_context *touch = (struct lcd_touch_gt9271_context *)handle;
    touch->previous_count = 0;
    if (!touch->config.reset)
    {
        return STM_ERR_NOT_SUPPORTED;
    }
    stm_err_t err = touch->config.reset(touch->config.control_context, 0);
    if (err != STM_OK)
    {
        return err;
    }
    touch->config.delay_ms(touch->config.control_context, 30);
    err = touch->config.reset(touch->config.control_context, 1);
    if (err != STM_OK)
    {
        return err;
    }
    touch->config.delay_ms(touch->config.control_context, 50);
    return STM_OK;
}

static stm_err_t
touch_read(stm_lcd_touch_handle_t handle, stm_lcd_touch_point_t *point, size_t *count)
{
    struct lcd_touch_gt9271_context *touch = (struct lcd_touch_gt9271_context *)handle;
    uint8_t status = 0;
    uint8_t raw[LCD_TOUCH_GT9271_MAX_POINTS * 8u] = {0};
    *count = 0;
    stm_err_t err = stm_lcd_io_read_reg(handle->io, 0x814e, 2, &status, 1);
    if (err != STM_OK)
    {
        touch->previous_count = 0;
        return err;
    }
    if (!(status & 0x80u))
    {
        *point = touch->previous;
        *count = touch->previous_count;
        return STM_OK;
    }
    touch->previous_count = 0;
    uint8_t n = status & 0x0fu;
    if (n > LCD_TOUCH_GT9271_MAX_POINTS)
    {
        err = STM_ERR_VERIFY;
    }
    else if (n)
    {
        err = stm_lcd_io_read_reg(handle->io, 0x814f, 2, raw, (size_t)n * 8u);
    }
    uint16_t ids = 0;
    for (uint8_t i = 0; err == STM_OK && i < n; ++i)
    {
        const uint8_t *p = raw + (size_t)i * 8u;
        uint8_t id = p[0] & 0x0fu;
        uint16_t x = (uint16_t)(p[1] | ((uint16_t)p[2] << 8));
        uint16_t y = (uint16_t)(p[3] | ((uint16_t)p[4] << 8));
        if (ids & (1u << id))
        {
            err = STM_ERR_VERIFY;
            break;
        }
        ids |= (uint16_t)(1u << id);
        uint16_t logical_x = touch->config.coordinates.swap_xy ? y : x;
        uint16_t logical_y = touch->config.coordinates.swap_xy ? x : y;
        if (logical_x >= touch->config.coordinates.x_max ||
            logical_y >= touch->config.coordinates.y_max)
        {
            continue;
        }
        if (!*count)
        {
            point->x = x;
            point->y = y;
            point->id = id;
            *count = 1;
        }
    }
    uint8_t ack = 0;
    stm_err_t ack_err = stm_lcd_io_write_reg(handle->io, 0x814e, 2, &ack, 1);
    if (err == STM_OK)
    {
        err = ack_err;
    }
    if (err != STM_OK)
    {
        *count = 0;
        return err;
    }
    touch->previous = *point;
    touch->previous_count = *count;
    return STM_OK;
}

static void touch_destroy(stm_lcd_touch_handle_t handle)
{
    free(handle);
}

static const stm_lcd_touch_ops_t touch_ops = {
    .reset = touch_reset, .read_data = touch_read, .destroy = touch_destroy};

stm_err_t lcd_touch_gt9271_read_id(stm_lcd_touch_handle_t touch, char id[5])
{
    if (id)
    {
        memset(id, 0, 5);
    }
    if (!touch || !id || touch->ops != &touch_ops)
    {
        return STM_ERR_INVALID_ARG;
    }
    if (touch->accessing || touch->owner)
    {
        return STM_ERR_INVALID_STATE;
    }
    char local[5] = {0};
    touch->accessing = 1;
    stm_err_t err = stm_lcd_io_read_reg(touch->io, 0x8140, 2, (uint8_t *)local, 4);
    touch->accessing = 0;
    if (err != STM_OK)
    {
        return err;
    }
    memcpy(id, local, 5);
    return memcmp(id, "9271", 4) == 0 ? STM_OK : STM_ERR_NOT_SUPPORTED;
}

stm_err_t lcd_touch_gt9271_create(const lcd_touch_gt9271_config_t *config,
                                  stm_lcd_touch_handle_t *out)
{
    if (!config || !out)
    {
        return STM_ERR_INVALID_ARG;
    }
    if (*out)
    {
        return STM_ERR_INVALID_STATE;
    }
    if (!config->io || !config->io->ops || !config->io->ops->read_reg ||
        !config->io->ops->write_reg || !config->coordinates.x_max || !config->coordinates.y_max ||
        config->coordinates.swap_xy > 1 || config->coordinates.mirror_x > 1 ||
        config->coordinates.mirror_y > 1 || (config->reset && !config->delay_ms))
    {
        return STM_ERR_INVALID_CONFIG;
    }
    struct lcd_touch_gt9271_context *touch = calloc(1, sizeof(*touch));
    if (!touch)
    {
        return STM_ERR_NO_MEM;
    }
    touch->config = *config;
    stm_err_t err =
        stm_lcd_touch_base_init(&touch->base, &touch_ops, config->io, &config->coordinates);
    if (err != STM_OK)
    {
        free(touch);
        return err;
    }
    *out = &touch->base;
    return STM_OK;
}
