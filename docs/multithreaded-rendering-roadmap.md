# Rongine 多线程渲染架构规划

> 文档用途：记录目标架构、分阶段任务与当前进度，供后续按此开发。  
> 最后更新：2026-06-03

---

## 1. 目标

构建如下多线程渲染模型：

1. **主线程**：`pollEvents`、游戏/编辑器逻辑、`Input`、录制帧数据（`RenderFramePacket`），不直接调用 OpenGL。
2. **渲染线程**：持有 GL 上下文，执行场景绘制、光追 Compute、FBO 拾取读回、`ImGui` 绘制、`swapBuffers`。
3. **同步**：用 `glFenceSync` + 双缓冲 FBO 控制拾取读回时机；逐步减少「整帧 `RenderThread::sync()`」带来的主线程阻塞。
4. **拾取**：ID Buffer 颜色拾取在**绘制完成的 FBO** 上读取；同帧零延迟语义需 wait 本帧 fence（非边画边读同一缓冲）。

长期方向（与本规划并行、非阻塞项）：`RenderQueue` 排序/实例化、RDG 依赖图、CPU 射线拾取作为备选。

---

## 2. 当前代码基线（2026-06-03）

### 2.1 已有

| 模块 | 路径 | 状态 |
|------|------|------|
| 渲染线程队列 | `Rongine/src/Rongine/Renderer/Threading/RenderThread.cpp` | `submit` + `sync` 可用 |
| 主循环 | `Rongine/src/Rongine/Core/Application.cpp` | 主线程 `onUpdate` → 渲染线程 `ImGui+swap` → `sync` |
| 视口 resize | `Rongine-Editor/src/EditorLayer.cpp` (~102-116) | GL resize 在 `submit` 内，带 `sync` |
| 光追路径 | `EditorLayer.cpp` (~456-489) | `submit(Upload+BVH+Compute)`，**待修**见 2.2 |
| 光栅路径 | `EditorLayer.cpp` (~500-506) | `submit(execute)` + `sync`，**拾取仍在主线程** |
| FrameSync 占位 | `Rongine/src/Rongine/Renderer/Threading/FrameSync.h` | 空文件，待实现 |

### 2.2 已知问题（开发前必读）

- [x] **光栅拾取**：已迁入单次 `submit(execute + readPixelID)`，主线程无 GL。
- [x] **光追 lambda**：已按值捕获 `time`、`resetAccum`；`UploadSceneDataToGPU` 拼写已修正。
- [ ] **每帧多次 sync**：`EditorLayer` 内 resize + 光栅 `sync`，再加 `Application` 末尾 `sync`，主线程仍偏串行（阶段 3）。
- [x] **ImGui/GLFW**：输入在主线程；`ImGui` OpenGL 在渲染线程（保持）。

---

## 3. 目标架构图

### 3.1 线程职责

```
主线程                          渲染线程
────────                        ────────
pollEvents                      glfwMakeContextCurrent
onUpdate: 逻辑/Input/录包        ExecuteFrame(packet):
buildRenderGraph (仅组 Pass)       - 光追 或 光栅 draw → FBO[write]
填 PickRequest / 相机 / 场景版本   - glFenceSync
submit(ExecuteFrame)             - pick: WaitFence → readPixelID
submit(ImGui+swap)  (或合并)       - ImGui + swapBuffers
(可选) WaitFence / 读 pick 结果
点击逻辑（无 GL）
```

### 3.2 双缓冲 FBO + Fence（阶段 2）

```
FBO[0]  FBO[1]
   ↑       ↑
   └── ping-pong：writeIndex / readIndex

每帧（渲染线程）：
  bind FBO[write]
  RenderGraph.execute() / Compute
  Fence[write] = glFenceSync(...)

拾取模式 A（默认，同帧零延迟）：
  ClientWaitSync(Fence[write])
  readPixelID(FBO[write])
  swap(write, read)

拾取模式 B（可选，重叠 GPU/CPU）：
  不 wait 当前 write
  readPixelID(FBO[read])  // 上一帧已完成
  GPU 并行画 FBO[write]
  → 相机剧烈运动时可能 1 帧 ID 滞后
```

### 3.3 与「终极渲染架构」关系

