# Lab03 — Optimizer / Legalizer Co-optimization：格式與測試

依據：[本地 Guide](<Lab3_Guide/NYCU PDA Lab3 Optimizer and Legalizer Co-optimization.md>)、[課程資源 repository](https://github.com/coherent17/Optimizer-and-Legalizer-Co-optimization)，並核對 `main.cpp` 與下載的 evaluator 原始碼。輸入是已合法化的 placement，以及逐次 FF banking 的操作。每一步移除被合併的 FF、插入新 FF，並維持合法配置，盡量減少擾動。現有程式用 Boost.Geometry R-tree 檢查候選位置。

## 測資與工具

Guide 原目錄沒有測資或 evaluator，已從它指定的 repository 下載。固定 revision：`3851e8cf8044b3eb2a0c352ba5dc6731fd6a5e71`，每檔 SHA256 / Git blob hash 與來源 URL 見 [`../tests/resources.json`](../tests/resources.json)。

| `tests/data/` 中的檔名前綴 | 初始 cell 數 | placement row 數 | banking step 數 |
|---|---:|---:|---:|
| testcase1_16900 | 108685 | 604 | 1781 |
| testcase1_ALL0_5000 | 108685 | 604 | 3420 |
| testcase1_MBFF_LIB_7000 | 108685 | 604 | 9632 |
| testcase2_100 | 153457 | 1022 | 2948 |
| testcase3_4579 | 108084 | 604 | 5204 |

每案包含 `.lg`、`.opt` 一對。表中為實際內容計數，**不要用檔名尾數推定 banking 次數**。Guide 舊版寫 3 組 public case，但 upstream 目前 `testcase/` 有以上 5 組；7000 那组為 Guide 公告的額外高難度案例。未取得隱藏評測資料。

- `tests/tools/upstream/Evaluator`：原課程 Linux x86-64 靜態執行檔。
- `tests/tools/upstream/evaluator/`、`Makefile`：同版本 evaluator 原始碼及建置方式。
- `tests/tools/upstream/LICENSE`、`README.md`：保留上游授權與說明，非本專案原創。
- `tests/tools/testcase_checker.py`：上游 input placement checker。雖然要求 `--opt`，實際程式**沒有驗證 banking 操作或 solver 輸出**，不能替代 Evaluator。
- `tests/examples/*_post.lg`：上游 `WebCanvas/data/` 提供的 5 個示範解，供工具檢查／理解格式，**不是要求最佳化程式逐字複製的 golden output**。

較大的下載資料與二進位檔不納入 Git，可執行 `python3 tests/fetch_resources.py` 按固定版本還原。上游還有 GIF、影片、WebCanvas、DieUtilRate 等視覺化工具及其歷史資料副本；此次使用 `testcase/` 作為測試輸入來源，沒有混入其他目錄可能不同版本的 input，也未下載影片等大型展示附件。

## 輸入 `.lg`

```text
Alpha <alpha>
Beta <beta>
DieSize <lower_left_x> <lower_left_y> <upper_right_x> <upper_right_y>
<cell_name> <lower_left_x> <lower_left_y> <width> <height> <FIX|NOTFIX>
... 初始 cells
PlacementRows <start_x> <start_y> <site_width> <site_height> <site_count>
... placement rows
```

注意 `DieSize` 最後兩欄是右上座標，不是寬高。Guide 一般數值以浮點範圍描述，提醒處理 `DBL_MAX`；不要僅依現有案例的整數內容縮限格式。`FIX` cell 不能移動。初始 placement 應已合法，cell 左下角必須落在 row 的 site grid。

row 的 x 範圍為 `[start_x, start_x + site_width × site_count)`。本作業規定 site width 為 1。Q&A 允許假設各 row 有相同 startX、高度相同且在 y 方向連續；cell 可以跨多個 row，不能假設 cell 高度必等於單一 row 高度。

## 輸入 `.opt`

```text
Banking_Cell: <ff_to_remove_1> <ff_to_remove_2> ... --> <new_ff_name> <x> <y> <width> <height>
... 每行一個 banking 操作
```

箭頭前的所有 FF 從當前 placement 刪除。新 FF 的 `(x,y)` 是希望放置的初始位置，**未必合法**。按操作順序逐次插入／合法化，不能將所有操作一次套用後才檢查。之前新建的 banked FF 也可以在後續步驟移動。

## 輸出 `_post.lg`

每個 `.opt` 操作輸出一個區塊，順序完全對應：

```text
<new_ff_legal_x> <new_ff_legal_y>
<number_of_other_moved_cells>
<moved_cell_name> <new_x> <new_y>
... 共 number_of_other_moved_cells 行
```

這是**操作結果序列**，不是完整 placement 檔；沒有 `Alpha`、`DieSize` 等標頭。新 FF 的名字由 `.opt` 對應取得，不再輸出；其他被移動的 cells 才需具名列出。若沒有其他 cell 移動，第二行是 `0`。

合法性要求：每步完成後 cell 不重疊、不越界、on-site，fixed cells 不動。成本依 Guide 為 `alpha × 累計既有 cell 移動次數 + beta × 總曼哈頓位移`。具體累加與原位置的定義以固定版本 evaluator 的 `evaluateScore()` 為可執行基準，不用自行假定成本只計新 FF；結果表提供 Move Times、Total Distance 及 Total。

## 如何測試

從 repository 根目錄：

```bash
python3 tests/fetch_resources.py
make -C Lab03 -B
python3 tests/run.py --lab Lab03 --case testcase1_16900
# 五組完整測試
python3 tests/run.py --lab Lab03
```

預設 solver 每案 timeout 1800 秒（Guide 的 30 分鐘），evaluator 另外限 300 秒，可用 `--validator-timeout` 調整。測試可能花數分鐘或更久，請看各案 log 與狀態。

手動單案（從 repository 根目錄，務必使用新的輸出目錄）：

```bash
run_dir=$(mktemp -d)
Lab03/Legalizer Lab03/tests/data/testcase1_16900.lg \
  Lab03/tests/data/testcase1_16900.opt "$run_dir/testcase1_16900_post.lg"
Lab03/tests/tools/upstream/Evaluator Lab03/tests/data/testcase1_16900.lg \
  Lab03/tests/data/testcase1_16900.opt "$run_dir/testcase1_16900_post.lg"
```

原始 solver 使用 `ios::app`；工程版本已改為每次開啟一個輸出 stream 並 truncate。測試 runner 仍建立新目錄保存各次證據。官方 Evaluator 在操作數量不符或某些合法性失敗時仍可能 exit 0，需確認沒有 `Error` 並出現完整成本表；runner 已額外判讀訊息。

可單獨檢查初始 input placement：

```bash
python3 Lab03/tests/tools/testcase_checker.py \
  --lg Lab03/tests/data/testcase1_16900.lg --opt Lab03/tests/data/testcase1_16900.opt
```

需要重建 evaluator 時，原 Makefile 為 C++11、`-static`、`-lpthread`。建議複製 `tests/tools/upstream/` 到 `tests/build/` 再執行 `make -B`，保留下載原件及其 hash；其中 `run1` 等便利 target 使用上游資料夾結構，不適用本地整理後的路徑。

## 優化前已知差異與回歸重点

原始 parser 依名字開頭推定 fixed 且將輸出座標存成整數；工程版本已改為讀取 `FIX/NOTFIX` 與保留 double 精度，並驗證 banking 名稱、重複 ID 與輸入格式。`testcase2_100.lg` 這個**檔名字串**在預設 legacy 模式仍會觸發不同 legalization 分支，故重命名 input 可能改變結果。新的實驗應明確指定 `--strategy first-fit` 或 `--strategy nearest`。legacy / minimum 輸出 0 個其他 moved cells；repair 可輸出非 FIX cell 的移動清單。

目前回歸已涵蓋非零 die/row 原點、site 對齊、多 row cell、逐步合法性、FIX 屬性與浮點精度；生成壓力測試另外驗證 20 萬 cells、5,000 次 banking 的每一步占用狀態。repair 已支援有限局部移動，legacy 保留原行為。R-tree 查詢要區分邊界相碰與內部重疊，也要涵蓋包含與完全相同矩形。比較優化版本時需記錄合法性、Move Times、Total Distance、Total、solver runtime 及 peak memory；不要只看總執行時間，也不要將示範解視為唯一解。

## 可比較的搜尋路徑

`--search interval`（預設）每個嘗試 row 取得一次障礙物區間，排序後尋找第一個可放置空隙；`--search point` 保留逐候選 R-tree 查詢供比較。nearest 策略使用雙向逐候選搜尋，search 選項不改變此分支。`--stats` 在 stderr 印出 spatial query 與 row 計數。狀態改為 solver 私有，R-tree 僅儲存 box 與 cell ID，banking 透過名稱索引移除，已取消每步全表掃描。五組公開測資與原始程式輸出逐字一致；詳細效能結果見 [benchmark](../benchmarks/README.md)。

## 最小位移與局部修復

```bash
build/Lab03/Legalizer input.lg input.opt result.lg --strategy minimum --stats
build/Lab03/Legalizer input.lg input.opt result.lg --strategy repair \
  --repair-cells 2 --repair-candidates 16 --repair-radius 20 --stats
python3 benchmarks/legalizer_study.py --repeat 3 --output benchmarks/work/legalizer.json
python3 benchmarks/legalizer_study.py --generated --output benchmarks/work/legalizer-generated.json
```

minimum 在其他 cells 不動的條件下，跨 rows / gaps 比較最近 site，平手依 y、x。
repair 限制候選點、直接 blockers 數與每個搬移的曼哈頓半徑；不做無限連鎖推移。
當步評分包含所有 moved cells 的位移變化與移動次數，不只新 FF。
`--search` 僅影響 legacy first-fit；minimum / repair 使用自己的區間搜尋。
`--stats` 增加 `move_times`、`total_distance`、`objective`、`maximum_displacement_seen`。
最後一項是歷來回報位置中的最大原始位移，不是官方 score。官方計分保留已 banking cell
的最後位移紀錄；同名重用也保留 evaluator 的首筆物件語義。

預期無候選時 core placement 不變，CLI exit 3；輸出檔可能已含前面成功步驟，不能當作完整解。
配置失敗等內部 commit 錯誤後須丟棄 session。詳見 [設計紀錄](../docs/decisions/004-bounded-legalization.md)。
200 組小型 oracle 以全部 site 枚舉檢查 minimum，另測跨 row、FIX、重複移動、回復與名稱重用。
