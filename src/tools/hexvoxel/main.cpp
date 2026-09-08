//
// hexvoxel —— 异质性体元计算工具 (独立可执行, 无 Vulkan 依赖)
//
// 以面元 (OBJ 三角网格) 为基准, 逐体元计算:
//   * 三正交方向投影聚集指数  Ax Ay Az
//   * 任意方向聚集指数外推    A(d) = |dx|Ax + |dy|Ay + |dz|Az
//   * 体密度 rho  (以及场景平均体密度)
//   * 面元基准真值 A_true(d) 与外推估计的对比 ("面元先计算")
//
// 用法:
//   hexvoxel <scene.obj> <voxel_size_m> [选项]
//     --n 8                 截面采样网格 N (默认 8)
//     --m 4                 列方向采样点 M (默认 4)
//     --k 4                 体积采样 K^3 (默认 4)
//     --int 1               激活封闭固体内部 FULL 体元 (默认 1)
//     --dir3 <label>:dx,dy,dz   追加统计方向 (可多次)
//     --dirza <label>:zenith,azimuth
//           遥感角约定: zenith 为与 +Y (天顶) 夹角, azimuth 为自 +Z 向 +X 的方位角
//     --out <prefix>        输出前缀 (默认 <obj名>_hex): 生成 .tsv / .json
//
// 输出:
//   <prefix>.tsv    逐体元 ix iy iz kind ax ay az rho (kind: 1=FULL 2=MIXED)
//   <prefix>.json   场景统计 + 各方向 A_est vs A_true 误差
//   stdout          摘要

#include "src/base/hexvoxel.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <array>
#include <string>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;

struct Options {
    int n{8}, m{4}, k{4};
    bool includeInteriors{true};
    std::vector<std::pair<std::string, std::array<float,3>>> dirs; // label -> 单位方向
    std::string outPrefix;
};

