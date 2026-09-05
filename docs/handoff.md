# Rongine 渲染路线 Handoff

> 文档用途：记录引擎渲染架构现状、学习计划、算法实现队列、架构演进目标与实现路径。
> 作为「接下来做什么、为什么这么做、做到哪了」的单一事实来源。
> 最后更新：2026-09-05
> 关联文档：[multithreaded-rendering-roadmap.md](multithreaded-rendering-roadmap.md)（2026-06-03，多线程渲染分阶段任务，部分内容已被后续提交超越，以本文为准）

---

## 1. 当前架构现状（2026-09-05）

### 1.1 总体定位

**单渲染线程 MVP + lambda 任务队列 + 立即模式 GL 抽象**。
已跨入多线程渲染第一步，但帧流水线没有重叠（形态 2：同步双线程），距 Unreal/Unity 式 RHI + 多线程帧流水线有明确差距。

### 1.2 已有资产

| 模块 | 位置 | 说明 |
|------|------|------|
| 主循环 | `Rongine/src/Rongine/Core/EntryPoint.h` → `Rongine.ApplicationImpl.ixx` `Application::run()` | 可变 dt + VSync 限帧；pollEvents → onUpdate → submit(ImGui+swap) → sync |
| 渲染线程 | `Rongine/src/Rongine/Renderer/Threading/Rongine.RenderThread.ixx` | GL 上下文独占渲染线程；`std::queue<std::function>` + mutex + condvar；接口 `start/shutdown/submit/sync` |
| 渲染抽象 | `Rongine.Renderer.Interfaces.ixx`（`RendererAPI`/`RenderCommand` 静态门面） | 立即模式：每个调用立即转发 GL；仅 OpenGL 实现 |
| Pass 层 | `RenderPass` / `RenderGraph` / `PipelineState` / `UniformBuffer` / `Material` | RenderGraph 为**顺序 Pass 列表**，非数据依赖驱动 |
| 编辑器帧流程 | `Rongine-Editor/src/RongineEditor.EditorLayer.ixx` | 主线程 `buildRenderGraph()`（GeometryPass + WireframePass，声明式 spec）；渲染线程 `execute()` 后做 FBO `readPixelID` 拾取；相机 UBO 走 binding=0 |
| 事件系统 | 主循环帧首 `pollEvents` 同步分发；ImGui NewFrame 拆为 GL 后端（渲染线程）/平台输入（主线程）双阶段 | 提交 `1274a0a` |
| Compute 旁路 | 光谱路径追踪 `RenderComputeFrame` | 走同一 submit 队列 |

C++20 module（`.ixx`）构建已迁移完成（`fe17ffc`）。

### 1.3 已知问题

- **每帧 `sync()` 全等**：主线程帧尾阻塞等渲染线程，流水线零重叠；`EditorLayer` 内 resize 等场景还有帧中 sync。
- **`sync()` 等待期间泵消息**：等待循环里 `glfwPollEvents()` + `PeekMessageW`，事件可能在帧中间被重入分发，与帧首 poll 形成乱序双路径（resize/close 可能一帧内处理两次）。
- **通信开销**：每次 `submit` 一次 condvar `notify_one`（内核调用级开销）+ 一次 `std::function` 堆分配；渲染线程 `wait_for(1ms)` 轮询。
- **可变时间步**：无固定模拟步长，物理/回放/网络同步不可扩展。
- **无命令缓冲**：`RenderCommand` 直接打 GL，无录制/回放，无法做录制/执行并行，也无法迁现代 API。
- **无排序**：entt 遍历直接 draw，无材质/深度排序、无实例化。
- 旧 roadmap 阶段 0~4（FramePacket / 双缓冲 FBO + Fence / 弱化 sync / PBO 拾取）大部分未完成，其目标被本文 §5 的新路径吸收。

### 1.4 在引擎谱系中的位置

对照 UE（Game/Render/RHI 三线程 + Scene Proxy 快照）、Filament（单驱动 + fence 环 + 严格帧生命周期）、bgfx（encoder 录制 + 单 API 线程消费 + 确定性排序）、Godot 4（Server 命令队列 + 按需同步）：
Rongine 已有"队列 + 专职消费线程"原语，缺**命令缓冲、帧重叠（FiF）、per-frame 资源轮转**三件套。

---

## 2. 总路线：算法先行，三阶段演进

核心决策（2026-09 定）：**不先重构架构**。渲染算法与引擎架构正交，先用现有 GL 层把 GAMES202 算法落地（视觉反馈强、知识保质期长），再学 Vulkan，最后让 RHI 从两个 API 的共性里长出来。

