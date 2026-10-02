# Physical Design Automation Lab

這是由 NYCU Physical Design Automation 課程 Lab01–04 整理而來的 C++17 EDA 專案。
四個程式處理不同的實體設計問題，保留原始輸入／輸出及課程驗證工具，並加入可重現建置、
邊界測試、sanitizer、效能量測與可比較的演算法路徑。
課程規格、測資及助教提供的工具歸原作者所有；工程改善不代表這些資產是本專案原創。

| Lab | 問題與既有演算法 | 本次工程改善 | 格式／操作 |
|---|---|---|---|
| 01 | Corner stitching；插入矩形、point finding、鄰居枚舉 | RAII、模組拆分、邊界與空間索引、scan 對照、stitch／raster oracle | [Lab01](Lab01/TESTING.md) |
| 02 | Fixed-outline floorplanning；B*-tree、simulated annealing | 核心 library、skyline / journal、私有亂數與平行多起點、可比較退火政策、首次合法解診斷 | [Lab02](Lab02/TESTING.md) |
| 03 | Incremental FF banking legalization；nearest-row、R-tree | 交易式 core session、跨 row 最小位移、有限局部修復、逐步官方成本與查詢統計 | [Lab03](Lab03/TESTING.md) |
| 04 | Die-to-die global routing；逐 net A* heuristic search | layer-state A*、結構化路徑與 usage、預算式重繞、最佳完整解、Dijkstra / 成本 oracle | [Lab04](Lab04/TESTING.md) |

## 面試展示入口

[英文技術摘要](docs/portfolio.md) · [六輪交付紀錄](docs/interview-progress.md) ·
[統一實驗報告與圖表](docs/experiments/README.md) · [設計決策](docs/decisions/001-state-transactions.md)

```bash
# 完成下方建置後，不需網路或下載 evaluator
python3 scripts/demo.py --bin-root build --output build/demo
# 用瀏覽器開啟 build/demo/index.html；可切換 routing net 與壅塞標示
```

Demo 從 solver 輸出產生 SVG / HTML，獨立重算 floorplan 的合法性、HPWL、area，
以及 routing 的連通性、edge usage、overflow、via 與原始成本。輸入由固定 seed 生成。
[已產生的 floorplan](docs/demo/floorplan.svg) 與 [routing](docs/demo/routing.svg) 可直接預覽。

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
新增 PRNG、rollback、多起點一致性、狀態交易、政策品質、逐步修復與離線 demo，共 12 組 CTest。
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
`-DPDA_STRICT_WARNINGS=ON -DPDA_WARNINGS_AS_ERRORS=ON` 額外啟用 conversion/shadow
診斷並將警告視為錯誤。GCC 11.4、Clang 14 及 sanitizer 組合均通過，CI 強制執行。
Boost include 作為 system headers。

## 使用

```bash
build/Lab01/Lab1 Lab01/tests/data/case0.txt /tmp/case0.out

# 固定 seed 與工作量；同一工具鏈可重現解，runtime 欄仍會不同
build/Lab02/Lab2 0.5 Lab02/tests/data/ami33/ami33.block \
  Lab02/tests/data/ami33/ami33.nets /tmp/ami33.rpt \
  --seed 1 --iterations 1000000 --stats
# 也可使用 --seconds 10；不能同時指定 iterations 與 seconds
# 獨立起點平行執行：每個 seed 各有自己的工作／CPU 預算
build/Lab02/Lab2 0.5 Lab02/tests/data/ami49/ami49.block \
  Lab02/tests/data/ami49/ami49.nets /tmp/ami49.rpt \
  --seed 1 --iterations 300000 --restarts 8 --threads 4 --stats

build/Lab03/Legalizer Lab03/tests/data/testcase1_16900.lg \
  Lab03/tests/data/testcase1_16900.opt /tmp/post.lg \
  --strategy first-fit --search interval --stats

build/Lab04/D2DGRter Lab04/tests/data/testcase0/testcase0.gmp \
  Lab04/tests/data/testcase0/testcase0.gcl Lab04/tests/data/testcase0/testcase0.cst /tmp/route.lg
```

Lab02 預設仍使用約 280 CPU 秒，新增 `--iterations` 供公平比較固定工作量；
`--hpwl strings` 保留未做 pin ID 最佳化的對照路徑，`--hpwl ids` 為預設。
隨機探索在給定預算內找不到合法解時回傳錯誤，不會將越界解宣稱為成功。
預設 `--packing skyline --rollback journal`；`dense` 與 `snapshot` 可獨立選回參考路徑。
多起點以 objective 選合法最佳解，同分保留較早起點；固定迭代數時解不受執行緒數影響。
使用不同時間預算或 seed 的結果不能只比較 runtime。

