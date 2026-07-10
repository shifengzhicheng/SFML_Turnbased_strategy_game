# Command Lines

一个使用 SFML 2.x 实现的实时 rogue RTS 自走棋游戏。玩家升级单一 CMD 经济、选择兵线并排队生产单位；工蜂、寻路、战斗和基地附近建筑摆放会自动执行。原工程是 Visual Studio/Win32 项目；当前仓库已整理为跨平台 CMake 工程，并通过 `third_party/SFML` submodule 从 SFML 源码编译依赖。

## 获取代码

```bash
git clone --recurse-submodules <repo-url>
```

如果已经 clone 过仓库：

```bash
git submodule update --init --recursive
```

## 编译

macOS / Linux / Ninja 等单配置生成器：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Windows / Visual Studio 等多配置生成器：

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## 运行

macOS / Linux：

```bash
./build/sfml_tbs
```

Windows 多配置生成器通常在：

```powershell
.\build\Release\sfml_tbs.exe
```

构建时会把 `data/` 复制到可执行文件目录，因此也可以从其他工作目录直接运行对应的可执行文件路径。

## 测试

当前测试覆盖 A*、异步寻路交接、随机地图约束、操作规则、兵种定义和窗口坐标映射：

```bash
ctest --test-dir build --output-on-failure
```

也可以运行无窗口脚本模拟来检查节奏：

```bash
./build/sfml_tbs --simulate-player 800
./build/sfml_tbs --simulate-plan balanced 800
./build/sfml_tbs --simulate-plan rush 800
./build/sfml_tbs --simulate-plan greedy 800
./build/sfml_tbs --simulate-plan balanced 900 --simulate-dt 0.25
./build/sfml_tbs --train-policies 40 420 20260616 --simulate-dt 0.25
./build/sfml_tbs --simulate-ignore-gameover 900
```

`--simulate-player` 默认使用 `balanced` 操作队列。脚本和 AI 都通过同一个
`GameOperation` 入口执行“选线、升经济、升科技、造兵营/塔、排兵”等操作，
便于比较不同策略是否真的容易上手、节奏是否合理。模拟不会 sleep 等真实时间；
`--simulate-dt` 可以把每 tick 推进到最多 `0.25`，用来快速跑长局且避免跳过移动节奏。
`--train-policies` 会让两个从随机权重开始的轻量策略模型自对弈，使用胜负/基地血量/军队/经济科技差更新操作偏好。
正式 AI 已切到同一套 `GameOperation` 策略模型：先评估合法操作，再结合经济/科技节奏、兵线压力和反塔需求执行短操作队列。
如果需要复现实验地图，可以设置 `TBS_MAP_SEED=123` 后再运行模拟。

## 当前玩法

- 资源只有一种：`CMD`。CMD 来自自然增长和击杀赏金，击杀赏金按敌方单位造价比例返还。
- `ECONOMY` 按钮会提高自然 CMD 增长，并增加基地附近可见工蜂数量；地图水晶只是视觉锚点，不需要点击或占领。
- `UPGRADE` 提升科技等级，并触发三选一 rogue 战术强化；双方基地状态会显示已选强化。
- 兵营和防御塔由按钮自动放在基地/兵线附近，玩家只需要选 Top/Mid/Bot 后造兵。
- 生产建筑被拆会触发补救：返还部分 CMD、临时 HQ 护盾和少量基地修复，降低一波崩盘的挫败感。
- 兵种移动速度差异明显：骑兵最快适合绕线/切后排，攻城车最慢但射程略远于防御塔，需要前排保护才能稳定拆塔。
- 7 分 30 秒后进入可见加时，建筑伤害和 HQ 压力逐步提高；14 分钟仍未结束时触发双方对称的 `FINAL`，所有兵线越过外围目标直攻 HQ，避免僵局拖过 15 分钟。
- AI 使用策略模型做正式决策，会在开局兵营、经济/科技、兵种混编、防守塔和攻城反塔之间按局势打分切换。
- 开始界面只保留 `START` / `HELP` 两个入口；结算界面只保留 `PLAY AGAIN`，减少无关按钮干扰。

## 项目结构

- `src/`：游戏源码
- `tests/`：确定性寻路、地图、操作规则、数值定义和响应式坐标回归测试
- `data/`：运行资源；当前只保留必需字体 `data/ttf/arial.ttf`
- `tools/`：平衡实验、地图/单位/侧栏预览以及完整前端离屏截图工具
- `third_party/SFML/`：SFML 2.6.2 submodule，跨平台从源码编译
- `CMakeLists.txt`：跨平台构建入口

## 开发预览

美术由 SFML 程序化生成，预览目标不会进入默认构建。需要检查实际像素时运行：

```bash
cmake --build build --target sfml_tbs_art_preview sfml_tbs_map_preview sfml_tbs_sidebar_preview
cmake --build build --target sfml_tbs_gameplay_preview sfml_tbs_frontend_preview
./build/sfml_tbs_art_preview build/art_preview.png
./build/sfml_tbs_map_preview build/map_preview.png
./build/sfml_tbs_sidebar_preview build/sidebar_preview.png
./build/sfml_tbs_gameplay_preview build/gameplay_preview.png
./build/sfml_tbs_frontend_preview build
```

窗口可以自由缩放；渲染和输入共享固定逻辑画布并使用 letterbox 映射，因此格子比例和按钮命中不会随窗口尺寸变化。

## 说明

原仓库中的 Visual Studio 工程文件、Windows `.exe/.dll/.lib/.pdb/.obj` 产物、重复 Debug/Release 资源目录已清理。当前构建不依赖系统安装的 SFML，也不保留旧 Windows 二进制兜底。

`third_party/SFML` 保持官方 submodule 内容，当前锁定在 SFML `2.6.2`。项目自身只通过 CMake 链接 `sfml-graphics`、`sfml-window`、`sfml-system`，未启用 SFML audio/network。

`.github/workflows/build.yml` 会在 macOS、Linux 和 Windows 上从该 submodule 构建并运行同一套测试；Linux CI 通过 `xvfb` 提供 SFML 图形上下文。
