# stm_lcd_touch_gt9271：GT9271 通用触摸驱动

构造函数返回 `stm_lcd_touch_handle_t`；芯片负责协议解析，框架负责坐标变换与单点快照，port 负责输入采样和松开状态。核心不依赖 MCU/HAL。

## 🤖 让 Agent 帮助接入

> 将 `stm_lcd_touch_gt9271` 接入当前工程。先读 AGENTS.md、README、公开头和 HAL 示例，核对 MCU、器件、总线、引脚及尺寸/方向，保留已有改动，不猜接线。使用通用句柄和配套本地框架，按组件协议提供 IO，不增加芯片专用 LVGL 包装。报告实际源码版本、软件验证和未验证项，未经确认不烧录或发布。

## 协议与快照

使用 2 字节寄存器地址，ID=0x8140，状态=0x814E，触点=0x814F，每点 8 字节；保留原最多 10 点的协议解析。校验整帧重复 ID，选择首个在配置原始范围内的有效点。复位保持 0/30 ms → 1/50 ms；RST/INT 地址选择由板级完成，不猜共享复位。7 位地址常量保留 0x14 与 0x5D。

未就绪帧保持上一原始快照，避免持续按住时提前松开；有效零触点帧释放。通信/畸形帧失败清空缓存。ready 帧读取/解析出错仍尝试写 0 到状态寄存器 ACK，首个错误优先；仅 ACK 失败也不发布新快照。`lcd_touch_gt9271_read_id(touch, id)` 保留：成功检查字符串 `9271`，不匹配返回 NOT_SUPPORTED 并保留实际 ID，通信失败清零 5 字节输出；port 借用后不能外部读 ID。

通用 API 暴露零或一个触点；读取完整原始多点帧不等于向应用暴露多点列表。`stm_lcd_touch_read_data` 更新快照；`get_data` 不访问总线、不消费快照，容量为零允许空点数组并输出零。失败清空有效快照，输出数量清零。坐标仅由框架执行一次 swap → 边界检查 → mirror；配置尺寸是变换后的逻辑范围，驱动的原始选点检查不修改坐标。

## 最小接入

先初始化原子寄存器 IO，再创建：

```c
stm_lcd_touch_handle_t touch = NULL;
lcd_touch_gt9271_config_t config =
{
    .io = &touch_io,
    .coordinates =
    {
        .x_max = display_width,
        .y_max = display_height,
        .swap_xy = 0,
        .mirror_x = 0,
        .mirror_y = 0,
    },
    .reset = board_reset,
    .delay_ms = board_delay,
    .control_context = &board,
};
stm_err_t err = lcd_touch_gt9271_create(&config, &touch);
if (err == STM_OK)
{
    err = stm_lcd_touch_reset(touch);
}
// 无 reset 回调时跳过 reset；GT 在 port 创建前还需读取并核对 ID。
```

FT5206 必需 `read_reg`；GT9271 必需 `read_reg/write_reg`。适配器必须保持 HAL Mem_Read/Write 的原子寄存器事务，不能照搬 AXS 的独立写—STOP—读响应协议。7 位设备地址在 HAL 边界只左移一次。

独立测试可 read 后 get；交给 port 的 `.touch` 后由 port 独占读取，不能另加应用轮询/触摸回调包装。IO/TIMEOUT 离线探测、过期释放由 port 统一完成，VERIFY 释放输入但不直接判离线。

## 从 v0.2.0 迁移

| v0.2.0 | 当前工作区 |
| --- | --- |
| `lcd_touch_gt9271_handle_t` | `stm_lcd_touch_handle_t` |
| I2C 读写回调与 `void *io` | 通用 IO 的原子 `read_reg/write_reg`，独立控制上下文 |
| 顶层 x_max/y_max/swap/mirror | `.coordinates`，框架只变换一次 |
| `lcd_touch_gt9271_reset/read_data/get_data/delete` | `stm_lcd_touch_reset/read_data/get_data/delete` |
| 多点输出数组 | 通用单点快照，保留完整原始帧解析与校验 |

```cmake
add_subdirectory(Lib/stm_lcd)
add_subdirectory(Lib/stm_lcd_touch_gt9271)
target_link_libraries(your_firmware PRIVATE stm_lcd_touch_gt9271)
```

## 生命周期与错误

`create` 要求输出句柄初始为 `NULL`，仅分配小型控制对象、复制配置并借用 IO，不访问硬件。创建失败不发布实例；非空输出返回 `STM_ERR_INVALID_STATE` 并保留原值。回调和上下文必须持续有效，IO、HAL、GPIO 与像素缓冲归应用所有。

