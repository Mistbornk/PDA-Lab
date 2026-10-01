# Physical Design Automation Lab

這是由 NYCU Physical Design Automation 課程 Lab01–04 整理而來的 C++17 EDA 專案。
四個程式處理不同的實體設計問題，保留原始輸入／輸出及課程驗證工具，並加入可重現建置、
邊界測試、sanitizer、效能量測與可比較的演算法路徑。
課程規格、測資及助教提供的工具歸原作者所有；工程改善不代表這些資產是本專案原創。

| Lab | 問題與既有演算法 | 本次工程改善 | 格式／操作 |
|---|---|---|---|
| 01 | Corner stitching；插入矩形、point finding、鄰居枚舉 | RAII ownership、四向邊界索引、scan 對照、完整 stitch／raster oracle | [Lab01](Lab01/TESTING.md) |
| 02 | Fixed-outline floorplanning；B*-tree、simulated annealing | parser／tree／packing／annealer／report 分離、pin ID、seed／預算實驗、64-bit 成本 | [Lab02](Lab02/TESTING.md) |
| 03 | Incremental FF banking legalization；nearest-row、R-tree | 每列區間 first-fit、compact cell ID、直接刪除索引、parser/core 分離、策略與查詢統計 | [Lab03](Lab03/TESTING.md) |
| 04 | Die-to-die global routing；逐 net A* heuristic search | 連續 workspace、可選 layer-state A*、Guide 成本模型、獨立 Dijkstra oracle | [Lab04](Lab04/TESTING.md) |

## 建置

Linux x86-64 是原課程 verifier binary 的環境。編譯本專案需要 CMake 3.16+、C++17 compiler、
Boost headers；測試 runner 需要 Python 3.10+，benchmark 另需 GNU time。
在 Ubuntu 可安裝 `build-essential cmake libboost-dev python3 time`。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

輸出為 `build/Lab01/Lab1`、`build/Lab02/Lab2`、`build/Lab03/Legalizer`、`build/Lab04/D2DGRter`。
每個 Lab 原有 Makefile 仍可使用；建議採 root CMake，以免覆寫 Git 追蹤的舊 binary。

預設測試不依賴網路或課程 evaluator，包含已保存的 Lab01 回歸、獨立 raster oracle、
B*-tree 小型幾何／HPWL 驗證、兩種 legalization 搜尋等價性及 routing path 檢查。
Python 與 C++ 核心測試的檢查均不受 Release 的 `NDEBUG` 影響。

```bash
# Debug + ASan / UBSan，預設開啟 leak detection
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DPDA_SANITIZERS=ON
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure

# 還原固定版本的課程測資與驗證器
python3 tests/fetch_resources.py
python3 tests/fetch_resources.py --check
# 完整測試可能花數分鐘；Lab02 預設每案約 280 CPU 秒
python3 tests/run.py --bin-root build
# 或註冊為 CTest 的 official label
cmake -S . -B build -DPDA_FULL_TESTS=ON
ctest --test-dir build -L official --output-on-failure
```

GCC/Clang 預設 `-Wall -Wextra -Wpedantic`。
`-DPDA_STRICT_WARNINGS=ON` 額外啟用 conversion/shadow 診斷，便於逐步處理保留的課程程式；
這不是宣稱目前所有 legacy code 都已消除這兩類警告。Boost include 作為 system headers。

## 使用

```bash
build/Lab01/Lab1 Lab01/tests/data/case0.txt /tmp/case0.out

# 固定 seed 與工作量；同一工具鏈可重現解，runtime 欄仍會不同
build/Lab02/Lab2 0.5 Lab02/tests/data/ami33/ami33.block \
  Lab02/tests/data/ami33/ami33.nets /tmp/ami33.rpt \
  --seed 1 --iterations 1000000 --stats
# 也可使用 --seconds 10；不能同時指定 iterations 與 seconds

build/Lab03/Legalizer Lab03/tests/data/testcase1_16900.lg \
  Lab03/tests/data/testcase1_16900.opt /tmp/post.lg \
  --strategy first-fit --search interval --stats

build/Lab04/D2DGRter Lab04/tests/data/testcase0/testcase0.gmp \
  Lab04/tests/data/testcase0/testcase0.gcl Lab04/tests/data/testcase0/testcase0.cst /tmp/route.lg
```

Lab02 預設仍使用約 280 CPU 秒，新增 `--iterations` 供公平比較固定工作量；
`--hpwl strings` 保留未做 pin ID 最佳化的對照路徑，`--hpwl ids` 為預設。
隨機探索在給定預算內找不到合法解時回傳錯誤，不會將越界解宣稱為成功。
使用不同時間預算或 seed 的結果不能只比較 runtime。

Lab03 的 `--search point` 是逐候選碰撞查詢的對照路徑；`interval` 為預設。
`--strategy nearest` 保留另一個原始 heuristic，**並非全域最近或最佳合法化的保證**。
為兼容原課程解，未指定策略時保留 legacy 的 `testcase2_100.lg` 檔名分派；
實驗時應明確指定策略，避免改檔名影響結果。每次輸出會重新建立，已修正原先 append 舊結果的行為。

## 演算法與架構

[架構文件](docs/architecture.md) 記錄每個 Lab 的資料結構、複雜度、原始風險與保留限制。
共用部分僅有 checked I/O 與實驗工具；沒有將不相關的演算法強制套入同一 framework。

