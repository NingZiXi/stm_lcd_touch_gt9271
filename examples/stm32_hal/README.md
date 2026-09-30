# lcd_touch_gt9271：STM32 HAL 接入示例

本目录提供可复制到已有 HAL 应用的 `example.c` / `example.h`，不是完整 CubeMX 工程。先初始化板级时钟、GPIO 和总线，再传入实际 HAL 句柄/引脚。代码使用 STM32H7 HAL，其他系列自行替换头文件；示例不会自动编入组件库。

当前示例对应未发布的新 API；软件验证通过后仍需按实物回归。GT9271 的 v0.1.0 已在 H757 配套模组验证坐标、空白区域滑动及控件拖动；实测方向全部为 0。当前句柄迁移版本尚未实板回归。

## 接入步骤

1. 将组件和 stm_common 加入 CMake；LVGL port 先提供 LVGL 9 target。
2. 将本目录两个源码文件复制到应用，替换 HAL 头文件和实际板级参数。
3. 以 NULL 初始化句柄，按 example.h 的 start 接口创建；板级结构体必须持久有效。
4. 循环绘图/读取触点或调用 LVGL handler，检查每一步 `err != STM_OK`。
5. 停止所有访问后调用 `lcd_touch_gt9271_delete(&handle)`。

```cmake
target_sources(your_firmware PRIVATE App/example.c)
target_include_directories(your_firmware PRIVATE App)
target_link_libraries(your_firmware PRIVATE stm_lcd_touch_gt9271)
```

HAL_TIMEOUT 映射为 STM_ERR_TIMEOUT，HAL_ERROR/HAL_BUSY 映射为 STM_ERR_IO，start 失败保留首个错误并回收本次创建的对象，重复 start 不覆盖已有句柄。传输同步完成后才能复用缓冲；阻塞 API 不从中断调用。

## I²C 板级填写

填 `lcd_touch_gt9271_example_board_t` 的 i2c、address_7bit、可选 rst_port/rst_pin、逻辑宽高及方向。GT9271 用 0x5d/0x14、16 位寄存器；INT/RST 地址选择在调用 start 之前由板级完成，需按安装方向验证。HAL 接收 7 位地址左移一位。可在既有上电流程已完成复位时将 rst_port 设为空，避免重复复位。

start 接收 `&handle, &board`，poll 接收 `handle, points, capacity, &count`。poll 失败 count 为零；原样返回错误，不自动重试。零点/未就绪语义以芯片 README 为准。

完整 API、错误和资源契约见[中文主页](../../README.md)。
