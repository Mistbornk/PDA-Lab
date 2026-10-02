# Lab04 — Die-to-Die Global Routing：格式與測試

依據：[本地 Guide](<Lab4_Guide/IEE PDA_Lab4 Die-to-Die Global Routing.md>) 與其指定的 [Evaluator repository](https://github.com/YubiYubi719/NYCU-PDA-Lab4-Evaluator)，並核對 `main.cpp`。目標是在兩層 grid 上連接兩個 chip 的同 index bumps，最小化線長、overflow、GCell 與 via 的加權成本。現有程式逐 net 執行 A* search，並更新邊使用量。

## 測資與工具

| `tests/data/` 子目錄 | routing area `(x,y,w,h)` | grid `(w,h)` | grid 欄 × 列 | net 數 |
|---|---|---|---|---:|
| testcase0 | (12,0,180,120) | (10,10) | 18 × 12 | 6 |
| testcase1 | (50,30,300,200) | (10,10) | 30 × 20 | 10 |
| testcase2 | (0,513,1710,1140) | (10,10) | 171 × 114 | 57 |
| toycase（檔名前綴 example） | (80,100,780,1080) | (13,27) | 60 × 40 | 20 |

前三組的 `.gmp`、`.gcl`、`.cst` 與 PNG 複製自原 Guide 的 `publicCase/`。toycase 的三個輸入與 `example.lg` 示範解，以及 `tests/tools/upstream/Evaluator.tar` 由 Guide 指定的 repository 取得，固定 revision：`652fce633585336ba76a2e3931831e5f0a34acb5`。

已解開的 checker 在 `tests/tools/upstream/Evaluator/Evaluator`，是 Linux x86-64 靜態 ELF；上游沒有附 checker 原始碼，也未找到 LICENSE。保留來源歸屬，不將它宣稱為本專案原創或擅自指定授權。來源、大小、SHA256 與 Git blob hash 見 [`../tests/resources.json`](../tests/resources.json)。較大的 archive / binary 由 `python3 tests/fetch_resources.py` 還原，不納入 Git。

Guide 提到 3 組 public、2 組 hidden，這裡只有取得 public 與 upstream toycase；toycase 不是第四組原課程 public case。它的示範 `.lg` 只是一個有效解，不能作為唯一路徑標準答案。

## 輸入 `.gmp`：幾何與 bump

```text
.ra
<routing_area_x> <routing_area_y> <width> <height>
.g
<grid_width> <grid_height>
.c
<chip1_x> <chip1_y> <width> <height>
.b
<bump_index> <bump_x> <bump_y>
... chip1 bumps

.c
<chip2_x> <chip2_y> <width> <height>
.b
<bump_index> <bump_x> <bump_y>
... chip2 bumps
```

chip 座標相對 routing area；bump 座標相對所屬 chip。絕對座標 = routing area 原點 + chip 相對座標 + bump 相對座標。兩 chip 相同 index 的 bump 構成一個 net。Q&A 保證 index 從 1 連續遞增；現有程式亦依賴此假設。不要漏掉非零 routing area 原點。

## 輸入 `.gcl`：邊容量

```text
.ec
<left_edge_capacity> <bottom_edge_capacity>
... 每個 GCell 一對
```

GCell 順序是由左至右、由下至上（先 x 再 y）。每個 cell 給左邊及下邊 capacity；右邊由右鄰 cell 的 left capacity 取得，上邊由上鄰 cell 的 bottom capacity 取得。整個 area 的最右及最上外邊界不用給，因為禁止往外走。

## 輸入 `.cst`：成本

```text
.alpha <wirelength_weight>
.beta <overflow_weight>
.gamma <cell_cost_weight>
.delta <via_cost_weight>
.v
<via_cost>
.l
<M1 cell costs，按由左至右、由下至上排列>
.l
<M2 cell costs，同樣順序>
```

每層共 `num_rows × num_cols` 個 cost，`.l` 標記依出現順序代表 M1、M2，沒有額外層編號。Guide Q&A 對幾何／index 等欄位要求整數，但成本範例及實際 `.cst` 包含小數權重、via cost、GCell cost，這些欄位應用浮點數解析；不能因 Q&A 的概括描述把它們都讀成整數。

## 輸出 `.lg` 與合法性

```text
n<index>
M1 <start_x> <start_y> <end_x> <end_y>
via
M2 <start_x> <start_y> <end_x> <end_y>
via
...
.end
... 其餘 net
```

- 每個 net 由 chip1 走向 chip2，名稱如 `n1`，最後 `.end`。
- 座標必須是 **GCell 左下角的絕對座標**，不是 row/column index 或 chip 相對座標。
- M1 只走垂直方向；M2 只走水平方向；相接 segment 必須連續且全部在 routing area 內。
- `via` 單獨一行，在目前 path 位置換層。每條 net 的起終 layer 都是 M1；若起點直接水平出發，先輸出 via；若 M2 水平抵達終點，再輸出 via 回 M1。
- 必須完成所有 nets，可以穿越其他 bump 所在 GCell。
- capacity overflow 是成本項，**不等於 routing 不合法**；不能僅因 evaluator 將 overflow 標成紅色就判失敗。

成本：

```text
oriCost = alpha*WL + beta*OV + gamma*cellCost + delta*viaCost
OV(edge) = max(0, usage-capacity) * 0.5 * max(initial cellCost over both layers)
```

普通 GCell 使用所走 layer 的 cost；有 via 的位置使用兩層 GCell cost 的平均，再加 via cost（各乘對應權重）。Guide Q&A 5、7、11 特別釐清這些規則。評分還有相對所有參賽者中位 runtime 的修正，本地單機無法重建該排名基準；優化比較應保存原始成本分項與實測 runtime。

## 如何測試

從 repository 根目錄：

```bash
python3 tests/fetch_resources.py
make -C Lab04 -B
python3 tests/run.py --lab Lab04
```

runner 把同名 `.gmp/.gcl/.cst` 與新 `.lg` 放入同一個結果目錄，符合 evaluator 的路徑要求。以五項成功訊息（direction、area、all-net-routed、connectivity、alignment）及無錯誤訊息判定，不只看 exit code。官方訊息把 connectivity 拼作 `connecticity`，runner 按其實際輸出辨認。solver 每案預設 timeout 為 Guide 的 1200 秒。

單案手動操作：

```bash
mkdir -p tests/results/manual-lab4
cp Lab04/tests/data/testcase0/testcase0.{gmp,gcl,cst} tests/results/manual-lab4/
Lab04/D2DGRter tests/results/manual-lab4/testcase0.gmp \
  tests/results/manual-lab4/testcase0.gcl tests/results/manual-lab4/testcase0.cst \
  tests/results/manual-lab4/testcase0.lg
Lab04/tests/tools/upstream/Evaluator/Evaluator tests/results/manual-lab4 testcase0
```

只確認 evaluator 可用、不執行 solver：

```bash
Lab04/tests/tools/upstream/Evaluator/Evaluator Lab04/tests/data/toycase example
```

此命令檢查上游提供的 example 解；不能據此宣称自己的 router 已通過。

## 優化時保留的契約

路徑可能有多種合法解，应比較合法性與線長、overflow、GCell cost、via cost、Total，再比較時間及記憶體。先分析 A* heuristic 與代價單位是否一致、layer 是否為搜尋狀態的一部分、更新 overflow 的時機，再做效能改善；目前沒有證據保證這份實作產生全域最優解。

net 之間透過容量使用量相依，不應直接把逐 net loop 平行化。Guide 限制 solver 最多 4 threads；測試 runner 的 `--jobs` 是不同 case 的獨立 process，不是 router 內部平行化。回歸與獨立 Dijkstra oracle 已涵蓋非方形 grid、非零原點、起終 via、同 GCell 端點、零容量、共享邊 overflow 與所有 net 完整性；另有 1,000 × 600 網格的解析成本壓力測試。

## 目前工程版本

解析拆至 `parser.cpp`，原 Router 位於 `legacy_router.cpp` 且擁有每次執行的狀態。搜尋節點與 closed flags 改為可重用的一維連續空間，direction lists 改成固定 array；priority queue tie ordering、cost 與 sequential capacity 更新保持相容。已處理起終點同 GCell 的單點路徑，並修正大座標下曼哈頓距離相加的整數溢位；增加 CLI、容量、幾何、cost 與不完整輸入檢查。四組公開／toycase 的輸出與原始程式逐字一致。legacy 模式保留 layer-state 與 heuristic 的既有限制；下方 layered 模式提供另一個明確成本圖模型。

## 分層搜尋與獨立最短路徑驗證

新增 `--router layered`，預設 `--router legacy` 保留歷史解。layered 將同一 GCell 的 M1/M2 抵達狀態分開，計入起點、終點、換層 cell 與 via，並以每增加一條 net 的 overflow 差值計算成本。`--stats` 在 stderr 逐 net 輸出 incremental_cost 與 expanded。legacy 接受 stats 旗標但不提供這組新成本統計。

```bash
build/Lab04/D2DGRter Lab04/tests/data/testcase2/testcase2.gmp \
  Lab04/tests/data/testcase2/testcase2.gcl Lab04/tests/data/testcase2/testcase2.cst \
  /tmp/testcase2.lg --router layered --stats
ctest --test-dir build -R lab4_oracle --output-on-failure
```

Python oracle 使用 entry/exit × layer 四狀態圖的 Dijkstra，獨立比對 C++ 雙狀態 A*，80 組隨機網格共 400 nets，另逐一核對輸出路徑。詳細 [成本模型](../docs/layered-routing.md)。每 net 的最佳性是在先前容量固定時的圖模型內成立；逐 net 的 greedy 順序不保證全域多 net 最佳解。官方驗證與速度／品質取捨見 benchmark。

## 有預算的多 net 重繞

```bash
build/Lab04/D2DGRter input.gmp input.gcl input.cst output.lg \
  --router negotiated --history 0 --rounds 10 --stagnation 3 --seconds 2 --stats
python3 benchmarks/routing_study.py --output benchmarks/work/routing-study.json
python3 benchmarks/routing_study.py --generated --budgets 1 --output benchmarks/work/congestion.json
```

`--history 0` 是以原始增量成本重繞的對照，正值另加搜尋用 history penalty（預設 1）；
報告只計 Guide 的原始 objective。每輪以壅塞程度、net ordinal 排序，保留 best-so-far
完整解。`--stats` 輸出逐輪 cost / overflow / max overflow / wirelength / via / elapsed
以及最後 summary。壅塞 overflow 是成本項，並非自動判定不合法。

`--seconds` 包含第一個 layered 解的 routing wall time，不含 parsing / reporting，
且為定期檢查的 cooperative deadline；配置和評分可能超過指定時間。首個完整解之前
到期為 exit 3、不新建 report；之後到期回傳最佳完整解並標示 budget_exhausted。
未給時間時由輪數和停滯停止控制。legacy 不支援此預算選項。

官方案例成本持平、耗時增加；生成壅塞案例對簡單重繞有改善，但 history=1 並未全面更好。
詳見 [狀態與成本設計](../docs/decisions/002-negotiated-routing.md)、
[全部實驗](../docs/experiments/README.md)。