Lab03 的 first-fit 改善來自搜尋次數，而非改變放置目標：原程式從 row 左端開始，遇到重疊就跳到
障礙物右端，再重新查 R-tree。現在對該 row 與 cell 高度查一次障礙物，排序其 x 區間，直接跳過被占用的區間。
單列從多次 spatial query 改成一次 query 加 `O(k log k)` 排序；保留 row 的選擇與 tie ordering。
R-tree 仍負責不同高度 cell 的空間篩選，沒有假設所有 cell 都是單列高。

Lab02 把不變的 net pin 名稱在 parsing 後轉成整數 ID，HPWL 內迴圈只查向量。
B*-tree packing 與退火的接受／擾動流程保留，未用減少工作量造成表面上的速度改善。
Lab04 的 `legacy` 保留原公式與排序；新增 `--router layered` 分離位置與抵達層，依 Guide 計入端點與 via，並以獨立 Dijkstra 核對成本。詳見 [搜尋模型](docs/layered-routing.md)。

目前仍保留四個 Lab 目錄；新增的邊界如下：

```text
include/pda/io.hpp            共用 checked I/O
Lab01/header/TileList.h       tile ownership 與邊界索引
Lab02/include/, src/          資料模型與五個核心實作模組
Lab03/parser.cpp, legalizer.cpp
Lab04/parser.cpp, layered_router.cpp
tests/                       C++ 核心不變條件與 Python oracle
benchmarks/                  配對量測、多 seed／預算研究
.github/workflows/ci.yml      GCC／Clang／sanitizer CI
```

## 效能與驗證

[量測方法、原始 JSON 與結果](benchmarks/README.md) 提供同工具鏈比較、runtime、peak RSS、
objective 與解的 hash；[profiler 摘要](docs/profile-baseline.txt) 記錄 hotspot 證據。
不要把原課程 `Lab01 -O0` 與新的 Release 差異當成演算法速度提升。

| 代表案例 | 比較路徑 | 中位時間（秒） | 倍率 |
|---|---|---:|---:|
| Lab01 3,600 矩形 | 前版全掃描 → 邊界索引 | 2.63 → 1.35 | 1.95× |
| Lab02 ami33 | 同 seed／100 萬次迭代，strings → IDs | 23.17 → 7.26 | 3.19× |
| Lab03 testcase1_16900 | 原始程式 → 區間搜尋 | 38.07 → 7.37 | 5.17× |
| Lab04 testcase2 | 原始程式 → 連續 workspace | 0.38 → 0.15 | 2.53× |

每個配對的解與 objective 相同（Lab02 排除 runtime 行）；這些是所列案例的量測，
不是所有資料集的速度保證。Lab03 代表案例 peak RSS 中位數由 35.19 MiB 降至 28.29 MiB。


Lab01 的加速付出索引記憶體（該合成案例 4.27 → 6.47 MiB）。Lab04 新的 layered 版本是另一個品質／速度取捨：
四組資料成本下降 0.93%～21.82%，其中 testcase2 成本 174,838.81 → 136,686.48，時間 0.15 → 0.32 秒。
此方案以 `--router layered` 啟用，不把較慢但較佳的解描述為速度提升。
四輪逐項紀錄見 [iterations](docs/iterations.md)。

不同 nets 共享 capacity、不同 banking steps 共享 placement、退火依賴前次狀態，
因此未將這些內部迴圈強行平行化。測試與 benchmark 支援多個**獨立案例 process**，
量測 1／2／4／8 個 workers 的 batch scaling；這是實驗工作流的並行，不是單案 solver 的加速宣稱。

## 持續驗證

[CI workflow](.github/workflows/ci.yml) 在 push／PR 執行 GCC Release、Clang 14 Release 與 GCC ASan／UBSan，
預設不下載大型資源。手動 workflow dispatch 可選擇官方完整整合測試與 layered routing 驗證。
本機三種組合均已通過；GitHub 遠端執行須在這些變更提交並推送後才會發生。

```bash
# 三組資料、三個 seed、兩種預算、兩種 HPWL 模式，保留所有失敗
python3 benchmarks/lab2_study.py
```

研究將合法率、合法解成本分布、runtime 與記憶體分開記錄；結果與方法見 [benchmarks](benchmarks/README.md)。

## 限制與後續工作

- 公開測資通過不等於所有隱藏案例或全域最優性。課程輸入的非重疊初始 placement 等前提仍重要。
- Lab01 修補已使用邊界索引，但 overlap 與 split 定位仍會全表掃描；同一邊界 bucket 的最壞搜尋仍為線性。
- Lab02 已拆分 parser／tree／packing／annealer／report，保留 coordinate-sized contour 與整體 trial snapshot；
  同 seed 的可重現性限同平台／標準函式庫，尚未提供跨平台 PRNG 序列契約。
- Lab03 不會移動既有 cells 騰出空間，假設連續等高且同寬的 placement rows；
  只支援此課程範圍。更好的擾動／位移品質仍是演算法研究方向。
- Lab04 預設 legacy 仍是原 heuristic；可選 layered 在既有容量固定時搜尋每 net 的最短成本路徑，
  會花更多時間，且依然不保證多 net 全域最佳。
- 本 repo 尚未為原始課程內容取得統一再授權；Lab03 upstream MIT LICENSE 有保留，其他資產依來源歸屬。
