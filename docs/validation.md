# 工程版本驗證紀錄

以下先記錄第一階段工程基準，後續四輪紀錄於本頁末尾；原始課程 baseline 另行保留。
**16/16 公開／toycase 案例通過**；machine-readable 結果與 validator 訊息見 [validation.json](validation.json)。
這些結果不是全域最優性或任意損壞輸入安全性的證明。

| 範圍 | 驗證結果 |
|---|---|
| Lab01 四組原答案 | 全部通過；另用小型 raster oracle 比較 tile 數、鄰居、即時 point query |
| Lab02 三組 public | 預設約 280 CPU 秒、alpha=0.5，原 verifier 全部通過 |
| Lab03 五組 public | 官方 Evaluator 全部通過，與 historical output byte-identical |
| Lab04 三組 public + toycase | 官方 Evaluator 全部通過，與 historical output byte-identical |
| CMake Release | 編譯成功，CTest 通過 |
| CMake Debug + ASan/UBSan | 編譯成功，CTest 通過，含 leak detection |
| Lab03 大型 sanitizer integration | testcase2_100 通過，solver 約 34.8 秒，無 sanitizer 錯誤 |
| 原始 per-lab Makefile | 在獨立 source copy 強制建置，四個皆成功 |
| 下載資源校驗 | 66 個資源及 Lab04 archive 展開內容全部通過 |

`tests/regression.py` 的 10 個 unittest groups 內含多個案例／seed：

- 所有 CLI 的缺參數與缺檔，parser 的不完整輸入／非法名稱／範圍。
- Lab01：4 組原始答案、20 個 seed 的獨立 raster oracle、空 layout、重疊、邊界與 ID 錯誤。
- Lab02：one-block、alpha=0/0.5/1、rotation、B*-tree moves、零 net、負 terminal 座標、
  64-bit area、獨立 HPWL／geometry 檢查，以及 strings／IDs 一致性。
- Lab03：非零 row 原點、fractional geometry、FIX 屬性、重複輸出覆寫、未知名稱、
  malformed banking、無可行位置，以及 10 個 seed 的 interval／point 等價性與 incremental legality。
- Lab04：同 GCell 端點、非方形 grid、非零原點、path continuity、方向／via layer、容量、截斷資料與超過 INT_MAX 的曼哈頓距離。

核心演算法測試不用第三方 binary；公開 integration 用官方 validator。
測試包裝器也以成功程序、逾時程序與不存在的 executable 驗證正常／timeout／error 三個狀態，
timeout 會終止自己建立的 process group，不留下被包裝的 solver。未將非零或無成功訊息誤判為 PASS。

Lab02 的原始版本沒有固定 seed，所以未把這次不同隨機解的成本與最初 snapshot 做公平配對。
效能比較使用同 seed=1、1000000 iterations 的 strings／IDs 路徑，解（扣除 runtime 行）完全相同。
Lab03/04 同時比較原解 hash 和合法性，避免僅靠 solver exit code 或較短 runtime 判定改善。

原始 Lab01 ASan case7 洩漏為 38,528 bytes / 688 objects；ownership 改善後此問題不再出現在回歸測試。
其他仍保留的演算法限制見 [architecture](architecture.md) 及 [README](../README.md)。

## 後續四輪驗證

- GCC 11.4 Release、Clang 14 Release、GCC Debug + ASan／UBSan，四組 CTest 皆通過。
- `lab1_core`：兩模式各 100 seeds，每次插入後檢查所有 stitches、鄰居與全網格 queries。
- `lab2_core`：20,000 次樹擾動與獨立矩形 packing oracle，另驗證 64-bit HPWL。
- `lab4_oracle`：80 張小圖共 400 sequential nets，獨立 Dijkstra、成本及輸出合法性驗證。
- `regression`：既有 10 組 parser／CLI／geometry／官方 Lab01 回歸。
- Lab02 三組各 100 萬 iterations 的模組拆分前後比較，六次官方驗證全通過，解相同。
- Lab01 舊版／scan／indexed 共 36 次量測，比對官方或生成器的獨立答案。
- Lab04 兩變體四組資料各三次共 24 次官方驗證通過，新成本總和與官方 evaluator 吻合。
- CI YAML 使用 actionlint 1.7.12 驗證（shellcheck／pyflakes 未安裝，未執行其可選檢查）。本機已執行三個 core matrix 組合；尚未推送，無遠端 Actions run 可宣稱。

後續結果以 [四輪紀錄](iterations.md)、[benchmark JSON](../benchmarks/results/) 為準；上面的 `validation.json` 保留為前一階段的歷史證據。

最終本機三種建置的測試輸出與 source hash 見 [four-round-validation.json](four-round-validation.json)。
