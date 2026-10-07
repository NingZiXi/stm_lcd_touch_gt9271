# stm_lcd_touch_gt9271：GT9271 I²C 触摸驱动

提供寄存器解码、最多 10 点缓存与 swap_xy/mirror_x/mirror_y 坐标变换，不管理 I²C 外设。

## 最小调用

```c
lcd_touch_gt9271_handle_t touch = NULL;
lcd_touch_gt9271_config_t cfg = {
    .read_reg=board_read_reg, .write_reg=board_write_reg,
    .io=&board_io, .x_max=BOARD_LCD_WIDTH, .y_max=BOARD_LCD_HEIGHT,
    .swap_xy=0, .mirror_x=0, .mirror_y=0,
};
stm_err_t err = lcd_touch_gt9271_create(&cfg, &touch);
lcd_touch_gt9271_point_t point;
size_t count = 0;
if (err == STM_OK) err = lcd_touch_gt9271_read_data(touch);
if (err == STM_OK) err = lcd_touch_gt9271_get_data(touch, &point, 1, &count);
/* 退出时 lcd_touch_gt9271_delete(&touch)。有 reset 回调时须同时提供 delay_ms。 */
```

## 错误与资源契约

所有操作和传输/复位回调返回 `stm_err_t`，成功为 `STM_OK`，失败检查 `err != STM_OK`，不能使用 `err < 0`。HAL 适配将 `HAL_TIMEOUT` 映射为 `STM_ERR_TIMEOUT`，`HAL_ERROR/HAL_BUSY` 映射为 `STM_ERR_IO`；组件原样传递回调错误，延时回调仍返回 void。

| 情况 | 错误 |
| --- | --- |
| 空参数、非法调用参数 | `STM_ERR_INVALID_ARG` |
| 缺少必需回调、尺寸或方向配置错误 | `STM_ERR_INVALID_CONFIG` |
| 输出句柄非空、面板未初始化 | `STM_ERR_INVALID_STATE` |
| 控制对象/LVGL 对象分配失败 | `STM_ERR_NO_MEM` |
| 绘图越界或像素长度计算溢出 | `STM_ERR_OUT_OF_RANGE` |
| GT9271 ID 不匹配 | `STM_ERR_NOT_SUPPORTED` |
| 触摸帧点数等数据校验失败 | `STM_ERR_VERIFY` |

`create(config, &handle)` 要求 handle 初始为 NULL；复制配置，用 calloc/free 管理小型控制对象，芯片 create 不访问硬件。创建失败保持输出为空；非空输出被拒绝且原值不变。`delete(&handle)` 仅回收拥有的对象，成功清空 handle，空句柄也成功；NULL 句柄地址是参数错误。删除前停止并发访问，其他别名不会被自动清空。

板级拥有 HAL、总线、GPIO、背光、外部缓冲和回调上下文；组件不释放或重新配置这些资源。实例使用期间上下文必须有效，可用 NULL io 表示无上下文。应用串行调用，组件不默认线程安全，不在中断中调用阻塞操作，不增加日志/RTT/RTOS 依赖。同步传输返回前必须用完输入缓冲；共享总线在整笔事务外加锁，DMA/DCache 一致性由板级管理。

## 地址、坐标与失败状态

板级保存 7 位 I²C 地址，HAL 参数用 `address_7bit << 1`。GT9271 常用 0x5d/0x14，INT/RST 地址选择由板级执行；FT5206 常用 0x38，按实物核实。寄存器地址宽度分别为 GT9271 16 位、FT5206 8 位。

x_max/y_max 为变换后的逻辑尺寸，先 swap_xy、检查范围、再镜像；方向标志仅允许 0/1。越界点被过滤。get_data 返回实际复制的 min(点数,capacity)，capacity=0 允许空点数组；失败清零有效 count。复位清空缓存，通信/畸形帧失败清空本次状态，不自动重试；下一次 read_data 可恢复。

GT9271 无新帧时保持上一触摸状态，只有有效零点帧释放。就绪帧解析失败仍尝试 ACK，同时保留首个错误；ACK 失败不提交触点。read_id 读 4 字节并补 NUL，产品不匹配保留实际 ID，通信失败输出清空。

## CMake 与依赖

依赖 `stm_common` 的 `stm_err.h`，不复制公共错误码。优先复用已有 `stm_common` target，其次找同级源码；缺失时自动下载固定 v1.0.0 提交 `ce3d186dde2d374a8e9c7b9068a7b88f97d57dc1`。可设置 `STM_COMMON_FETCH=OFF` 禁止下载，`STM_COMMON_GIT_REPOSITORY=https://gitee.com/nzxhg/stm_common.git` 指定镜像，或 `FETCHCONTENT_SOURCE_DIR_STM_COMMON` 指定离线源码。已有 target/同级源码无需网络。

```cmake
add_subdirectory(Lib/stm_lcd_touch_gt9271)
target_link_libraries(your_firmware PRIVATE stm_lcd_touch_gt9271)
```

手动集成时添加组件 include/源码及 stm_common 头文件目录。LVGL port 还要求应用提前提供 LVGL 9 的 `lvgl` target 和配置。

## 从 v0.1.0 迁移

| 旧接口 | 当前接口 |
| --- | --- |
| `stm_lcd_touch_gt9271_t` 公开结构体 | `lcd_touch_gt9271_handle_t`，初始 NULL |
| `stm_lcd_touch_gt9271_config_t` | `lcd_touch_gt9271_config_t` |
| `new_i2c(实例地址, config)` | `lcd_touch_gt9271_create(config, &handle)` |
| 直接访问结构体 / 无销毁接口 | `lcd_touch_gt9271_delete(&handle)` |
| int 与负数错误码 | `stm_err_t`，`err != STM_OK`，回调同步迁移 |

其他操作使用 lcd_touch_<型号> 前缀，point_t 与 MAX_POINTS 常量也使用此组件前缀。

2026-10-07，`v0.2.0` 接口在 STM32H757XIH6 CB V1.0、WKS101HD031-WCT 10.1 寸 800×1280 模组（ILI9881C/GT9271）、LVGL 9.3.0 上完成回归：诊断显示持续刷新、按钮与触摸输入、五次连续软件复位及 Release 启动通过；官方 Widgets 的滑动、点击由用户现场确认正常，读取状态中刷新、输入和切帧错误均为 0。板级使用 RGB565 DIRECT 双缓冲，触摸 mirror_x=0、mirror_y=0；结论限于该组合，不代表其他模组已验证。 本轮状态只保存最后触点，未单独留存各角坐标测量记录。

## 软件验证与发布状态

```sh
cmake -S tests -B build/tests -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

主机测试覆盖参数/配置、分配失败、资源回收、多实例和错误传递，并编译 C11/C++17 公共头文件。测试分配器仅用于测试构建，不加入产品固件。中文 HAL 示例见 [examples/stm32_hal/README.md](examples/stm32_hal/README.md)。许可证见 [LICENSE](LICENSE)。

`v0.2.0` 采用不透明句柄、`create/delete` 和统一 `stm_err_t`，包含破坏性接口迁移，不保留旧接口包装。`v0.1.0` 继续保留；升级前按上表迁移类型、回调和生命周期。此版本的主机测试、C11/C++17 头文件、中文 HAL 示例及 H757 Debug/Release 集成构建已通过。