```
阶段一（现在，2-4 个月）：GAMES202 算法在 Rongine 落地   ← 看第 3 节
阶段二（1-3 个月）：GPU 模型 + Vulkan（vkguide.dev 路线）← 看第 5.2 节
阶段三：RHI 设计 + 渲染架构重构                          ← 看第 5.3 节
```

**阶段一纪律**（防手痒）：
1. 引擎侧只加不改——允许加 pass 类型、MRT、FBO 格式，禁止顺手重构底层。
2. 算法逻辑收敛在"一个 RenderGraph pass + 几个 shader"边界内，不渗入 Renderer3D/Layer/Application 管道，保证阶段三可整体搬迁。
3. 不做任何"提前为 Vulkan 设计"的抽象。

---

## 3. 阶段一：GAMES202 算法实现队列

课程进度：已学至第九讲（PCSS/VSSM/SDF 阴影、PRT/SH/小波、RSM/LPV/VXGI、SSAO/SSDO/SSR）。
后续课程（PBR 材质+IBL split-sum、实时光追、降噪 SVGF）**必须学完**——是全课工业价值最高的部分。

### 3.1 实现优先级

| 优先级 | 算法 | 理由 | 依赖 |
|--------|------|------|------|
| 必做 ① | **PCSS 软阴影** | 性价比最高：深度 pass、blocker 搜索、Poisson 采样、可变滤波半径 | 无 |
| 必做 ② | **SSAO** | 成本极低，第一个屏幕空间算法，SSDO/SSR 思想家族的入门 | 无 |
| 必做 ③ | **SH（球谐）+ 简化 PRT** | PRT 工业界已弃用，但 SH 数学是 IBL/探针/Lumen 的共同地基，必须内化；只做漫反射 2~3 阶投影 demo | 无 |
| 基建 | **Bloom + ACES tonemap 后处理链** | SSR 的跑道；顺带练 pass 编排 | 无 |
| 必做 ④（学完 L10-13 后） | **PBR 直接光 + IBL split-sum** | 全课头号重点，引擎没有 PBR+IBL 等于没有光照系统 | 后处理链 |
| 选做 | VSSM、SSDO | 数学漂亮/增量小，有余力再做 | PCSS / SSAO |
| 必做 ⑤ | **SSR** | 需要本帧着色结果+深度/法线，挂后处理链做最顺 | PBR/IBL + 后处理链 |
| 只读不写 | RSM / LPV / VXGI / 小波 / SDF 阴影 | 历史脉络与思想（Lumen 的前身），实现烧时间且资产不可迁移 | — |

每实现一个算法前问一句：**"它在我下一个阶段还有用吗？"** 押注可迁移资产。

### 3.2 执行顺序

```
PCSS → SSAO → 简化 PRT/SH   （穿插继续看课）
  → Bloom/tonemap 后处理链
  → （学完 L10-13）PBR + IBL split-sum   ← 中场哨
  → SSR
  → （学完 RT/降噪）理论储备；实现推迟到阶段二有 Vulkan ray query 后
```

---

## 4. 关键概念备忘（讨论结论沉淀）

设计决策时的依据，避免重新推导：

- **帧延迟公式**：`延迟 ≈ 1(构建) + 已提交未完成积压(≤FiF) + 1(GPU执行) + 已渲染未上屏积压(≤缓冲数-1)`。FiF 与 swapchain 深度是两个独立旋钮；三缓冲只在 GPU 瓶颈时有意义，代价是排队深度。
- **背压分级**：fence 已 signal → 直写映射内存（零等待）；未 signal → CPU 暂存 + 命令流内推迟（顺序性即正确性）；队列满 → 帧边界阻塞（兜底）。等待会转移不会消失，工程上选择"等什么/在哪等/何时等"。
- **late latch**：相机常量在命令回放最后一刻用最新输入重写（ring 的最后槽位），抵消 CPU 侧流水线深度；剔除仍用帧首相机保证一致。纯旋转安全，平移有视差穿帮。对 Rongine：相机 UBO 已在 ring，改造点小。
- **队列通信开销**：锁不贵、争用贵。UE 三层方案：架构上入队次数 O(改动数)（proxy 快照把重数据挪出队列）+ 无锁 MPSC（一次 CAS，标签指针解 ABA）+ 帧重叠让队列浅。Rongine 当前真正贵的是每 submit 一次 condvar notify + function 堆分配。
- **RHI 选型结论**：NVRHI（薄、显式、MIT、内建验证层、D3D12/Vulkan/Metal）为主选；WebGPU 仅在需要浏览器时考虑；The Forge 是框架不是纯 RHI，读源码比集成好。自写 Vulkan-only 是学习价值最高的路线，定价 6~12 个月。
- **引擎侧不变量**（改造线程模型时确立）：事件只属于主循环帧首，禁止帧中重入分发。

