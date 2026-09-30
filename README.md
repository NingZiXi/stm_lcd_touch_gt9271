# GT9271 I²C 触摸组件

支持 ID 读取、0–10 点坐标读取、状态寄存器应答及方向变换；用户提供 16 位寄存器读写回调。7 位 I²C 地址通常为 `0x5D` 或 `0x14`，HAL I²C API 使用左移后的地址。板级管理复位和 INT 上电时序；镜像和 XY 交换由 `stm_lcd_touch_gt9271_config_t` 控制。`read_data()` 成功后通过 `get_data()` 读取当前触摸状态。控制器没有新帧（0x814E 的 ready 位为 0）时保持上一状态；只有 ready 位为 1 且触点数为 0 才表示松手。即使轮询周期短于触摸报告周期，连续按住/拖动也不会因两帧间隙而被误判为释放。读失败时清空缓存，避免旧触点误报。`-1` 无效参数，`-2` I²C 失败，`-3` ID 或触点数异常。

```cmake
add_subdirectory(lib/stm_lcd_touch_gt9271)
target_link_libraries(your_app PRIVATE stm_lcd_touch_gt9271)
```

见 [examples/stm32_hal](examples/stm32_hal/) 板级示例；STM32H757 配套 800×1280 触摸实测方向为 `swap_xy=0, mirror_x=0, mirror_y=0`，其他安装方向必须重新测量。运行 `cmake -S tests -B build/tests && cmake --build build/tests && ctest --test-dir build/tests --output-on-failure`。MIT 许可证见 LICENSE。