Lab02 新增 `--policy progress|feasibility`，以迭代數控制溫度與可行性階段；
`--trace-every 5000` 輸出首次合法解、outline excess 與 best-cost trace。
120 次短預算比較中，新策略各 40/40 合法，legacy 為 12/40；但 `vda317b` 的合法成本
顯著較差，因此預設仍為 legacy。成功率與成本必須一起比較。

Lab03 新增 `--strategy minimum`，在不移動其他 cells 的單次插入問題求最小曼哈頓位移；
`--strategy repair --repair-cells 2 --repair-candidates 16 --repair-radius 20` 可移動有限數量
非 FIX cells。修復只保證候選中的當步分數選擇，整段 banking 成本可能退步。

Lab04 新增 `--router negotiated --history 0 --rounds 10 --stagnation 3 --seconds 2 --stats`。
初始解與重繞共用 wall budget，保留原始成本最低的完整結果；history penalty 不算入原始分數。
官方案例未改善成本，生成壅塞案例的簡單重繞改善約 1.58–6.81%，但花更多時間。

Lab03 的 `--search point` 是逐候選碰撞查詢的對照路徑；`interval` 為預設。
`--strategy nearest` 保留另一個原始 heuristic，**並非全域最近或最佳合法化的保證**。
為兼容原課程解，未指定策略時保留 legacy 的 `testcase2_100.lg` 檔名分派；
實驗時應明確指定策略，避免改檔名影響結果。每次輸出會重新建立，已修正原先 append 舊結果的行為。

## 演算法與架構

[架構文件](docs/architecture.md) 記錄每個 Lab 的資料結構、複雜度、原始風險與保留限制。
四個 `lab1_core`–`lab4_core` libraries 分開解析、求解、結構化結果與輸出。
共用部分僅有 typed checked I/O 與實驗工具；沒有將不相關的演算法強制套入同一 framework。

Lab03 的 first-fit 改善來自搜尋次數，而非改變放置目標：原程式從 row 左端開始，遇到重疊就跳到
障礙物右端，再重新查 R-tree。現在對該 row 與 cell 高度查一次障礙物，排序其 x 區間，直接跳過被占用的區間。
單列從多次 spatial query 改成一次 query 加 `O(k log k)` 排序；保留 row 的選擇與 tie ordering。
R-tree 仍負責不同高度 cell 的空間篩選，沒有假設所有 cell 都是單列高。

Lab02 把不變的 net pin 名稱在 parsing 後轉成整數 ID，HPWL 內迴圈只查向量。
skyline 將逐座標 contour 換成最多 `2n+1` 個區段，空間不再隨座標大小成長；
undo journal 只保存本次操作觸及的樹節點與旋轉。接受／擾動策略保留，固定工作量的
36 次正式比較皆合法、解一致。詳見 [設計與消融實驗](docs/floorplanning-engineering.md)。
Lab04 的 `legacy` 保留原公式與排序；新增 `--router layered` 分離位置與抵達層，依 Guide 計入端點與 via，並以獨立 Dijkstra 核對成本。詳見 [搜尋模型](docs/layered-routing.md)。

目前仍保留四個 Lab 目錄；新增的邊界如下：

```text
include/pda/io.hpp            共用 checked I/O
Lab01/header/, src/          parsing／layout／report、tile ownership 與兩種索引
Lab02/include/, src/          parser／tree／packing／annealer／undo／parallel／report
Lab03/parser.cpp, legalizer.cpp
Lab04/parser.cpp, legacy_router.cpp, layered_router.cpp, routing_state.cpp
tests/                       C++ 核心不變條件與 Python oracle
benchmarks/                  配對量測、多 seed／預算研究、統一報告
scripts/                     已驗證的離線 demo、完整本機驗證
.github/workflows/ci.yml      GCC／Clang／sanitizer CI
```

## 效能與驗證

[量測方法、原始 JSON 與結果](benchmarks/README.md) 提供同工具鏈比較、runtime、peak RSS、
objective 與解的 hash；[profiler 摘要](docs/profile-baseline.txt) 記錄 hotspot 證據。
不要把原課程 `Lab01 -O0` 與新的 Release 差異當成演算法速度提升。

| 代表案例 | 比較路徑 | 中位時間（秒） | 倍率 |
|---|---|---:|---:|
| Lab01 10,000 矩形 | split／overlap scan → 空間索引 | 5.82 → 0.52 | 11.19× |
| Lab02 ami33 | `89a0239` → skyline／journal，30 萬次迭代 | 2.222 → 1.346 | 1.65× |
| Lab02 ami49 | 同上，3 個配對 seed | 8.059 → 2.709 | 2.98× |
| Lab02 vda317b | 同上 | 19.370 → 5.473 | 3.54× |
| Lab03 testcase1_16900 | 原始程式 → 區間搜尋 | 38.07 → 7.37 | 5.17× |
| Lab04 testcase2 | 原始程式 → 連續 workspace | 0.38 → 0.15 | 2.53× |

