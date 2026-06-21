# 虚幻引擎多人射击游戏核心架构与系统设计

## 📖 项目概述
本项目基于 UE5 官方 First Person 模板进行底层重构与扩展。系统采用 **Listen Server (监听服务器)** 模式构建 Client-Server 网络架构，深度集成 **Gameplay Ability System (GAS)**，实现了一个包含战术 AI、数据驱动武器库、无缝局域网联机及多维胜负判定机制的完整多人射击竞技 Demo。


## ⚙️ 核心系统架构

### 1. 底层网络与会话管理 (Networking & Session)
本项目从底层构建了标准的局域网联机闭环，确保多人对战的无缝接入与公平性。
* **局域网大厅机制**：通过 `UArenaGameInstance` 接入在线子系统 (`IOnlineSubsystem`)。
* **公开会话**：房主调用 `CreateSession` 建立最大4人的公开连接会话，并以 Listen Server 模式载入地图。
* **无缝漫游 (Client Travel)**：客户端利用 `FOnlineSessionSearch` 动态检索子网内的活跃主机，匹配成功后通过 `JoinSession` 实现无缝直连。
* **权限仲裁 (Server Authoritative)**：将高敏感操作 (如伤害结算、弹药扣除、武器拾取) 收归服务器。
* **反作弊机制**：客户端请求均通过 Server RPC (如 `Server_Fire`) 上报，从物理底层阻断客户端内存篡改等作弊途径。

### 2. 对局仲裁与状态机 (GameMode & GameState)
游戏的生命周期与胜负判定由宏观控制器严格把控。
* **规则与计分双轨制**：`AArenaShooterGameMode` 掌管400分限时击杀目标。
* **多目标玩法**：玩家进入撤离区 (`AArenaExtractionZone`) 触发重叠判定 (Overlap) 后，系统自动补齐积分并宣告特殊胜利。
* **全员存活状态巡检**：为防止游戏进入死锁，系统在玩家死亡回调中遍历全服 `PlayerController`。
* **自动失败判定**：若判定无存活且具备输入能力 (`InputEnabled`) 的玩家，则直接触发失败 (`OnDefeat`)。
* **全网状态广播**：`AArenaShooterGameState` 集中管理全局分数、时间与任务目标，通过 `RepNotify` 机制将关键数据的变化精准广播至所有客户端 UI。

### 3. 核心战斗能力系统 (GAS Integration)
玩家角色的战斗交互深度融合了 GAS 架构，实现了极高的扩展性与优秀的网络手感。
* **客户端本地预测 (Client-side Prediction)**：为消除射击延迟，客户端开火时通过 `FScopedPredictionWindow` 生成 PredictionKey，在本地无延迟应用弹药扣除的 GameplayEffect，掩盖网络延迟 (Latency)，实现“零延迟”开火手感。
* **原生伤害无缝桥接 GAS**：重写原生的 `TakeDamage` 方法，动态生成瞬时型 (Instant) Gameplay Effect，将环境伤害无缝转入 GAS 系统底层的真实血量扣除中。

### 4. 数据驱动实体系统 (Data-Driven Entities)
摒弃复杂的代码派生，采用高度数据驱动的设计模式管理武器与弹道实体。
* **参数化武器库**：通过定义 `FWeaponStats` 结构体，将射速、后坐力偏转、弹丸数量及散布角度暴露至蓝图。
* **手感模拟**：仅需调整数值，即可依托底层的锥形射线逻辑模拟出突击步枪、霰弹枪等不同手感。
* **物资争夺与状态继承**：实现了动态的2把武器库存系统。
* **物理连续性**：当玩家拾取新枪掉落旧枪 (`DropCurrentWeapon`) 时，服务器会精准截取 GAS 中当前的剩余弹药数据，并将其作为初始参数注入新生成的掉落物实体，保证战场资源的真实物理连续性。
* **硬核无重力弹道**：弹药实体 (`AArenaShooterProjectile`) 采用 $10000.f$ 的极高初始速度，并将重力缩放与弹跳设为 $0.0f$，还原电竞级高速直线弹道。

### 5. 战术 AI 与感知体系 (AI & Perception)
敌人的设计不仅满足 PVE 交互，更着重优化了网络效能与战术压迫感。
* **感知体系**：AI 控制器配备 `AIPerceptionComponent` (视觉半径 1500，听觉半径 3000)。
* **群狼战术与感知共享**：当单个 AI 确认目标后，立刻调用 `AlertNearbyEnemies` 遍历半径 2000 内的友军，并强制向其 Blackboard (黑板) 注入玩家坐标，触发群体追击。
* **状态双重保险**：AI 在执行攻击 (`PerformAttack`) 前，会校验玩家是否附带 `Dead` 标签。
* **算力节约**：若目标已死亡，主动清空黑板变量终止攻击，杜绝算力浪费。

### 6. 表现层与网络解耦 (Decoupled Presentation)
为保障多人在高频交火下的帧率稳定性，系统将“逻辑仲裁”与“视听表现”进行了严格的架构解耦。
* **事件驱动 UI 同步**：废弃 Tick 刷新机制，3D血条等 UI 渲染绑定于 `OnRep_Health` 等网络回调。
* **无损更新**：当服务器数据改变时，利用 `BlueprintImplementableEvent` 通知前端无损更新。
* **多播渲染剥离**：枪口火焰、子弹轨迹与受击音效严格剥离出伤害计算管线。
* **极限带宽优化**：服务器验证合法开火后，仅调用非可靠多播 (Unreliable NetMulticast)。
* **本地视觉渲染**：各客户端收到指令后自行调用 `UGameplayStatics::SpawnEmitterAtLocation` 进行本地视觉渲染，将带宽开销降至最低。

### 7. 引擎端蓝图与资产配置展示 (Blueprint Configurations)
为确保代码端逻辑能够在引擎表现层被准确触发与呈现，开发阶段的引擎端配置需严格对齐以下标准。
* **战术行为树与后台决策服务**：展示 AI 逻辑编排的解耦。由 C++ 自定义编写的 `BTS_UpdateDistance` 距离测算服务节点，正在后台静默驱动战术决策。
* **高可用 UI 防错蓝图连线**：在读取玩家体力/血量渲染进度条前，强制执行 `IsValid` 空指针检测拦截未加载实体，并利用 `Max(1.0)` 节点在除法底层杜绝 Divide by zero 崩溃风险。
* **数据驱动武器实例化展示**：体现基于 `FWeaponStats` 的数据驱动优势。以霰弹枪 (Shotgun) 为例，无需编写多余逻辑，仅在配置面板中修改每次发射弹丸数 (`BulletsPerShot = 8`) 与散布角度 (`SpreadAngle = 8.0`)，即可配合底层 for 循环与锥形射线算法瞬间生成极具破坏力的近战武器。
* **增强输入映射上下文 (Enhanced Input)**：自定义的 `IA_ToggleSmartFire` (智能辅助开火切换) 和 `IA_SwitchWeapon` (换枪) 等高级动作已解耦并注册在 `IMC_Default` 资产中，实现无感知的硬件支持。
