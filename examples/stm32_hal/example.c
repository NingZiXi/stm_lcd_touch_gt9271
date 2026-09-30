/* Copy into a HAL application after I2C and board-specific INT/RST startup. */
#include "stm32h7xx_hal.h"
#include "stm_lcd_touch_gt9271.h"
typedef struct {
    I2C_HandleTypeDef *i2c;
    uint8_t address_7bit;
} touch_example_io_t;
static int read_reg(void *context, uint16_t reg, uint8_t *dst, size_t len) {
    touch_example_io_t *io = context;
    if (!io || !dst || len > UINT16_MAX) return -1;
    return HAL_I2C_Mem_Read(io->i2c, io->address_7bit << 1, reg,
                            I2C_MEMADD_SIZE_16BIT, dst, (uint16_t)len, 100) == HAL_OK ? 0 : -1;
}
static int write_reg(void *context, uint16_t reg, const uint8_t *src, size_t len) {
    touch_example_io_t *io = context;
    if (!io || !src || len > UINT16_MAX) return -1;
    return HAL_I2C_Mem_Write(io->i2c, io->address_7bit << 1, reg,
                             I2C_MEMADD_SIZE_16BIT, (uint8_t *)src, (uint16_t)len, 100) == HAL_OK ? 0 : -1;
}
int example_gt9271_start(stm_lcd_touch_gt9271_t *touch, touch_example_io_t *io,
                         uint16_t width, uint16_t height) {
    stm_lcd_touch_gt9271_config_t cfg = {
        .io=io, .read_reg=read_reg, .write_reg=write_reg, .x_max=width, .y_max=height,
    };
    char id[5];
    if (!touch || !io || !io->i2c || !width || !height) return -1;
    if (stm_lcd_touch_gt9271_new_i2c(touch, &cfg)) return -1;
    return stm_lcd_touch_gt9271_read_id(touch, id);
}
