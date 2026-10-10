# GT9271 STM32 HAL 接入示例

源码为 [example.c](example.c) 和 [example.h](example.h)，使用真实 STM32H7 HAL 头文件；消费工程需先完成 I2C，7 位地址、坐标尺寸/方向、可选 RST。GT9271 的 INT/RST 地址选择和共享复位必须先由板级协调。 示例不猜引脚、时钟或屏幕通道。

## 添加与启动

将 `example.c` 编入消费工程，并添加本目录 include，链接 `stm_lcd_touch_gt9271` 与实际 HAL target。组件间依赖按[组件 README](../../README.md)设置，使用本次迁移匹配的本地框架。

```c
static lcd_touch_gt9271_example_board_t board; // 初始化硬件字段；内嵌 IO 首次必须为零。
static stm_lcd_touch_handle_t device = NULL;
// 填写 board 的 HAL、引脚、尺寸等实际配置后：
stm_err_t err = lcd_touch_gt9271_example_start(&device, &board);
// 成功时把通用句柄传给 port；板级 board 不得离开作用域。
```

`example_start` 建立通用 IO、创建设备、可选硬复位；面板还执行 init。失败回收本次拥有的设备和 IO，HAL/引脚/帧缓冲归应用。HAL_TIMEOUT → TIMEOUT，HAL_ERROR/HAL_BUSY → IO，驱动不吞错误。

独立驱动测试用 `lcd_touch_gt9271_example_poll`（read/get）；port 创建后停止应用侧轮询，由 port 统一服务。单点快照按框架逻辑坐标返回。

## 接入同一个 LVGL port 与退出

port 配置直接填写 `.io`、`.panel`、可选 `.touch`、外部缓冲和时钟，不增加芯片专用 LVGL 绘图/输入包装。删除顺序：先 `lvgl_port_delete(&port)`，然后 `lcd_touch_gt9271_example_stop(&device, &board)`；该函数删除设备后 deinit 内嵌 IO，不释放 HAL。在途/被借用/停止失败时保留资源，继续服务并重试，不清零有效对象。

本示例经过主机/真实 HAL 编译检查。2026-10-11 H757 配套屏幕消费工程使用相同通用组件通过显示/触摸与五次软件复位，具体范围见[组件 README](../../README.md)。板级工程使用自己的 DSI/LTDC 和软件 I2C 适配，不能将该结论当作本示例所有 HAL 操作或接线的实板验收。
