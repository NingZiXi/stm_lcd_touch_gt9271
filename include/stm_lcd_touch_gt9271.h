#ifndef STM_LCD_TOUCH_GT9271_H
#define STM_LCD_TOUCH_GT9271_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define STM_LCD_TOUCH_GT9271_MAX_POINTS 10u
#define STM_LCD_TOUCH_GT9271_I2C_ADDR_A 0x14u
#define STM_LCD_TOUCH_GT9271_I2C_ADDR_B 0x5du
typedef struct { uint16_t x, y; uint8_t id; } stm_lcd_touch_gt9271_point_t;
typedef struct {
    /* Access 16-bit register addresses; complete before returning; 0 = success. */
    int (*read_reg)(void *io, uint16_t reg, uint8_t *data, size_t length);
    int (*write_reg)(void *io, uint16_t reg, const uint8_t *data, size_t length);
    void (*delay_ms)(void *io, uint32_t milliseconds);
    void (*reset)(void *io, int high);
    void *io;
    uint16_t x_max, y_max;
    uint8_t swap_xy, mirror_x, mirror_y;
} stm_lcd_touch_gt9271_config_t;
typedef struct {
    stm_lcd_touch_gt9271_config_t config;
    stm_lcd_touch_gt9271_point_t points[STM_LCD_TOUCH_GT9271_MAX_POINTS];
    uint8_t count, created;
} stm_lcd_touch_gt9271_t;
/* Returns 0 on success, -1 invalid arguments, -2 transport error, -3 bad ID/data. */
int stm_lcd_touch_gt9271_new_i2c(stm_lcd_touch_gt9271_t *touch, const stm_lcd_touch_gt9271_config_t *config);
int stm_lcd_touch_gt9271_reset(stm_lcd_touch_gt9271_t *touch);
int stm_lcd_touch_gt9271_read_id(stm_lcd_touch_gt9271_t *touch, char id[5]);
/* No new controller frame preserves the last touch state; a ready zero-point
 * frame releases it. Transport/malformed-frame failures clear cached points. */
int stm_lcd_touch_gt9271_read_data(stm_lcd_touch_gt9271_t *touch);
int stm_lcd_touch_gt9271_get_data(const stm_lcd_touch_gt9271_t *touch, stm_lcd_touch_gt9271_point_t *points, size_t capacity, size_t *count);
#ifdef __cplusplus
}
#endif
#endif
