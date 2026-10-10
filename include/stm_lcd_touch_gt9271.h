/**
 * @file stm_lcd_touch_gt9271.h
 * @brief GT9271 单点快照通用触摸接口。
 */
#ifndef STM_LCD_TOUCH_GT9271_H
#define STM_LCD_TOUCH_GT9271_H
#include "stm_lcd.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define LCD_TOUCH_GT9271_MAX_POINTS 10u
#define LCD_TOUCH_GT9271_I2C_ADDR_A 0x14u
#define LCD_TOUCH_GT9271_I2C_ADDR_B 0x5du

typedef struct
{
    stm_lcd_io_handle_t io;             // 借用的原子寄存器 IO。
    stm_lcd_touch_config_t coordinates; // 框架统一坐标变换，仅执行一次。
    void (*delay_ms)(void *, uint32_t); // 有复位回调时必需。
    stm_err_t (*reset)(void *, int);    // 可选 RST 电平控制。
    void *control_context;              // 借用的控制回调上下文。
} lcd_touch_gt9271_config_t;

/**
 * @brief 创建通用单点触摸句柄，不访问硬件。
 * @param config 配置副本，IO 与上下文由应用持有
 * @param out 初始为空的输出句柄地址
 * @return STM_OK 或参数、配置、分配及借用错误；失败不修改已有句柄。
 */
stm_err_t lcd_touch_gt9271_create(const lcd_touch_gt9271_config_t *config,
                                  stm_lcd_touch_handle_t *out);
/**
 * @brief 读取 GT9271 产品标识，不改变触摸快照。
 * @param touch 本组件创建的句柄，禁止用其他型号句柄
 * @param id 五字节输出，失败通信时清零，ID 不匹配仍保留读取结果
 * @return STM_OK、NOT_SUPPORTED、参数、状态或通信错误。
 */
stm_err_t lcd_touch_gt9271_read_id(stm_lcd_touch_handle_t touch, char id[5]);
#ifdef __cplusplus
}
#endif
#endif
