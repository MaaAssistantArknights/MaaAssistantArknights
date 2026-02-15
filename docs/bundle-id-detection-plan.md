# Bundle ID 检测方案 - 替代分辨率检测

## 核心思路

使用 WDA 的 `mobile: activeAppInfo` API 查询当前前台应用的 Bundle ID，直接判断游戏是否已启动并在前台，而不是通过检测屏幕分辨率变化。

## 优势分析

### 相比分辨率检测的优点

1. **更直接准确**
   - 直接判断目标应用是否在前台
   - 不受屏幕方向影响
   - 不依赖截图操作

2. **更快速**
   - 轻量级 API 调用（< 100ms）
   - 无需等待屏幕旋转
   - 通常 1-3 秒内完成检测

3. **更可靠**
   - 不受分辨率变化影响
   - 不受 WDA 缓存影响
   - 避免了方向抖动问题

4. **逻辑更清晰**
   - 启动游戏 → 检查游戏是否在前台 → 完成初始化
   - 不需要关心中间的屏幕旋转过程

5. **代码更简单**
   - 代码量减少约 50%
   - 不需要复杂的状态机
   - 不需要处理并发初始化问题

## WDA API 支持

### activeAppInfo 方法

**HTTP 请求：**
```http
POST /session/{sessionId}/execute/sync
Content-Type: application/json

{
  "script": "mobile: activeAppInfo",
  "args": []
}
```

**响应格式：**
```json
{
  "value": {
    "bundleId": "com.hypergryph.arknights",
    "pid": 1234,
    "name": "Arknights",
    "processArguments": {
      "args": [],
      "env": {}
    }
  }
}
```

**返回字段：**
- `bundleId`: 前台应用的 Bundle ID（关键字段）
- `pid`: 进程 ID
- `name`: 应用名称
- `processArguments`: 进程参数

## 实现方案

### Phase 1: WDAController 添加查询前台应用方法

**File**: `src/MaaCore/Controller/WDAController.h`

```cpp
public:
    // 查询当前前台应用的 Bundle ID
    std::optional<std::string> get_active_bundle_id() const;

private:
    mutable std::mutex m_http_mutex;  // 保护 HTTP 通道（已在 v3 中添加）
```

**File**: `src/MaaCore/Controller/WDAController.cpp`

```cpp
std::optional<std::string> asst::WDAController::get_active_bundle_id() const
{
    LogTraceFunction;

    // 线程安全：加锁保护 HTTP 请求
    std::lock_guard<std::mutex> lock(m_http_mutex);

    try {
        // 使用 WDA 的 execute script API 调用 mobile: activeAppInfo
        json::object execute_params;
        execute_params["script"] = "mobile: activeAppInfo";
        execute_params["args"] = json::array {};

        auto response = const_cast<WDAController*>(this)->http_post(
            "/session/" + m_session_id + "/execute/sync",
            execute_params.to_string()
        );

        if (!response.success()) {
            Log.warn("Failed to get active app info, status:", response.status_code);
            return std::nullopt;
        }

        auto json_opt = json::parse(response.body);
        if (!json_opt) {
            Log.warn("Failed to parse active app info response");
            return std::nullopt;
        }

        // 响应格式: {"value": {"bundleId": "...", "pid": 1234, ...}}
        if (json_opt->contains("value") && json_opt->at("value").is_object()) {
            const auto& value = json_opt->at("value");
            if (value.contains("bundleId") && value.at("bundleId").is_string()) {
                std::string bundle_id = value.at("bundleId").as_string();
                Log.trace("Active bundle ID:", bundle_id);
                return bundle_id;
            }
        }

        Log.warn("bundleId not found in response");
        return std::nullopt;
    }
    catch (const std::exception& e) {
        Log.error("Exception in get_active_bundle_id:", e.what());
        return std::nullopt;
    }
}
```

### Phase 2: 改进 StartGameTaskPlugin

**File**: `src/MaaCore/Task/Miscellaneous/StartGameTaskPlugin.cpp`

