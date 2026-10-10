/**
 * @file example.c
 * @brief HAL 原子寄存器 IO 及通用触摸接入。
 */
#include "example.h"
#include <limits.h>

static stm_err_t hal_result(HAL_StatusTypeDef status)
{
    if (status == HAL_OK)
    {
        return STM_OK;
    }
    return status == HAL_TIMEOUT ? STM_ERR_TIMEOUT : STM_ERR_IO;
}

static stm_err_t read_reg(void *io, uint16_t reg, uint8_t address_bytes, uint8_t *data, size_t len)
{
    lcd_touch_gt9271_example_board_t *board = io;
    if (address_bytes != 2 || !data || !len || len > UINT16_MAX)
    {
        return STM_ERR_INVALID_ARG;
    }
    return hal_result(HAL_I2C_Mem_Read(board->i2c, (uint16_t)(board->address_7bit << 1), reg,
                                       I2C_MEMADD_SIZE_16BIT, data, (uint16_t)len, 100u));
}

static stm_err_t
write_reg(void *io, uint16_t reg, uint8_t address_bytes, const uint8_t *data, size_t len)
{
    lcd_touch_gt9271_example_board_t *board = io;
    if (address_bytes != 2 || !data || !len || len > UINT16_MAX)
    {
        return STM_ERR_INVALID_ARG;
    }
    return hal_result(HAL_I2C_Mem_Write(board->i2c, (uint16_t)(board->address_7bit << 1), reg,
                                        I2C_MEMADD_SIZE_16BIT, (uint8_t *)data, (uint16_t)len,
                                        100u));
}

static void delay_ms(void *io, uint32_t ms)
{
    (void)io;
    HAL_Delay(ms);
}

static stm_err_t reset(void *io, int high)
{
    lcd_touch_gt9271_example_board_t *board = io;
    HAL_GPIO_WritePin(board->rst_port, board->rst_pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return STM_OK;
}

static const stm_lcd_io_ops_t io_ops = {.read_reg = read_reg, .write_reg = write_reg};

stm_err_t lcd_touch_gt9271_example_start(stm_lcd_touch_handle_t *touch,
                                         lcd_touch_gt9271_example_board_t *board)
{
    if (!touch || !board)
    {
        return STM_ERR_INVALID_ARG;
    }
    if (*touch || board->io.ops)
    {
        return STM_ERR_INVALID_STATE;
    }
    if (!board->i2c || !board->address_7bit || board->address_7bit > 0x7f || !board->width ||
        !board->height)
    {
        return STM_ERR_INVALID_CONFIG;
    }
    const lcd_touch_gt9271_config_t cfg = {
        .io = &board->io,
        .control_context = board,
        .delay_ms = delay_ms,
        .reset = board->rst_port ? reset : NULL,
        .coordinates = {.x_max = board->width,
                        .y_max = board->height,
                        .swap_xy = board->swap_xy,
                        .mirror_x = board->mirror_x,
                        .mirror_y = board->mirror_y},
    };
    stm_err_t err = stm_lcd_io_init(&board->io, &io_ops, board);
    if (err == STM_OK)
    {
        err = lcd_touch_gt9271_create(&cfg, touch);
    }
    if (err == STM_OK && board->rst_port)
    {
        err = stm_lcd_touch_reset(*touch);
    }
    char id[5];
    if (err == STM_OK)
    {
        err = lcd_touch_gt9271_read_id(*touch, id);
    }
    if (err != STM_OK)
    {
        (void)stm_lcd_touch_delete(touch);
        if (board->io.ops)
        {
            (void)stm_lcd_io_deinit(&board->io);
        }
    }
    return err;
}

stm_err_t lcd_touch_gt9271_example_stop(stm_lcd_touch_handle_t *touch,
                                        lcd_touch_gt9271_example_board_t *board)
{
    if (!touch || !board)
    {
        return STM_ERR_INVALID_ARG;
    }
    stm_err_t err = stm_lcd_touch_delete(touch);
    if (err == STM_OK && board->io.ops)
    {
        err = stm_lcd_io_deinit(&board->io);
    }
    return err;
}

stm_err_t lcd_touch_gt9271_example_poll(stm_lcd_touch_handle_t touch,
                                        stm_lcd_touch_point_t *points,
                                        size_t capacity,
                                        size_t *count)
{
    if (count)
    {
        *count = 0;
    }
    if (!count || (capacity && !points))
    {
        return STM_ERR_INVALID_ARG;
    }
    stm_err_t err = stm_lcd_touch_read_data(touch);
    if (err == STM_OK)
    {
        err = stm_lcd_touch_get_data(touch, points, capacity, count);
    }
    return err;
}
