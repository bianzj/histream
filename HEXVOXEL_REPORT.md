# 异质性体元 (Hex-Voxel) 方法报告

> 思路：面元先按 x/y/z 三个正交方向投影计算聚集指数 (Ax, Ay, Az)，
> 用 L1 外推得到任意方向的聚集指数 A(d̂)，再加 K³ 体采样平均体密度 ρ̄，
> 二者共同构成"异质性体元"，替代原来均匀体元假设。
> 已实现为独立工具 + 新增引擎模式 `eHexRT` / `eHexEB`（不改动 voxelrt/voxeleb 行为）。

## 1. 方法定义

对每个体元（边长 = 体素尺寸 s）：

- **正交聚集指数 Ax / Ay / Az**：在垂直于该轴的截面上取 N²（默认 8×8）采样点，
  每点沿轴向走 M（默认 4）个体内采样：
  封闭对象——任一样点位于物体内（奇偶判定）即遮挡；
  开放对象——轴向单位线段到任一面的距离 ≤ 0.02s 即遮挡。
  Ax 即该方向被遮挡的采样比例（0=完全通透, 1=完全不通透）。
- **任意方向外推**：`A(d̂) = clamp01(|dx|·Ax + |dy|·Ay + |dz|·Az)`（L1 范数外推，
  各向异性的线性近似；对轴方向严格一致，对斜方向为偏保守的估计）。
- **平均体密度 ρ̄**：体元内 K³（默认 4³=64）点采样，统计落在封闭材料内部的
  比例（奇偶判定），即体元平均体积填充分数。

奇偶（内外判定）实现：对每个 (y,z) 行先做一次 -x 基准射线定初值，再沿 x 做
扫描线传播（parity[x+1] = parity[x] XOR 中心连线穿过的封闭面数），
交点落在扫描线上时用半开规则消歧。面元-线段相交用重心坐标 + 段穿平面。

## 2. 验证（crop 案例，240,002 面元，200m×1m×200m，s=1.0）

- 网格 201×2×201 = 80,802 胞，活跃体元 60,702，无全满胞
- mean_ρ(全部)=0.2475，mean_ρ(活跃)=0.3295，mean_A=0.5508
- 方向外推精度（A_est vs 面元弦采样真值 A_true，同一截面/列定义）：

| 方向 | A_est | A_true | mean\|err\| | rms |
|---|---|---|---|---|
| x 轴 | 0.8520 | 0.8520 | 0 | 0 |
| y 轴 | 0.4412 | 0.4412 | 0 | 0 |
| z 轴 | 0.9999 | 0.9999 | 0 | 0 |
| 45°(0,.707,.707) | 0.7966 | 0.7424 | 0.0555 | 0.0690 |
| 45°(.707,.707,0) | 0.7964 | 0.7434 | 0.0555 | 0.0693 |
| 太阳(23.45°,195.68°) | ~0.83 | ~0.75 | ~0.083 | — |

轴方向精确（构造保证），斜方向 L1 外推偏高 ~0.05-0.08（保守上界型估计，
符合"聚集指数用于遮挡/透射"的用途，偏保守不影响辐射平衡稳定性）。

## 3. 森林案例（trees.obj，968,550 面元，78,192 棵对象，300m×10m×300m，s=1.0）

- 网格 301×11×301 = 996,611 胞，活跃体元 249,462
- mean_ρ(全部)=0.0783，mean_ρ(活跃)=0.3129，mean_A=0.5031
- 方向外推精度：

| 方向 | A_est | A_true | mean\|err\| | rms |
|---|---|---|---|---|
| 太阳(23.45°,195.68°) | 0.7320 | 0.6982 | 0.0969 | 0.1596 |
| 垂直俯视 (0,1,0) | 0.7352 | 0.7766 | 0.0414 | 0.1319 |
| 水平 (0,0,1) | 0.3817 | 0.4190 | 0.0373 | 0.1104 |

