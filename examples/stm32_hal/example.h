/** @file example.h @brief HAL I2C 触摸接入，先完成板级 INT/RST 地址选择。 */
#ifndef STM_LCD_TOUCH_GT9271_EXAMPLE_H
#define STM_LCD_TOUCH_GT9271_EXAMPLE_H
#include "stm32h7xx_hal.h"
#include "stm_lcd_touch_gt9271.h"
typedef struct {
    I2C_HandleTypeDef *i2c;
    uint8_t address_7bit;
    GPIO_TypeDef *rst_port;
    uint16_t rst_pin,width,height;
    uint8_t swap_xy,mirror_x,mirror_y;
} lcd_touch_gt9271_example_board_t;
stm_err_t lcd_touch_gt9271_example_start(lcd_touch_gt9271_handle_t *touch,lcd_touch_gt9271_example_board_t *board);
stm_err_t lcd_touch_gt9271_example_poll(lcd_touch_gt9271_handle_t touch,lcd_touch_gt9271_point_t *points,size_t capacity,size_t *count);
#endif