| 层级 | 内容 | 文档/状态 |
|------|------|-----------|
| 线程模型 | 本文件 | 进行中 |
| 提交优化 | RenderQueue、排序、实例化 | 未开始 |
| Pass 调度 | RDG 依赖图 | `RenderGraph` 现为线性执行器 |
| 多后端 | RHI | 未开始 |

---

## 4. 分阶段任务与进度

进度图例：`[x]` 完成 · `[~]` 进行中 · `[ ]` 未开始

### 阶段 0：修正线程边界（预计 1–2 天）

**目标**：无主线程 GL；光栅/光追路径可稳定运行。

- [x] 主线程 `onUpdate`，渲染线程 `ImGui + swap`（`Application.cpp`）
- [x] 视口 resize 的 GL 操作在 `submit` 内
- [~] 光追：`submit` 内 Upload + BVH + Compute
  - [ ] lambda 按值捕获：`scene, rebuild, camera, time, resetAccum`
  - [ ] 修正 `UploadSceneDataToGPU` 命名
- [~] 光栅：`submit(execute)` + `sync`
  - [ ] 将 **FBO 像素拾取** 移入同一 `submit`（`execute` 之后）
  - [ ] NURBS 控制点投影拾取保留主线程（纯数学）
  - [ ] `sync` 后仅做点击确认逻辑（无 GL）
- [ ] 删除主线程 `m_framebuffer->bind/readPixelID`（光栅拾取块）

**验收**：拖动窗口/旋转相机/悬停实体/点击选择正常；无主线程 GL 调用。

**参考代码结构**（`EditorLayer::onUpdate` 光栅段）：

```cpp
buildRenderGraph();
// 主线程：mouseX/Y, viewportSize, NURBS 控制点, doFboPick
RenderThread::submit([this, mouseX, mouseY, viewportSize, doFboPick]() {
    Renderer3D::resetStatistics();
    m_renderGraph.execute();
    if (doFboPick) { /* bind FBO, readPixelID, 写 m_Hovered* */ }
});
RenderThread::sync();
// 主线程：Phase2 点击逻辑
```

---

### 阶段 1：FramePacket 统一提交（预计 3–5 天）

**目标**：CPU「录帧」、GPU「消费」；`onUpdate` 内不再散落多个 `submit`。

- [ ] 新增 `RenderFrame.h`（或 `Renderer/Threading/RenderFramePacket.h`）
- [ ] 新增 `ExecuteFrame(packet)`（渲染线程入口）
- [ ] `EditorLayer` 填充 packet：`camera`, `time`, `scene`, `rebuild`, `pick`, `rayTracing` 等
- [ ] `Application::run` 改为每帧：
  1. `onUpdate`（各 Layer 只填 packet / 逻辑）
  2. `submit(ExecuteFrame)`
  3. `submit(ImGui+swap)` 或合并为单次 submit
  4. `sync`（阶段 1 可保留）

**验收**：`EditorLayer` 光栅/光追路径无直接 `submit` 散落；帧逻辑可读。

---

### 阶段 2：FrameSync + 双缓冲 FBO + Fence（预计 1 周）

**目标**：实现规划中的 fence 同步与 FBO ping-pong。

- [ ] 实现 `FrameSync.cpp`：
  - `InsertFence(index)`
  - `WaitFence(index, timeout)`
  - `SwapFramebufferIndices()`
- [ ] `EditorLayer` 或 `FrameSync` 管理 `m_Framebuffers[2]`
- [ ] `buildRenderGraph` 的 `TargetFramebuffer` 绑定 `FBO[writeIndex]`
- [ ] 拾取模式 A（默认）：`WaitFence(write)` 后 `readPixelID`
- [ ] （可选）拾取模式 B：读 `FBO[read]` 与画 `FBO[write]` 并行

**验收**：Profiler 可见 pick 等待时间；相机旋转时拾取仍准确（模式 A）。

---

### 阶段 3：弱化整帧 sync（预计 3–5 天）

**目标**：主线程只等待「场景+fence」，不必空等整个队列。

- [ ] `RenderThread::sync` 仅用于 shutdown / resize 等特殊情况
- [ ] 每帧 `WaitFence` 替代 EditorLayer 内光栅专用 `sync`
- [ ] `Application` 末尾合并场景+UI 任务或精确依赖 fence