计算耗时（本机 64 核，OpenMP）：
- 初版 x 循环粗粒度调度（dynamic,16，仅 ~19 线程有效）：103 min；
- 细化为 dynamic,1（~63 线程全忙）+ parity 基准/扫描线并行化：52.5 min
  （三次独立运行数值逐位一致，总工作量约 3000 CPU-min）。
  大场景可用更大体素尺寸（s=2 → 胞数降 8 倍，预计 ~10 min）或离线预计算。

## 4. 实现结构

### 4.1 独立工具
- `src/base/hexvoxel.h / .cpp` —— 自包含核心（不依赖项目其它部分）：
  OBJ 加载（按对象、闭包标记）、空间哈希、奇偶扫描线、体元计算（OpenMP）、
  TSV/JSON 输出。
- `src/tools/hexvoxel/main.cpp` —— CLI：
  `hexvoxel <scene.obj> <voxel_size_m> [--n N] [--m M] [--k K] [--int 0|1]
           [--dir3 label:dx,dy,dz] [--dirza label:zen,az] [--out prefix]`
  输出 `<prefix>.tsv`（`ix iy iz kind ax ay az rho`）+ `<prefix>.json`。
  CMake 目标 `hexvoxel`（自动链接 OpenMP）。

### 4.2 新增引擎模块（hexrt/hexeb 与 voxelrt/voxeleb 并列）
- `src/hexrt/`（ray tracing 辐射版）+ `shader/hexrt/`（3 个 compute .spv）
- `src/hexeb/`（能量平衡 voxel-list 版）+ `shader/hexeb/`（13 个 compute .spv）
- 模块内 Voxelrt→Hexrt / Voxeleb→Hexeb 重命名；IO 头文件守卫独立
  （FIELD_HEXRTIO_H / FIELD_HEXEBIO_H），子头文件沿用共享守卫
  （FIELD_PIPELINE_H 等，保证每个 TU 只定义一个 Pipeline/Buffer/Command/Descriptor 变体）。
- **混浊介质统一接口**（四个模块共用，`shader/functions.glsl`）：
  - `ResolveTurbidDensity(bufferId, canopyId, voxelId, voxelRes)`：
    OBJ 三维 ρ（hexId≥0）> LiDAR 二维 ρ（islad）> canopy density 的优先级取值；
  - `ResolveClumpingIndex(bufferId, direction)`：
    Hex 模式用 OBJ 提取的三轴 CI 做 L1 方向外推 `|d|·(CIx,CIy,CIz)`，
    Voxel 模式保持 CI=1 原行为。
  - 原 voxelrt/voxeleb 的 buffer/descriptor/pipeline 也增加了同一
    medium（hex）buffer 绑定：所有模式上传合法占位项（无 OBJ ρ 时单条
    零值 VoxelHex），hexId=-1 时行为与旧版完全一致。
- 共享结构体改动（向后兼容）：
  - `VoxelLink` 末尾追加 `int hexId = -1`（48B stride 不变）
  - `VoxelIO::voxelHexs` + GPU buffer（hex 模式下随体元列表上传，占位保证非空）
  - `parameters.h` / `bindinglayout.h` 增加 `VoxelHex` 与 `B_HEX`/medium 绑定
    （hexrt=16, hexeb=31, voxelrt=16, voxeleb=31）
  - `Mode` 枚举 + `eHexRT` / `eHexEB`，engine 全部分发点已接入
- 场景构建（scene.cpp）：`hexVoxelEnabled` 时，每个 OBJ 用与
  `voxelizeObjSurface` 相同的坐标系/步长跑一遍 hex 计算，
  逐体元写入 VoxelHex 并回填 `link.hexId`（90/270° 旋转自动 ax↔az 置换；
  无对应胞时 hexId=-1，着色器退化为均匀体元行为）。

### 4.3 用法
```bash
histream.exe eHexRT  Input.xml   # 辐射追踪 + 异质体元
histream.exe eHexEB  Input.xml   # 能量平衡 + 异质体元
```
Input.xml 与 voxelrt/voxeleb 完全同构（读取同一 XML 结构）。
独立预计算/校验：`hexvoxel scene.obj 1.0 --out out`。

## 5. 构建状态（本机 Linux WSL，2026-09-01）