```cpp
bool StartGameTaskPlugin::_run()
{
    if (m_client_type.empty()) {
        return false;
    }

    // check for MAC / iOS
    if (ctrler()->get_controller_type() == ControllerType::MacPlayTools ||
        ctrler()->get_controller_type() == ControllerType::WDA) {

        // 获取目标游戏的 Bundle ID
        auto target_bundle_id = Config.get_package_name(m_client_type);
        if (!target_bundle_id) {
            Log.error("Failed to get bundle ID for client_type:", m_client_type);
            return false;
        }

        Log.info("Starting game with bundle ID:", target_bundle_id.value());

        bool start_result = ctrler()->start_game(m_client_type);
        if (!start_result) {
            return false;
        }

        // For WDA with deferred init, wait for game to be in foreground
        if (ctrler()->get_controller_type() == ControllerType::WDA &&
            !ctrler()->is_fully_initialized()) {

            Log.info("Game start command sent, waiting for app to be in foreground...");

            constexpr int max_attempts = 30;      // 30 次
            constexpr int poll_interval_ms = 500; // 每次 500ms
            constexpr int stable_count = 3;       // 需要连续 3 次检测到游戏在前台

            int consecutive_success = 0;
            bool game_in_foreground = false;

            for (int i = 0; i < max_attempts; ++i) {
                // 检查是否需要退出
                if (need_exit()) {
                    Log.info("Task exit requested during game launch wait");
                    return false;
                }

                // 查询当前前台应用的 Bundle ID
                auto active_bundle = ctrler()->get_active_bundle_id();

                if (active_bundle && active_bundle.value() == target_bundle_id.value()) {
                    consecutive_success++;
                    Log.trace("Game is in foreground (stable {}/{})",
                             consecutive_success, stable_count);

                    if (consecutive_success >= stable_count) {
                        Log.info("Game confirmed in foreground: {}", target_bundle_id.value());
                        game_in_foreground = true;
                        break;
                    }
                }
                else {
                    // 重置计数器（防止应用切换抖动）
                    if (consecutive_success > 0) {
                        Log.trace("App switched, resetting counter. Current: {}",
                                 active_bundle.value_or("unknown"));
                        consecutive_success = 0;
                    }

                    Log.trace("Waiting for game to be in foreground (attempt {}/{}), current: {}",
                             i + 1, max_attempts, active_bundle.value_or("unknown"));
                }

                sleep(poll_interval_ms);
            }

            if (!game_in_foreground) {
                Log.error("Timeout waiting for game to be in foreground after {} seconds",
                         max_attempts * poll_interval_ms / 1000);
                // 仍然尝试完成初始化，让分辨率检查决定是否失败
            }

            // 游戏在前台后，额外等待一小段时间让界面稳定
            Log.info("Game in foreground, waiting for UI to stabilize...");
            sleep(1000);  // 等待 1 秒让游戏界面完全加载和旋转

            // 完成延迟初始化（此时会进行分辨率检查）
            if (!ctrler()->complete_init()) {
                Log.error("Failed to complete deferred initialization");
                return false;
            }

            Log.info("WDA initialization completed successfully");
        }

        return true;
    }

    // ... rest of Android logic unchanged ...
}
```

### Phase 3: Controller 添加接口（简化版）

**File**: `src/MaaCore/Controller/Controller.h`

```cpp
public:
    // 查询当前前台应用的标识符（WDA 返回 Bundle ID）
    std::optional<std::string> get_active_bundle_id() const;
```

**File**: `src/MaaCore/Controller/Controller.cpp`

```cpp
std::optional<std::string> asst::Controller::get_active_bundle_id() const
{
    if (m_controller_type == ControllerType::WDA) {
        auto wda_ctrl = std::dynamic_pointer_cast<WDAController>(m_controller);
        if (wda_ctrl) {
            return wda_ctrl->get_active_bundle_id();
        }
    }
    return std::nullopt;
}
```

## 完整流程

```
用户点击"启动游戏"
  ↓
Controller::connect() with deferred_init=true
  ↓ (跳过截图和分辨率检查)
状态: Connected
  ↓
StartGameTaskPlugin::_run()
  ↓
调用 WDA /wda/apps/launch 启动游戏
  ↓
轮询检测前台应用 (每 500ms)
  ├─ 调用 get_active_bundle_id()
  ├─ 比较 Bundle ID
  └─ 需要连续 3 次匹配
  ↓ (通常 1-3 秒)
检测到目标游戏在前台
  ↓
等待 1 秒让界面稳定（包括屏幕旋转）
  ↓
调用 complete_init()
  ├─ 截图
  ├─ 检查分辨率（此时应该已经是横屏）
  └─ 创建 ControlScaleProxy
  ↓
状态: Initialized
  ↓
MAA 正常运行
```

## 与 v3 方案对比

| 方面 | v3 (分辨率检测) | Bundle ID 检测 |
|------|----------------|---------------|
| **检测目标** | 屏幕分辨率 | 前台应用 Bundle ID |
| **数据源** | `query_window_size()` | `activeAppInfo` API |
| **检测速度** | 慢（等待旋转）| 快（1-3 秒）|
| **可靠性** | 中（受方向影响）| 高（直接判断）|
| **代码复杂度** | 高（~350 行）| 低（~180 行）|
| **状态机** | 5 状态 | 3 状态即可 |
| **并发处理** | 需要 CV + mutex | 简单 mutex 即可 |
| **方向抖动** | 需要处理 | 不受影响 |
| **WDA 缓存** | 需要刷新 | 不受影响 |

## 代码改动量

### 新增代码
- `WDAController::get_active_bundle_id()`: ~50 行
- `Controller::get_active_bundle_id()`: ~10 行
- `StartGameTaskPlugin` 轮询逻辑: ~60 行
- **总计**: ~120 行

### 修改代码
- `Controller::connect()`: ~20 行（deferred init 逻辑）
- `Controller::complete_init()`: ~40 行（简化版，不需要 CV）
- **总计**: ~60 行