Lab01 使用三次 GNU time 中位數；Lab02 使用三個配對 seed 的 wrapper wall time 中位數，
後兩列保留前一階段的歷史結果。每個配對的解與 objective 相同（Lab02 排除 runtime 行）；
這些是所列案例的量測，
不是所有資料集的速度保證。Lab03 代表案例 peak RSS 中位數由 35.19 MiB 降至 28.29 MiB。


Lab01 的索引以額外記憶體減少候選走訪；各大小的 RSS 與所有樣本保存在原始 JSON。Lab04 新的 layered 版本是另一個品質／速度取捨：
四組資料成本下降 0.93%～21.82%，其中 testcase2 成本 174,838.81 → 136,686.48，時間 0.15 → 0.32 秒。
此方案以 `--router layered` 啟用，不把較慢但較佳的解描述為速度提升。
前四輪紀錄見 [iterations](docs/iterations.md)，續作與 AGENTS 要求對照見 [completion](docs/completion.md)。

不同 nets 共享 capacity、不同 banking steps 共享 placement、退火依賴前次狀態，
因此保留這些相依迴圈的順序。Lab02 額外在單一 solver 內執行獨立 seed 的多起點搜尋：
相同 8 次搜尋在 1／2／4／8 threads 為 21.128／10.761／5.488／3.040 秒，
8 threads 為 **6.95×**，每個起點結果與所選解一致。
[同步、CPU 預算與 scaling](docs/parallel-floorplanning.md) 與獨立案例 process 的吞吐量測分開記錄。
新增 [Lab01 容量與分布研究](docs/lab1-capacity.md)，涵蓋密集、長條、共邊與碎片化資料，保留候選走訪數與 RSS。

## 持續驗證

[CI workflow](.github/workflows/ci.yml) 在 push／PR 執行 GCC Release、Clang 14 Release 與 GCC ASan／UBSan，
另外執行獨立 TSan 與四個 libFuzzer parser / Clang Static Analyzer 工作；不下載大型資源。
手動 workflow dispatch 可選擇官方完整整合測試與 layered routing 驗證。
本機三種嚴格警告組合均已通過，四份原始 Makefile 亦在獨立副本建置成功。
遠端結果見 [GitHub Actions](https://github.com/Mistbornk/PDA-Lab/actions/workflows/ci.yml)。

```bash
# 大型生成資料，不下載 evaluator；以獨立 legality／成本 oracle 檢查
python3 tests/stress.py --scale large --output benchmarks/work/stress.json
# 10 個 seed、3／8 CPU 秒、三組資料，保留全部失敗
python3 benchmarks/build_revision.py 89a0239
python3 benchmarks/lab2_comparison.py --suite time \
  --before benchmarks/work/revision-89a023969a2d/build \
  --output benchmarks/work/equal-cpu.json
```

[120 次等 CPU 時間研究](docs/quality-study.md) 保留 45 次短預算無解；合法結果由基準版的
31/60 增至 44/60。共同合法 seed 的成本未退步，但不作跨資料集或成功率保證。
結果、條件與全部 raw data 見 [benchmarks](benchmarks/README.md)。

## 限制與後續工作

- 公開測資通過不等於所有隱藏案例或全域最優性。課程輸入的非重疊初始 placement 等前提仍重要。
- Lab01 的邊界 bucket 與 R-tree 候選搜尋仍有線性最壞情況；不宣稱任意分布皆有相同加速。
- Lab02 skyline 最壞 packing 仍為 `O(n²)`，所有 nets 的 HPWL 仍重新計算；沒有全域最優性保證。
  scalar PRNG 有固定序列契約，但初始 `std::shuffle` 仍依賴標準函式庫，完整解的可重現性限同工具鏈。
- Lab03 假設連續等高且同寬的 rows。minimum 只保證固定其他 cells 的單次最小位移；
  repair 只搜尋有限 blockers / 候選，可能無法找到存在的解，也可能使後續 banking 總分退步。
- Lab04 預設 legacy 仍是原 heuristic；可選 layered 在既有容量固定時搜尋每 net 的最短成本路徑，
  會花更多時間，且依然不保證多 net 全域最佳。
- 本 repo 尚未為原始課程內容取得統一再授權；Lab03 upstream MIT LICENSE 有保留，其他資產依來源歸屬。