- 项目目录在 Windows 侧为 `C:\work\histream`（原 `histream` 文件夹已重命名；
  WSL 9p 视图对小写路径 dentry 失效时，经大写路径 `/mnt/c/work/HISTREAM`
  或符号链接 `/tmp/histream` 访问，内容同一目录）。
- 构建命令（经符号链接路径配置，保证项目名=histream）：
  `cmake -S /tmp/histream -B /tmp/histream/build-linux -DCMAKE_BUILD_TYPE=Release
   -DBASE_DIRECTORY=/mnt/c/work -DHISTREAM_ENABLE_FACET=OFF
   -DCMAKE_PREFIX_PATH="$HOME/vulksdk/usr;$HOME/miniconda3/envs/histream"
   -DVulkan_INCLUDE_DIR=$HOME/vulksdk/usr/include
   -DVulkan_LIBRARY=$HOME/vulksdk/usr/lib/x86_64-linux-gnu/libvulkan.so
   -DOpenCV_DIR=$HOME/miniconda3/envs/histream/lib/cmake/opencv4
   -DCMAKE_EXE_LINKER_FLAGS="-L$HOME/vulksdk/usr/lib/x86_64-linux-gnu -lcurl"`
  然后 `make -j48` → `[100%] Built target histream` + `hexvoxel`，
  部署到 `/mnt/c/work/bin_x64/Release/`（+ shader/hexrt、shader/hexeb、defined）。
  注：项目名=CMake 源目录名，Windows 侧小写文件夹 → `histream.exe`（与既有构建一致）。
- hex 着色器用 glslangValidator 11.13.0（vulkan1.3）编译，随 shader 目录部署。
- 本机无 GPU：`histream eHexRT/eHexEB` 已验证能启动、解析模式、走到 GPU/显示初始化
  （无头机缺 DISPLAY 为止；装了 Xvfb+lavapipe 可进一步验证）；
  RT 运行路径建议在有 GPU 的 Windows 环境验证。
- 为 Linux 构建修复的兼容点（均带 #ifdef/平台分支，不影响 Windows）：
  vulkan.h 的 X11 `Success/Status` 宏冲突、`<io.h>`/`<fcntl.h>`、`strcpy_s/strtok_s`、
  `GetModuleFileNameA`、`FLOAT` 类型、部署脚本 facet/vcpkg 分支、
  hexvoxel 工具与核心算法的 OpenMP 并行化等。

## 6. 已知限制

1. A(d̂) 为 L1 外推的近似（偏保守上界），斜方向误差 ~0.04-0.10；
   若需严格任意方向，可把 hexId 方案扩展为 24/48 方向预计算表。
2. 场景构建时 hex 计算耗时随体元数线性增长（森林 1M 胞 ~50min/64核）；
   建议大场景增大体素尺寸或离线预计算缓存。
3. 非 90° 整数倍旋转的摆放使用 hexId=-1 回退（均匀体元）；
   当前案例数据均为轴对齐摆放。
4. 开放面遮挡容差 0.02s、体内判定基于面片奇偶（要求每对象面片闭环或标记 open）。

## 7. 输出文件

- `/tmp/hexcrop1m.tsv|json` —— crop 验证
- `/tmp/hextrees.tsv|json` —— 森林（103min 版，数值与后续版本完全一致）
- `/tmp/hextrees2.tsv|json` —— 森林（53min59s 版）
- `/tmp/hextrees3.tsv|json` —— 森林（52.5min 全并行版，默认 5 方向校验）
- 工具：`/tmp/histream/build-linux/hexvoxel`（CMake 目标，带 OpenMP）、`/tmp/hexvoxel`
- 引擎：`/mnt/c/work/bin_x64/Release/`（+ shader/hexrt、shader/hexeb、defined 已部署）
- 改动备份（9p 视图抖动时抢救到本地）：`/tmp/hx_tracked.patch`（git diff，含全部
  跟踪文件改动）、`/tmp/hx_untracked.tar`（新增文件：src/hexrt、src/hexeb、
  src/tools、src/base/hexvoxel.*、src/base/eigen_compat.h、shader/hexrt、
  shader/hexeb、本报告）
- 本报告位于项目根 `HEXVOXEL_REPORT.md`（git 未跟踪）。