---

## 5. 架构演进目标与实现路径

### 5.1 目标架构（阶段三完成态）

```
主线程（游戏/编辑器逻辑）
  ├─ pollEvents（唯一事件入口，帧首）
  ├─ 固定步长模拟 tick + 渲染插值
  └─ 录制命令缓冲（RenderCommand 门面 → record，不直接调 API）

渲染线程 / RHI 层
  ├─ 命令缓冲 flush（录制/执行并行）
  ├─ 帧栅栏 + 允许落后一帧（OneFrameThreadLag 式，替代每帧全等 sync）
  ├─ per-frame 资源 ring（UBO/拾取 buffer ×2~3 轮转 + fence 门控复用）
  ├─ late latch：回放前重写相机常量
  └─ FrameGraph（Pass 声明读写 target，图驱动 barrier 与顺序）

后端：OpenGL（过渡）→ Vulkan / D3D12 / Metal（经 RHI）
```

### 5.2 阶段一与目标架构的关系：现在就要守住的接口边界

阶段一写算法时，以下约束让资产可迁移（成本≈0，收益巨大）：

1. 算法 = RenderGraph pass + shader 文件，不碰管道代码。
2. 每帧常量（相机、光照参数）统一走 UBO 声明式 spec，为 ring 轮转和 late latch 留位。
3. Pass spec 里显式声明 `TargetFramebuffer`（已是现状）——阶段三 FrameGraph 从这里长出读写声明。
4. 新增任何 GL 资源创建/调用走 `RenderCommand` 门面，不要绕过。

### 5.3 阶段二：Vulkan 学习（进入阶段三的前置）

- 材料：vkguide.dev（工程向）+ vulkan-tutorial + spec 当字典。
- 形式：独立小项目或分支重写渲染器，范围收敛：device/swapchain → 命令缓冲 + FiF=2 → 描述符 → 一个 mesh pass + 一个后处理 pass。
- **毕业标准**：能独立回答本文 §4 的每个时序问题在 Vulkan 里的落地方式（acquire/submit/present 信号量链、fence 轮转、barrier 推导）。
- 阶段一里"撞墙"的问题（半分辨率、MRT、带宽）是带着问题学 API 的素材。

### 5.4 阶段三：RHI + 重构的实施顺序

1. **命令缓冲插入 `RenderCommand` 门面之后**（学 bgfx）：record 代替立即 GL 调用，帧尾渲染线程 flush。不改线程模型，为翻译层/RHI 铺路；GL 调用点收敛到唯一一处。
2. **每帧 sync → 帧栅栏 + 落后一帧**（学 UE `OneFrameThreadLag`）：共享数据改快照/消息；拾取 `readPixelID`（GPU→CPU 回读）单独处理；确立"事件只属于帧首"不变量。旧 roadmap 的 FramePacket/双缓冲 FBO/FrameSync 方案在这一步合并落地。
3. **per-frame 资源 ring + fence 环**（学 Filament）：相机 UBO、拾取 buffer ×2~3 轮转。
4. **late latch**：回放前最后一刻重写相机 UBO，编辑器 Gizmo/转视角跟手。
5. **RenderGraph → FrameGraph**：Pass 声明读写 target，图推导 barrier 与顺序（GL 上先只做顺序+状态校验，Vulkan 后端长成真 frame graph）。
6. **固定时间步 + 插值**：accumulator 分离模拟 tick 与渲染 tick。
7. **RHI 后端替换**：`Rongine.Renderer.OpenGL.ixx` 换成 Vulkan 后端（自写或 NVRHI，届时按阶段二学习成果决定）；`RenderCommand` 门面与 RenderGraph 接口不动，算法资产整体搬迁。

### 5.5 里程碑

- [ ] M1：PCSS + SSAO 在 Rongine 跑通
- [ ] M2：SH/简化 PRT + 后处理链（Bloom/ACES）
- [ ] M3：PBR 直接光 + IBL split-sum（阶段一核心交付）
- [ ] M4：SSR
- [ ] M5：Vulkan 独立渲染器（阶段二毕业）
- [ ] M6：命令缓冲 + 帧栅栏 + 落后一帧
- [ ] M7：per-frame ring + late latch
- [ ] M8：Vulkan RHI 后端，算法资产搬迁完成

---

## 6. 变更日志

| 日期 | 说明 |
|------|------|
| 2026-09-05 | 初版：汇总架构现状评估、三阶段路线决策（算法先行）、GAMES202 实现队列、概念备忘与重构路径；吸收 2026-06-03 多线程 roadmap 的未完成项 |