删除顺序为 port → 设备 → IO → HAL/外部缓冲。删除空句柄成功，空句柄地址返回 `STM_ERR_INVALID_ARG`；被借用、正在传输或回调期间拒绝删除，实例保留供后续服务/重试。应用串行调用，禁止 ISR/递归访问；删除后自行清除其他别名。

所有操作/复位/传输回调返回 `stm_err_t`，以 `err != STM_OK` 判断错误，底层错误原样传递。延时回调保持 `void`。HAL 示例映射 `HAL_TIMEOUT` 为 `STM_ERR_TIMEOUT`，`HAL_ERROR/HAL_BUSY` 为 `STM_ERR_IO`。

| 情况 | 错误 |
| --- | --- |
| 空指针、空或倒置矩形、非法调用参数 | `STM_ERR_INVALID_ARG` |
| 缺少必需 IO 能力、尺寸/布尔配置非法 | `STM_ERR_INVALID_CONFIG` |
| 重复创建、未初始化、对象被借用或操作在途 | `STM_ERR_INVALID_STATE` |
| 控制对象分配失败 | `STM_ERR_NO_MEM` |
| 绘图越界或字节数溢出 | `STM_ERR_OUT_OF_RANGE` |
| 未实现的可选能力 | `STM_ERR_NOT_SUPPORTED` |
| 协议点数/触点 ID 校验失败 | `STM_ERR_VERIFY` |

## CMake 与离线依赖

公开链接 `stm_common` 与 `stm_lcd`，核心不依赖 HAL、LVGL 或日志。`stm_common` 解析顺序为已有 target → 同级源码 → 固定 v1.0.0 提交 `ce3d186dde2d374a8e9c7b9068a7b88f97d57dc1`；自动获取支持 `FETCHCONTENT_SOURCE_DIR_STM_COMMON` 离线覆盖、`STM_COMMON_FETCH=OFF` 和 `STM_COMMON_GIT_REPOSITORY` 镜像。

`stm_lcd` 解析顺序为已有 target → `STM_LCD_SOURCE_DIR` → `FETCHCONTENT_SOURCE_DIR_STM_LCD` → 同级源码 → 固定 v1.0.0 提交 `c359e54a657be38aec90c797ea19ee3d492d9284`。`STM_LCD_FETCH=OFF` 禁止下载；`STM_LCD_GIT_REPOSITORY` 可指向 GitHub/Gitee 镜像，默认固定 SHA 不改变。无效显式目录直接报错，不退回网络；多个组件使用同一 `stm_lcd` target/FetchContent 名称。

当前迁移组合的 ILI9881C、FT5206、GT9271 与新版 port 使用尚未发布的帧缓冲/原子寄存器扩展，**必须一起提供匹配的 `stm_lcd` 源码（聚合仓库 gitlink 固定）**；已发布 v1.0.0 不具备这些能力，配置时明确报错。ST7789/ST7796 核心仍可使用该正式框架的同步接口。依赖不自动追踪 main，也不伪造未来版本 SHA。

## 软件验证与版本边界

```sh
cmake -S tests -B build/tests -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

测试保留原协议用例，补充通用句柄、空参数/非法配置、重复创建、分配失败、借用回滚、删除重建、多实例与错误传递；公共头按 C11/C++17 消费。中文 HAL 示例见 [examples/stm32_hal](examples/stm32_hal/README.md)。同级新版 port 的集成测试将五种器件交给同一份 port 源码，并检查 PARTIAL/DIRECT 及失败路径。

当前提交是尚未发布新版本的通用接口迁移，原 `v0.2.0` tag 保留原 API，未发布新 tag 或 Release。

2026-10-11，匹配的本地 stm_lcd、ILI9881C、GT9271、stm_lvgl_port 和 LVGL 9.3.0 在 H757 配套 10.1 寸模组上通过 DIRECT 诊断 Debug 回归：800×1280 RGB565、DSI 两通道、板级 DMA2D，ST-Link 双核烧录独立读回、刷新持续推进、触摸/按钮事件和五次软件复位均正常，用户确认画面与触摸正常。触摸板级使用 PB10/PB11 软件 I2C，7 位地址 0x5d，mirror_x/y=0。

本次未重新验证 PARTIAL、Release、Widgets、掉电复位或长期稳定性，未独立量化各角坐标；HAL 硬件 I2C 示例仍为编译验证。其他模组/平台不在本次实板范围内，旧 tag 的验收结果不移用。固件、源码哈希和硬件日志留在消费工程本地构建目录。

## 许可证

[MIT](LICENSE)，保留维护者和既有来源说明。