bool parseArgs(int argc, char** argv, Options& opt, std::string& objPath, float& voxelSize, std::string& err)
{
    if (argc < 3) {
        err = "usage: hexvoxel <scene.obj> <voxel_size_m> [options]";
        return false;
    }
    objPath = argv[1];
    char* end = nullptr;
    voxelSize = std::strtof(argv[2], &end);
    if (end == argv[2] || !(voxelSize > 0.0f)) {
        err = "invalid voxel size: " + std::string(argv[2]);
        return false;
    }
    for (int i = 3; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&](const char* what) -> std::string {
            if (i + 1 >= argc) { err = std::string(what) + " needs a value"; throw std::runtime_error(err); }
            return argv[++i];
        };
        if (a == "--n") opt.n = std::atoi(next("--n").c_str());
        else if (a == "--m") opt.m = std::atoi(next("--m").c_str());
        else if (a == "--k") opt.k = std::atoi(next("--k").c_str());
        else if (a == "--int") opt.includeInteriors = std::atoi(next("--int").c_str()) != 0;
        else if (a == "--out") opt.outPrefix = next("--out");
        else if (a == "--dir3") {
            const std::string v = next("--dir3");
            const auto pos = v.find(':');
            std::string label = pos == std::string::npos ? "d" : v.substr(0, pos);
            std::string comp = pos == std::string::npos ? v : v.substr(pos + 1);
            float dx = 0, dy = 0, dz = 0;
            if (std::sscanf(comp.c_str(), "%f,%f,%f", &dx, &dy, &dz) != 3) {
                err = "bad --dir3 value: " + v; return false;
            }
            const float l = std::sqrt(dx * dx + dy * dy + dz * dz);
            opt.dirs.push_back({label, {dx / l, dy / l, dz / l}});
        } else if (a == "--dirza") {
            const std::string v = next("--dirza");
            const auto pos = v.find(':');
            std::string label = pos == std::string::npos ? "za" : v.substr(0, pos);
            std::string comp = pos == std::string::npos ? v : v.substr(pos + 1);
            double zenDeg = 0, azDeg = 0;
            if (std::sscanf(comp.c_str(), "%lf,%lf", &zenDeg, &azDeg) != 2) {
                err = "bad --dirza value: " + v; return false;
            }
            const double zen = zenDeg * kPi / 180.0;
            const double az = azDeg * kPi / 180.0;
            opt.dirs.push_back({label, {static_cast<float>(std::sin(zen) * std::sin(az)),
                                       static_cast<float>(std::cos(zen)),
                                       static_cast<float>(std::sin(zen) * std::cos(az))}});
        } else if (a == "-h" || a == "--help") {
            err = "help"; return false;
        } else {
            err = "unknown option: " + a; return false;
        }
    }
    if (opt.outPrefix.empty()) {
        opt.outPrefix = std::filesystem::path(objPath).stem().string() + "_hex";
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    Options opt;
    std::string objPath, err;
    float voxelSize = 1.0f;
    try {
        if (!parseArgs(argc, argv, opt, objPath, voxelSize, err)) {
            if (err != "help") { std::fprintf(stderr, "hexvoxel: %s\n", err.c_str()); return 2; }
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "hexvoxel: %s\n", e.what());
        return 2;
    }

    hexvoxel::HexScene scene;
    if (!hexvoxel::loadObj(objPath, voxelSize, scene, &err)) {
        std::fprintf(stderr, "hexvoxel: %s\n", err.c_str());
        return 1;
    }

    std::printf("== hexvoxel: heterogeneous voxel computation ==\n");
    std::printf("obj            : %s\n", objPath.c_str());
    std::printf("voxelSize      : %.3f m\n", voxelSize);
    std::printf("triangles      : %zu\n", scene.tris.size());
    std::printf("objects        : %zu\n", scene.objects.size());
    for (const auto& ob : scene.objects) {
        std::printf("  - %-24s tris=%u closed=%s\n", ob.name.c_str(), ob.triCount,
                    ob.closed ? "yes" : "no");
    }

    // 默认统计方向 (可被 --dir3/--dirza 追加)
    const std::vector<std::pair<std::string, std::array<float,3>>> defaults = {
        {"zenith", {0.0f, 1.0f, 0.0f}},
        {"horiz_z", {0.0f, 0.0f, 1.0f}},
        {"horiz_x", {1.0f, 0.0f, 0.0f}},
        {"45_z", {0.0f, 0.70710678f, 0.70710678f}},
        {"45_x", {0.70710678f, 0.70710678f, 0.0f}},
    };
    std::vector<std::pair<std::string, std::array<float,3>>> allDirs = defaults;
    for (const auto& d : opt.dirs) {
        const bool dup = std::any_of(allDirs.begin(), allDirs.end(),
                                     [&](const auto& e) { return e.first == d.first; });
        if (!dup) allDirs.push_back(d);
    }

    std::vector<float> dirFlat;
    std::vector<std::string> labels;
    std::vector<const char*> labelPtrs;
    for (const auto& d : allDirs) {
        dirFlat.push_back(d.second[0]);
        dirFlat.push_back(d.second[1]);
        dirFlat.push_back(d.second[2]);
        labels.push_back(d.first);
    }
    for (const auto& s : labels) labelPtrs.push_back(s.c_str());

    hexvoxel::HexConfig cfg;
    cfg.sampleN = opt.n;
    cfg.colM = opt.m;
    cfg.volK = opt.k;
    cfg.includeInteriors = opt.includeInteriors;
    std::printf("sampling       : N=%d M=%d K=%d interiors=%s\n", opt.n, opt.m, opt.k,
                opt.includeInteriors ? "on" : "off");
    std::printf("grid estimate  : ~%dx%dx%d voxels\n",
                (int)std::floor(scene.maxPos[0] - scene.minPos[0]) + 1,
                (int)std::floor(scene.maxPos[1] - scene.minPos[1]) + 1,
                (int)std::floor(scene.maxPos[2] - scene.minPos[2]) + 1);

    hexvoxel::HexResult r =
        hexvoxel::compute(scene, cfg, dirFlat.data(), static_cast<int>(allDirs.size()),
                          labelPtrs.data());

    std::printf("computed in    : %.1f ms\n", r.computeMs);
    std::printf("grid           : %d x %d x %d = %lld voxels\n", r.gridX, r.gridY, r.gridZ,
                (long long)r.gridX * r.gridY * r.gridZ);
    std::printf("active voxels  : %lld (FULL=%lld, MIXED=%lld, EMPTY=%lld)\n",
                r.nFull + r.nMixed, r.nFull, r.nMixed, r.nEmpty);
    std::printf("mean volume density (all cells)  rho_mean = %.4f\n", r.meanRhoAll);
    std::printf("mean volume density (active)     rho_act  = %.4f\n", r.meanRhoActive);
    std::printf("mean/max aggregation index       A        = %.4f / %.4f\n", r.meanA, r.maxA);
    std::printf("direction check (A_est extrapolation vs A_true facet reference):\n");
    for (const auto& d : r.dirStats) {
        std::printf("  %-10s dir=(%+.3f,%+.3f,%+.3f) n=%d  A_est=%.4f A_true=%.4f "
                    "mean|err|=%.4f rms=%.4f\n",
                    d.label.c_str(), d.dir[0], d.dir[1], d.dir[2], d.nVoxels, d.meanAest,
                    d.meanAtrue, d.meanAbsErr, d.rmsErr);
    }

    std::string tsv = hexvoxel::toTsv(r);
    std::ofstream tsvOut(opt.outPrefix + ".tsv");
    tsvOut << tsv;
    std::string json = hexvoxel::toJson(r, objPath);
    std::ofstream jsonOut(opt.outPrefix + ".json");
    jsonOut << json;
    std::printf("output         : %s.tsv (%lld rows)  %s.json\n", opt.outPrefix.c_str(),
                (long long)r.voxels.size(), opt.outPrefix.c_str());
    return 0;
}
