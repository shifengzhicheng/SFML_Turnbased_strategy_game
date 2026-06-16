# SFML Turnbased Strategy Game

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

当前包含 A* 和随机地图约束测试：

```bash
ctest --test-dir build --output-on-failure
```

也可以运行无窗口脚本模拟来检查节奏：

```bash
./build/sfml_tbs --simulate-player 800
./build/sfml_tbs --simulate-ignore-gameover 900
```

## 当前玩法

- 资源只有一种：`CMD`。CMD 来自自然增长和击杀赏金，击杀赏金按敌方单位造价比例返还。
- `ECONOMY` 按钮会提高自然 CMD 增长，并增加基地附近可见工蜂数量；地图上的 CMD 标记只是读图锚点，不再需要占矿或抢矿。
- `UPGRADE` 提升科技等级，并触发三选一 rogue 战术强化；双方基地状态会显示已选强化。
- 兵营和防御塔由按钮自动放在基地/兵线附近，玩家只需要选 Top/Mid/Bot 后造兵。
- 攻城车射程略远于防御塔，防御塔成群时需要用攻城车逼塔、再用部队保护攻城车。
- AI 使用开局、宏运营、防守和反塔攻城几套内部策略，会根据玩家兵种、塔/兵营数量和基地压力切换。

## 项目结构

- `src/`：游戏源码
- `tests/`：基础逻辑测试，覆盖 A* 寻路边界和随机地图连通性
- `data/`：运行所需字体和 SVG 美术源稿；单位、按钮、背景等贴图在运行时由 SFML 矢量图元生成
- `third_party/SFML/`：SFML 2.6.2 submodule，跨平台从源码编译
- `CMakeLists.txt`：跨平台构建入口

## 说明

原仓库中的 Visual Studio 工程文件、Windows `.exe/.dll/.lib/.pdb/.obj` 产物、重复 Debug/Release 资源目录已清理。当前构建不依赖系统安装的 SFML，也不保留旧 Windows 二进制兜底。

`third_party/SFML` 保持官方 submodule 内容，当前锁定在 SFML `2.6.2`。项目自身只通过 CMake 链接 `sfml-graphics`、`sfml-window`、`sfml-system`，未启用 SFML audio/network。
