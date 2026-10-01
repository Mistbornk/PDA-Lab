# Lab02 — Fixed-outline Floorplanning：格式與測試

依據：[Lab2.pdf](Lab2_Guide/Lab2.pdf)，並核對 `main.cpp`。課程要求在固定 outline 內配置 hard macros，不重疊，並最小化面積與 HPWL 的加權成本。現有程式採 B*-tree 與 simulated annealing。

## 測資與工具

| `tests/data/` 子目錄 | macro 數 | terminal 數 | net 數 | outline |
|---|---:|---:|---:|---|
| ami33 | 33 | 40 | 121 | 1205 × 1095 |
| ami49 | 49 | 22 | 396 | 5336 × 7673 |
| vda317b | 317 | 0 | 0 | 10000 × 10000 |

每案包含同名 `.block` 與 `.nets`。PDF 將第三組簡稱 vda317，實際檔名為 **vda317b**。資料與 `tests/tools/verifier` 均從原 `Lab2_Guide/` 複製；已補上 verifier 的執行權限。PDF 未附額外下載 URL，也未找到 verifier 原始碼或唯一的標準答案。verifier 是 Linux x86-64 靜態 ELF。

來源與校驗碼見 [`../tests/resources.json`](../tests/resources.json)。原始 Guide 保留，verifier 副本可由 `python3 tests/fetch_resources.py` 重建。

## 輸入

`.block`：

```text
Outline: <width> <height>
NumBlocks: <N>
NumTerminals: <T>
<macro_name> <width> <height>
... 共 N 個 macro
<terminal_name> terminal <x> <y>
... 共 T 個 terminal
```

`.nets`：

```text
NumNets: <M>
NetDegree: <degree>
<macro_or_terminal_name>
... 共 degree 個連接對象
... 共 M 個 net
```

所有輸入數值為整數；各類物件數均小於 500。terminal 位置固定，在 outline 上或外面，不能像 macro 一樣移動。晶片原點為 `(0,0)`，macros 不需保留 channel。macro 的 pin 在中心，依 PDF 規則取整數座標。

額外命令列參數 `alpha` 在 `[0,1]`，含兩端：

```text
Cost = alpha × A + (1-alpha) × W
A = 配置 bounding box 的 width × height
W = 所有 net 的 HPWL 加總
HPWL(net) = int(x_max) - int(x_min) + int(y_max) - int(y_min)
```

PDF 指定 Cost 為整數，HPWL 必須遵循其截斷規則；不要因 verifier 只印有限位科學記號，就認定成本完全相等。vda317b 沒有 net，是面積導向的案例；`alpha=0` 時尤其要另查合法性。

## 輸出 `.rpt`

```text
<整數 cost>
<total wirelength>
<bounding-box area>
<bounding-box width> <bounding-box height>
<runtime_seconds>
<macro_name> <lower_left_x> <lower_left_y> <upper_right_x> <upper_right_y>
... 每個 macro 一行
```

座標為整數，輸出需涵蓋所有 macro，無重疊且整個配置位於固定 outline 內，矩形尺寸必須對應輸入 macro（現有演算法會嘗試旋轉）。runtime 欄位只是回報值，Guide 的 300 秒限制由外部量測。

## 如何測試

從 repository 根目錄：

```bash
make -C Lab02 -B
python3 tests/fetch_resources.py
python3 tests/run.py --lab Lab02 --alpha 0.5
```

每案先執行 solver，再呼叫原課程 verifier，完整保留 `solver.log`、`validator.log`、`.rpt` 和 JSON 結果。不能只判斷程式 exit code，應確認 verifier 中所有檢查通過，包含 outline、面積、cost、重疊等。腳本預設每案 solver 上限 300 秒。

單案手動測試：

```bash
mkdir -p tests/results/manual-lab2
Lab02/Lab2 0.5 Lab02/tests/data/ami33/ami33.block \
  Lab02/tests/data/ami33/ami33.nets tests/results/manual-lab2/ami33.rpt
Lab02/tests/tools/verifier 0.5 Lab02/tests/data/ami33/ami33.block \
  Lab02/tests/data/ami33/ami33.nets tests/results/manual-lab2/ami33.rpt
```

可用 `--case ami33` 篩選單案。alpha 邊界測試另外用 `--alpha 0`、`--alpha 1`，不共用前次的輸出檔。

## 優化時保留的契約

這是多解且隨機的最佳化問題，不能 byte-for-byte 比對 `.rpt`。必須先驗證合法性，再比較 objective、runtime 與記憶體，不能用較快但越界或 cost 較差的解宣稱改善。原程式沒有 seed 參數。工程版本新增 `--seed N`，同時初始化兩個亂數來源；`--iterations N` 可固定工作量並停用依時間變動的擾動／reheat。預設退火仍約使用 **280 秒 CPU time**；多次執行可能產生不同解。平行執行多個測例時 wall time 也可能超過 CPU time，效能比較應單獨執行、重複多次。可用 `--seconds S` 改變 CPU 時間預算，`--stats` 輸出實際 seed 與迭代數。iterations 與 seconds 不可同時使用。`--hpwl ids` 是解析後的整數 pin 索引路徑，`--hpwl strings` 保留對照；相同 seed、迭代預算應產生相同解（不比較 runtime 行）。

新增 parser 或演算法測試時應涵蓋奇數 macro 尺寸的中心取整、零 net、alpha 端點、rotation、outline 貼邊、全部 macro 恰好出現一次。工程版本已檢查 CLI、格式、count、名稱參照與 alpha 範圍，並處理單一 macro 與 64-bit 面積／成本。找不到合法配置時回傳錯誤；此時不應計算 speedup。

## 核心模組與直接測試

`include/model.hpp` 定義 Problem／Placement／Options／Result；`src/` 下的 parser、tree、packing、annealer、report 各有清楚邊界。CMake 的 `lab2_core` 可供測試直接連結，`main.cpp` 僅負責 CLI。

`ctest --test-dir build -R lab2_core --output-on-failure` 會做 20,000 次 B*-tree 擾動，檢查唯一根、parent/child 對應、無環、全節點可達，並以 O(n²) 矩形參考演算法獨立核對 packing；另測 64-bit HPWL。檢查不受 Release NDEBUG 影響。

為保留既有固定 seed 的解，退火保留 libc `rand()` 序列；同 process 不可同時呼叫 `solve`。多 seed 實驗使用獨立 process。

## 多 seed 與工作量實驗

執行 `python3 benchmarks/lab2_study.py` 會以三組正式資料、seeds 1／7／19、10 萬與 30 萬 iterations 配對比較 strings／IDs。報告保存預算用盡的案例，並分別列出合法率與合法解成本，不只取成功 seed。已測樣本中 ami33／ami49 在 10 萬 iterations 均無解，30 萬才全數合法；vda317b 的 10 萬次也不是每個 seed 成功。需自行擴大預算，不能把一次成功當成保證。

兩條 HPWL 路徑在相同 seed／工作量下產生相同結果；若換 seed 或改時間預算，應比較官方合法性、cost／area／HPWL 與 runtime 分布。詳見 [完整研究](../benchmarks/README.md)。
