/**
 * @file example.h
 * @brief HAL I2C 触摸接入，INT/RST 地址选择由板级完成。
 */
#ifndef STM_LCD_TOUCH_GT9271_EXAMPLE_H
#define STM_LCD_TOUCH_GT9271_EXAMPLE_H
#include "stm32h7xx_hal.h"
#include "stm_lcd_touch_gt9271.h"
#include "stm_lcd_impl.h"

typedef struct
{
    struct stm_lcd_io io;                // 板级持有，首次使用必须清零。
    I2C_HandleTypeDef *i2c;              // 借用的已初始化 HAL。
    uint8_t address_7bit;                // 7 位地址，HAL 调用时只左移一次。
    GPIO_TypeDef *rst_port;              // 可选复位口。
    uint16_t rst_pin;                    // 复位引脚。
    uint16_t width, height;              // 变换后的逻辑边界。
    uint8_t swap_xy, mirror_x, mirror_y; // 布尔变换，交由通用框架执行。
} lcd_touch_gt9271_example_board_t;

/**
 * @brief 建立板级 IO 并创建、可选复位触摸设备。
 * @param device 初始为空的通用句柄地址
 * @param board 已清零 IO 且 HAL 已初始化的板级配置，须持续有效
 * @return STM_OK；失败回收本次创建资源，底层错误原样传递。
 */
stm_err_t lcd_touch_gt9271_example_start(stm_lcd_touch_handle_t *device,
                                         lcd_touch_gt9271_example_board_t *board);
/**
 * @brief 删除设备后清除板级 IO，不释放 HAL 或帧缓冲。
 * @param device 已先删除 port 的句柄地址，成功清空
 * @param board 创建时使用的板级配置
 * @return STM_OK；在途、被借用或停止失败时保留资源供重试。
 */
stm_err_t lcd_touch_gt9271_example_stop(stm_lcd_touch_handle_t *device,
                                        lcd_touch_gt9271_example_board_t *board);
/**
 * @brief 独立驱动验证用轮询，接入 port 后由 port 采样。
 * @param touch 通用触摸句柄
 * @param points 逻辑触点输出，容量为零可为空
 * @param capacity 输出容量，框架最多一个触点
 * @param count 输出数量，失败清零
 * @return STM_OK 或设备读取错误。
 */
stm_err_t lcd_touch_gt9271_example_poll(stm_lcd_touch_handle_t touch,
                                        stm_lcd_touch_point_t *points,
                                        size_t capacity,
                                        size_t *count);
#endif
