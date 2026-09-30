# STM32 HAL 接入示例

配置 GPIO 或 HAL I²C，依据电路在 RST 释放时设置 INT 电平，确定 7 位 I²C 地址。`read_reg` 和 `write_reg` 接收 16 位寄存器地址，返回 0 为成功。每次调用应在返回前同步完成。

```c
static int read_reg(void *io, uint16_t reg, uint8_t *dst, size_t n);
static int write_reg(void *io, uint16_t reg, const uint8_t *src, size_t n);
stm_lcd_touch_gt9271_t touch = {0};
stm_lcd_touch_gt9271_config_t cfg = {
    .read_reg = read_reg, .write_reg = write_reg,
    .x_max = 800, .y_max = 1280, .mirror_x = 0, .mirror_y = 0,
};
char id[5];
if (stm_lcd_touch_gt9271_new_i2c(&touch, &cfg) == 0 &&
    stm_lcd_touch_gt9271_read_id(&touch, id) == 0) {
    stm_lcd_touch_gt9271_point_t points[10];
    size_t count;
    if (stm_lcd_touch_gt9271_read_data(&touch) == 0)
        stm_lcd_touch_gt9271_get_data(&touch, points, 10, &count);
}
```

可构建的 GPIO、I²C 与 LVGL 实际板级示例位于 STM32H757 主工程 `examples/display_lvgl_demo.c`。

`example.c` 提供 HAL I²C 寄存器回调；先在板级完成 INT/RST 地址选择，再调用 `example_gt9271_start()`。

轮询时，`get_data()` 返回当前状态：未就绪帧保持上一触点，明确的就绪零点帧才释放。不要把“未准备好新帧”解释为松手；此区别对 LVGL 连续拖动和滑动手势十分重要。I²C 或非法帧错误会清空触点，调用者应按释放状态处理并记录错误。