**验收**：主线程 `onUpdate` 耗时下降；无死锁、无花屏 pick。

---

### 阶段 4（可选）：PBO 异步读 ID

- [ ] `readPixelID` 改为 PBO 异步回读
- [ ] 悬停用上一帧结果，点击时强制 sync 读

---

## 5. 关键文件清单

| 文件 | 阶段 | 动作 |
|------|------|------|
| `Rongine-Editor/src/EditorLayer.cpp` | 0–2 | 拾取/光追/光栅提交改造 |
| `Rongine-Editor/src/EditorLayer.h` | 2 | 可选双 FBO 成员 |
| `Rongine/src/Rongine/Core/Application.cpp` | 1–3 | 帧循环与 submit 顺序 |
| `Rongine/src/Rongine/Renderer/Threading/RenderThread.*` | 1 | 可选 `submitFrame(packet)` |
| `Rongine/src/Rongine/Renderer/Threading/FrameSync.*` | 2 | Fence + 缓冲切换 |
| `Rongine/src/Rongine/Renderer/Threading/RenderFramePacket.h` | 1 | 新建 |
| `Rongine/src/Rongine/Renderer/Renderer3D.*` | 0 | 仅调用方式，少改接口 |
| `Rongine/src/Rongine.h` | 1 | 导出新头文件 |

---

## 6. 每帧顺序（目标态）

```
1. Main: pollEvents
2. Main: Layer::onUpdate
       - 填 RenderFramePacket
       - buildRenderGraph()（主线程）
3. Render: ExecuteFrame(packet)  // 场景 + fence + pick
4. Render: ImGui begin/end + swapBuffers
5. Main: 使用 pick 结果（若 ExecuteFrame 写回成员）
       - 或 WaitFence 后读（阶段 3 细化）
```

**当前态**（阶段 0 未完成）：

```
onUpdate 内: submit(execute) → sync → 主线程 pick(GL)  // 待改为 render 内 pick
Application: submit(ImGui) → sync
```

---

## 7. 设计约束（开发时遵守）

1. **所有 GL 调用**在 `RenderThread::isRenderThread() == true` 时执行。
2. **lambda 捕获**：一律按值捕获相机、`time`、`reset`、鼠标坐标等，禁止 `[&]` 捕获栈变量。
3. **场景指针**：`submit` 执行期间主线程不修改同一 `Scene` 的 mesh 数据；或引入 `sceneVersion` 检测冲突。
4. **ImGui 输入**：`pollEvents` + `Input` 在主线程；不在主线程调用 `ImGui_ImplGlfw_NewFrame`（保持与 `ImGuiLayer` 一致）。
5. **拾取**：读 FBO 前必须 fence 完成；双缓冲时禁止读正在写的 attachment。

---

## 8. 进度总览

| 阶段 | 名称 | 状态 | 完成度 |
|------|------|------|--------|
| 0 | 线程边界修正 | 进行中 | ~60% |
| 1 | FramePacket | 未开始 | 0% |
| 2 | 双缓冲 + Fence | 未开始 | 0% |
| 3 | 弱化 sync | 未开始 | 0% |
| 4 | PBO 异步拾取 | 未开始 | 0% |

**建议下一步（阶段 0 收尾）**：

1. 按第 4 节「阶段 0」完成光栅拾取迁入 `submit`。
2. 修复光追 lambda 捕获与 `UploadSceneDataToGPU` 拼写。
3. 编译运行，验证悬停/点击/光追/resize。

---

## 9. 变更日志

| 日期 | 说明 |
|------|------|
| 2026-06-03 | 初版：汇总多线程渲染讨论、目标架构、阶段 0–4 与当前代码进度 |

---

## 10. 参考对话要点

- 颜色拾取与「整帧并行」矛盾：同帧准确拾取需 fence；真并行读另一缓冲可能 1 帧 ID 滞后。
- `RenderThread::sync()` 当前等于等队列清空，不是 GPU fence；长期由 `FrameSync` 替代场景等待。
- 光追：`UploadSceneDataToGPU` / `BuildAccelerationStructures` / `RenderComputeFrame` 均须在渲染线程。
