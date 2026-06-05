# SFML Turnbased Strategy Game

一个使用 SFML 2.x 实现的回合制策略游戏。原工程是 Visual Studio/Win32 项目；当前仓库已整理为跨平台 CMake 工程，并通过 `third_party/SFML` submodule 从 SFML 源码编译依赖。

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

当前包含基础寻路单元测试：

```bash
ctest --test-dir build --output-on-failure
```

## 项目结构

- `src/`：游戏源码
- `tests/`：基础逻辑测试，目前覆盖 A* 寻路边界
- `data/`：运行所需字体和 SVG 美术源稿；单位、按钮、背景等贴图在运行时由 SFML 矢量图元生成
- `third_party/SFML/`：SFML 2.6.2 submodule，跨平台从源码编译
- `CMakeLists.txt`：跨平台构建入口

## 说明

原仓库中的 Visual Studio 工程文件、Windows `.exe/.dll/.lib/.pdb/.obj` 产物、重复 Debug/Release 资源目录已清理。当前构建不依赖系统安装的 SFML，也不保留旧 Windows 二进制兜底。

`third_party/SFML` 保持官方 submodule 内容，当前锁定在 SFML `2.6.2`。项目自身只通过 CMake 链接 `sfml-graphics`、`sfml-window`、`sfml-system`，未启用 SFML audio/network。
