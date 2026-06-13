# Cable Jam - 充电请排队

一个 Windows 桌面版单机轻休闲解谜游戏。玩家需要观察一团互相阻挡的数据线，找出当前插头朝外且路径畅通的线，把它们按合适顺序抽走，最终清空棋盘。

![](https://github.com/user-attachments/assets/ba0c3cfa-0c66-489a-80b1-ae04d880995e)

仓库地址：https://github.com/sdcb/cable-jam

## 玩法

- 点击数据线任意位置即可尝试抽走它。
- 插头朝向棋盘外侧，并且前方没有其它数据线阻挡时，数据线会被抽走。
- 不能抽走的数据线会轻微抖动并播放阻挡反馈。
- 清空全部数据线即可过关。
- 评分基于失败点击次数：0-1 次为 3 星，2-3 次为 2 星，4-5 次为 1 星，超过 5 次为 0 星。

## 特性

- 81 个自动生成关卡。
- 棋盘尺寸随难度平滑增长，后期最高到 28 x 18。
- 关卡由固定随机种子生成，保证可复现且可解。
- 支持关卡解锁、最佳点击次数、最佳星级记录。
- 内置音效和图标，发布为单个 Windows GUI 程序。
- 基线支持 Windows 8，静态链接 MSVC 运行时。

## 技术栈

- C++ / Win32
- Direct2D / DirectWrite / WIC
- XAudio2.8 程序化音效，抽线音按《American Patrol》旋律逐音推进
- cJSON
- doctest
- CMake / Ninja / MSVC

## 项目结构

```text
.
├── assets/                 # 音效资源
│   └── audio/              # 历史音效脚本和素材
├── icons/                  # 应用图标
├── spec/                   # 游戏规则和规格文档
├── src/
│   ├── app/                # App 主流程、Win32 窗口、DPI、入口 WinMain
│   ├── audio/              # XAudio2 音频引擎和程序化旋律音效
│   ├── core/               # 场景/覆盖层基类、几何、计时、补间
│   ├── game/               # 游戏状态、关卡生成、进度存储
│   ├── graphics/           # Direct2D 渲染上下文
│   ├── resources/          # Windows 资源 ID 和 rc 资源
│   └── ui/                 # 主菜单、关卡、游戏场景和对话框
├── tests/
│   ├── scene_viewer/       # UI 场景查看和截图测试工具
│   └── unit/               # doctest 单元测试
├── CMakeLists.txt
└── CMakePresets.json       # VS2026 release 预设
```

## 构建

先进入 VS2026 x64 开发者命令行。Community、Professional、Enterprise 或 Build Tools 均可，只要环境里有 MSVC、Windows SDK、CMake 和 Ninja。

```powershell
cmake --preset vs2026-release
cmake --build --preset vs2026-release
ctest --preset vs2026-release --output-on-failure
```

生成目标：

- `cable_jam.exe`：正式 Windows GUI 程序。
- `scene_viewer.exe`：测试/调试用场景查看器，可按参数打开指定场景并截图。
- `unit_tests.exe`：游戏逻辑单元测试。

## GitHub Actions

`.github/workflows/build.yml` 会在 Windows VS2026 runner 上编译：

- `win-x64`
- `win-x86`
- `win-arm64`

x64 和 x86 构建会运行测试；arm64 为交叉编译产物，不在 x64 runner 上运行测试。每个架构会上传对应的 `cable_jam.exe` artifact。

## 运行数据

- `appsettings.json` 默认按当前工作目录读写。
- 过关记录包括已解锁关卡、最佳点击次数和最佳星级。
- 图标通过 Windows `.rc` 嵌入程序，音效在运行时生成。

## License

MIT License. See [LICENSE](LICENSE).
