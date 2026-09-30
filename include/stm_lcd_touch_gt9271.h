/** @file stm_lcd_touch_gt9271.h @brief 芯片独立接口，统一错误码与不透明句柄。 */
#ifndef STM_LCD_TOUCH_GT9271_H
#define STM_LCD_TOUCH_GT9271_H
#include <stddef.h>
#include <stdint.h>
#include "stm_err.h"

#ifdef __cplusplus
extern "C" {
#endif
#define LCD_TOUCH_GT9271_MAX_POINTS 10u
#define LCD_TOUCH_GT9271_I2C_ADDR_A 0x14u
#define LCD_TOUCH_GT9271_I2C_ADDR_B 0x5du
typedef struct lcd_touch_gt9271_context *lcd_touch_gt9271_handle_t;
typedef struct { uint16_t x, y; uint8_t id; } lcd_touch_gt9271_point_t;
typedef struct {
    stm_err_t (*read_reg)(void *io, uint16_t reg, uint8_t *data, size_t length); /**< 同步寄存器读取，长度为字节。 */
    stm_err_t (*write_reg)(void *io, uint16_t reg, const uint8_t *data, size_t length); /**< 同步写入，用于确认数据帧。 */
    void (*delay_ms)(void *io, uint32_t milliseconds); /**< 有 reset 时必需。 */
    stm_err_t (*reset)(void *io, int high); /**< 可选 RST 电平回调。 */
    void *io; /**< 借用，允许 NULL；I2C 7 位地址在板级上下文中保存。 */
    uint16_t x_max, y_max; /**< 变换后逻辑尺寸，上限不包含；swap_xy 后检查范围。 */
    uint8_t swap_xy, mirror_x, mirror_y; /**< 只允许 0/1。 */
} lcd_touch_gt9271_config_t;
/**
 * @brief 创建控制对象，不初始化总线或接管板级资源。
 * @param config 配置被复制，io/回调上下文与外部缓冲必须在实例存续期有效。
 * @param out 输出句柄地址；*out 必须为 NULL，非空返回 INVALID_STATE 并保持原值。
 * @return STM_OK、INVALID_ARG、INVALID_CONFIG 或 NO_MEM；失败不改变 *out。
 * @note 芯片 create 不访问硬件。使用 calloc/free，仅允许应用串行线程调用。
 */
stm_err_t lcd_touch_gt9271_create(const lcd_touch_gt9271_config_t *config, lcd_touch_gt9271_handle_t *out);
/** @brief 仅释放拥有的控制对象；成功清空 *handle，空句柄也成功。
 * @note 调用前停止并发访问；其他别名不自动清空，删除后禁止使用。
 */
stm_err_t lcd_touch_gt9271_delete(lcd_touch_gt9271_handle_t *handle);
/** @brief 清空触点缓存并执行可选复位，失败可重试，不改变总线配置。 */
stm_err_t lcd_touch_gt9271_reset(lcd_touch_gt9271_handle_t touch);
/** @brief 读取并缓存触点；通信/畸形帧失败清空缓存，错误原样返回。
 * @note 未就绪帧保持旧状态，有效零点帧释放；就绪帧即使解析失败仍尝试 ACK，保留首个错误。
 */
stm_err_t lcd_touch_gt9271_read_data(lcd_touch_gt9271_handle_t touch);
/** @brief 复制 min(缓存点数,capacity) 个点到调用者数组，*count 是实际复制数量。
 * @note capacity=0 允许 points=NULL；失败清零有效的 count，不修改 points。
 */
stm_err_t lcd_touch_gt9271_get_data(lcd_touch_gt9271_handle_t touch, lcd_touch_gt9271_point_t *points, size_t capacity, size_t *count);
/** @brief 读 4 字节产品 ID 并补 NUL；不匹配返回 NOT_SUPPORTED，保留实际 ID。
 * @note id 必须至少 5 字节；通信失败时输出清空。
 */
stm_err_t lcd_touch_gt9271_read_id(lcd_touch_gt9271_handle_t touch, char id[5]);

#ifdef __cplusplus
}
#endif
#endif