### 对比 v3
- v3 新增: ~250 行
- v3 修改: ~150 行
- **Bundle ID 方案减少约 50% 代码量**

## 优势总结

### 1. 更直接的判断逻辑 ✅
- 不需要猜测屏幕是否旋转
- 直接知道游戏是否在前台
- 逻辑清晰易懂

### 2. 更快的响应速度 ✅
- 不需要等待屏幕旋转动画（通常 1-2 秒）
- API 调用延迟低（< 100ms）
- 总体等待时间减少 50%

### 3. 更简单的实现 ✅
- 不需要复杂的 5 状态状态机
- 不需要 condition_variable
- 不需要处理方向抖动
- 不需要刷新 WDA 缓存

### 4. 更可靠的检测 ✅
- 不受屏幕方向影响
- 不受分辨率缓存影响
- Bundle ID 是确定性的字符串比较

### 5. 更好的用户体验 ✅
- 启动更快
- 失败信息更明确（"游戏未启动"vs"分辨率不对"）
- 减少等待时间

## 潜在问题和解决方案

### 问题 1: 游戏在前台但还在加载

**现象**: Bundle ID 检测通过，但游戏界面还在加载中

**解决方案**:
- 检测到游戏在前台后，额外等待 1 秒
- 这 1 秒足够让游戏完成界面加载和屏幕旋转
- 如果仍然不够，`complete_init()` 的分辨率检查会失败

### 问题 2: 游戏启动失败但没有错误

**现象**: 启动命令成功，但游戏实际没有启动

**解决方案**:
- 15 秒超时机制
- 超时后仍然尝试 `complete_init()`
- 让分辨率检查作为最终验证

### 问题 3: 用户手动切换应用

**现象**: 游戏启动后，用户切换到其他应用

**解决方案**:
- 连续 3 次检测机制
- 如果中途切换应用，计数器重置
- 必须连续 3 次（1.5 秒）检测到游戏才认定成功

### 问题 4: Bundle ID 不匹配

**现象**: 配置的 Bundle ID 与实际不符

**解决方案**:
- 记录详细日志，显示当前前台应用
- 用户可以根据日志修正配置
- 超时后仍然尝试初始化，让用户看到错误信息

## 测试用例

### Test Case 1: 正常流程
1. iPhone 主屏幕（竖屏）
2. 连接 WDA with deferred_init
3. 启动游戏
4. 1 秒后检测到游戏在前台（连续 3 次）
5. 等待 1 秒
6. complete_init() 成功（横屏 2436x1125）
7. MAA 正常运行

### Test Case 2: 游戏启动慢
1. 启动游戏
2. 5 秒后才检测到游戏在前台
3. 等待 1 秒
4. complete_init() 成功
5. 总耗时约 6 秒（仍在 15 秒超时内）

### Test Case 3: 用户切换应用
1. 启动游戏
2. 检测到游戏在前台 1 次
3. 用户切换到设置应用
4. 计数器重置
5. 用户切回游戏
6. 重新开始计数，连续 3 次后成功

### Test Case 4: 游戏启动失败
1. 启动游戏命令发送
2. 游戏实际没有启动（崩溃或其他原因）
3. 15 秒超时
4. 仍然尝试 complete_init()
5. 分辨率检查失败（仍然是主屏幕）
6. 返回错误，任务失败

### Test Case 5: Bundle ID 配置错误
1. 配置的 Bundle ID 不正确
2. 游戏启动成功，但 Bundle ID 不匹配
3. 15 秒超时
4. 日志显示当前前台应用的 Bundle ID
5. 用户可以根据日志修正配置

## 风险评估

| 风险 | 严重性 | 概率 | 缓解措施 |
|------|--------|------|----------|
| API 调用失败 | 中 | 低 | 返回 nullopt，继续轮询 |
| Bundle ID 不匹配 | 低 | 低 | 详细日志，用户可修正 |
| 游戏加载慢 | 低 | 中 | 额外等待 1 秒 |
| 用户切换应用 | 低 | 低 | 连续 3 次检测 |
| 超时 | 低 | 低 | 15 秒足够长 |

## 实现优先级

1. **P0**: Phase 1 (WDAController 添加 `get_active_bundle_id()`)
2. **P0**: Phase 2 (StartGameTaskPlugin 轮询逻辑)
3. **P1**: Phase 3 (Controller 接口封装)
4. **P2**: 完善错误处理和日志

## 结论

**Bundle ID 检测方案明显优于分辨率检测方案**：

1. ✅ **更快**: 1-3 秒 vs 3-15 秒
2. ✅ **更简单**: 180 行 vs 350 行
3. ✅ **更可靠**: 不受方向、缓存影响
4. ✅ **更直接**: 判断游戏是否在前台，而不是猜测
5. ✅ **更易维护**: 逻辑清晰，状态简单

**推荐立即采用此方案**，替代 v3 的复杂分辨率检测方案。
