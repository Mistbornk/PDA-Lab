# Lab01 — Corner Stitching：格式與測試

依據：[主規格](Lab1_Guide/2024PDA_Lab1.pdf)、[補充規格](<Lab1_Guide/Lab1 Supplementary.pdf>)，並核對 `src/main.cpp`。這是原課程的 corner stitching 作業；現有程式實作插入矩形、point finding 與鄰居查詢。空白 tile 必須形成最大水平條帶：不能再與左右空白區合併。

## 測資與工具

| 輸入（`tests/data/`） | block 數 | point query 數 | 標準輸出（`tests/expected/`） |
|---|---:|---:|---|
| case0.txt | 5 | 2 | output0.txt |
| case1.txt | 16 | 0 | output1.txt |
| case2.txt | 30 | 10 | output2.txt |
| case7.txt | 360 | 0 | output7.txt |

以上為 Guide 原有檔案的副本，來源和 SHA256 記於 [`../tests/resources.json`](../tests/resources.json)。PDF 沒有提供其他測資或 checker 下載連結。官方補充文件指定用 `diff` 比對答案；沒有另外找到 Lab01 官方驗證執行檔。新加入的 [`../tests/run.py`](../tests/run.py) 是本專案測試包裝程式。

`tests/tools/draw_block_layout.py` 是原有繪圖工具，**不是 correctness checker**。`tests/examples/layout0.txt` 與 `layout0.png` 為繪圖範例。隱藏測資並未公開。

## 輸入

```text
<outline_width> <outline_height>
P <x> <y>
<block_id> <lower_left_x> <lower_left_y> <width> <height>
...
```

第一行指定左下角為 `(0,0)` 的 outline。後續兩種指令可以交錯，必須依序執行，直到 EOF，沒有 block 或 query 的總數欄位。

- 所有值都是整數。矩形不重疊、不越界，可以貼邊。
- ID 為不重複的正整數，可以不連續、不按大小輸入；上限 `INT_MAX-1`。
- block 數 1–25000，outline 寬高各不超過 200000。
- query 範圍為 `0 <= x < outline_width`、`0 <= y < outline_height`。
- tile 包含左、下邊，不包含右、上邊，即 `[x,x+w) × [y,y+h)`。
- 只有角相碰不算相鄰；必須共享正長度的邊界。

例如 `case0.txt` 在插入 block 之前與之後各查詢一次 `P 35 35`。兩次答案不同，不能等全部插入後才查詢。

## 輸出

```text
<最終 tile 總數，含 solid 與 space>
<block_id> <相鄰 solid tile 數> <相鄰 space tile 數>
... 每個 solid block 一行，按 ID 遞增
<query 所在 tile 左下角 x> <query 所在 tile 左下角 y>
... 每個 query 一行，按輸入順序
```

鄰居統計取最終狀態；query 的答案取**該指令執行當下**的狀態。計數只含 outline 內的 tile。

## 如何測試

以下從 repository 根目錄執行：

```bash
make -C Lab01 -B
python3 tests/run.py --lab Lab01
```

腳本逐行比較 token（容忍空白差異，保留行及欄位順序），4 個案例均需通過。每次輸出與 JSON 報告位於新建的 `tests/results/<時間>/`；solver 預設每案上限為 Guide 的 60 秒。

單案手動測試：

```bash
mkdir -p tests/results/manual-lab1
Lab01/Lab1 Lab01/tests/data/case0.txt tests/results/manual-lab1/output0.txt
diff -u Lab01/tests/expected/output0.txt tests/results/manual-lab1/output0.txt
```

繪圖（可選；需要 Python 的 numpy、matplotlib）：

```bash
MPLBACKEND=Agg python3 Lab01/tests/tools/draw_block_layout.py \
  Lab01/tests/examples/layout0.txt tests/results/manual-lab1/layout0.png
```

繪圖輸入與正式輸出**格式不同**：第一行 tile 數、第二行 outline 寬高，接著每行 `id x y width height`，solid 用正 ID、space 用負 ID。現有主程式的 layout 輸出區塊被註解，不能直接將正式答案傳給繪圖工具。

## 目前工程版本

新增 root CMake／CTest；`ctest --test-dir build --output-on-failure` 會執行公開回歸與小型 raster oracle。TileList 用 `unique_ptr` 擁有 tiles，stitches 是非 owning 指標；已修正結束時洩漏與合併後可能再次讀取舊 side pointer。新增 CLI、格式、ID、座標及重疊檢查，錯誤時 exit 1。演算法仍保留原始 split/merge 與全表修補流程。

## 優化時保留的契約

除了 4 組回歸結果，後續應增加邊界 query、非連續 ID、貼邊插入與只有角接觸的案例，以及 tile 覆蓋範圍、無重疊、最大水平空白條帶與 corner pointer 的 invariant 檢查。現有公開測資最大只有 360 個 block，不能據此宣稱在 25000 個 block 上有相同效能。應分別量測插入、查詢、鄰居枚舉，並先維持答案一致再調整資料結構。

## 邊界索引版本

預設使用 `--stitches indexed`；`--stitches scan` 保留全表掃描對照，兩者輸出格式相同。`--stats` 在 stderr 輸出 stitch 修補次數及檢查的候選數。索引沿用原本 split/merge 順序與最後匹配者的選擇規則，不改 tile 分割語意。

`ctest --test-dir build -R lab1_core --output-on-failure` 對兩種模式各測 100 seeds：每次插入後以幾何全掃描核對所有 stitches、鄰居集合，再查遍 16×12 網格。既有 Python raster oracle 另檢查最大水平空白 tiles、插入時點的查詢與四組正式測資。

效能實驗：`python3 benchmarks/lab1_stitches.py --before /path/to/prior/Lab1`，包含 case7 與 400／1,600／3,600 個分離矩形；合成案例有獨立可計算的 tile 數與鄰居答案。更少的候選查詢需額外索引記憶體；同一邊界聚集大量 tiles 時仍可能線性掃描。
