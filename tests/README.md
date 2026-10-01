# 課程測資與測試工具

這個目錄提供資源清單、固定版本下載器與統一測試入口。實際測資與課程驗證工具分別放在每個 `LabXX/tests/`；原本 `LabX_Guide/` 完整保留。最初資源整理保留原 solver；後續工程版本與 CMake 使用方式見 [root README](../README.md)。

此次 16 案的實測結果與負向驗證器檢查見 [VALIDATION.md](VALIDATION.md)。

| Lab | 已整理的 solver 測例 | 驗證方式 | 輸入／輸出與測試說明 |
|---|---:|---|---|
| Lab01 | 4 組 | Guide 標準答案，依序比較各行 token | [Lab01/TESTING.md](../Lab01/TESTING.md) |
| Lab02 | 3 組 | 原有 verifier | [Lab02/TESTING.md](../Lab02/TESTING.md) |
| Lab03 | 5 組 | Guide 指定的官方 Evaluator，另有來源碼與 input checker | [Lab03/TESTING.md](../Lab03/TESTING.md) |
| Lab04 | 3 組 public + 1 組 toycase | Guide 指定的官方 Evaluator | [Lab04/TESTING.md](../Lab04/TESTING.md) |

## 目錄

```text
LabXX/
  TESTING.md
  tests/
    data/       # 課程輸入，保持原檔名
    expected/   # 只有 Lab01：課程標準輸出
    examples/   # Lab01 繪圖範例、Lab03 上游示範解
    tools/      # 原課程 verifier、evaluator、checker、visualizer
tests/
  resources.json       # 每檔原始位置/URL、大小、hash、upstream revision
  fetch_resources.py   # 下載/還原並核對資源
  run.py               # 新增的測試包裝器，不是官方 verifier
  results/             # 每次執行的輸出/log/JSON（Git ignored）
```

Lab04 的原 PNG 放在各 case 目錄，toycase 的 `example.lg` 是上游示範解。Lab03 的示範解不能當唯一正解。課程／上游資產的歸屬保留，所有新增 wrapper 與說明另行區分。

## 還原、編譯與執行

從 repository 根目錄執行，需 Python 3.10+（wrapper 僅使用標準函式庫）、GNU Make、G++，Lab03 另外需要 Boost headers 與靜態 C++ runtime。驗證用的預編譯 binary 需要 Linux x86-64；Lab03 可由提供的 evaluator source 重建，Lab02、Lab04 沒有找到 source。

```bash
# 第一遍下載缺少的固定版本資源；既有檔案會校驗，不覆寫修改過的檔案
python3 tests/fetch_resources.py
# 不下載，核對所有整理後的檔案與 Lab04 解壓內容
python3 tests/fetch_resources.py --check

# repository 帶有舊 binary，所以用 -B 強制從目前 source 編譯
for lab in Lab01 Lab02 Lab03 Lab04; do make -C "$lab" -B || break; done

python3 tests/run.py --list
python3 tests/run.py --lab Lab01
python3 tests/run.py --lab Lab02 --case ami33 --alpha 0.5
python3 tests/run.py --lab Lab03 --case testcase1_16900
python3 tests/run.py --lab Lab04
# 全部 16 案，預設逐案執行，部分案例需數分鐘
python3 tests/run.py
```

可用 `--bin-root /path/to/isolated-build` 指向另一份 build，該目錄下應包含 `Lab01/Lab1`、`Lab02/Lab2`、`Lab03/Legalizer`、`Lab04/D2DGRter`。這能避免覆寫目前 Git 追蹤的舊 binary。`--output` 可以指定結果目錄，但該目錄必須不存在，以隔離每次執行的證據（原 Lab03 append 行為已在工程版修正）。

`--jobs N` 可同時跑 N 個獨立 case（預設 1），只用於縮短整合測試等待；**平行測試下的時間不可當成單獨 benchmark**。`--timeout` 覆蓋 solver 每案的 wall-time 上限；未指定時使用 Guide 的 60／300／1800／1200 秒。`--validator-timeout` 預設另給驗證器 300 秒。沒有進行不安全的 solver 內部平行化。

Lab01 的選用繪圖工具需要 `numpy`、`matplotlib`，不是 solver 或回歸 runner 的依賴。讀取 Guide PDF 時安裝的 `poppler-utils` 也不是執行測試必備。

## 結果判定與保存

每次執行保存每案輸出、solver log、validator log、`result.json` 與總表 `summary.json`。JSON 記錄實際指令、輸入 SHA256、執行檔 SHA256、alpha、wall time、timeout、return code、驗證結果及檔案路徑。任一案不通過時 runner exit 1。

Lab01 比對課程答案，忽略空白差異但保留行序。Lab02–04 同時要求驗證器正常結束、沒有錯誤訊息，以及預期的成功項目／成本表。尤其 Lab03 的官方工具可能在失敗時回傳 0，不能僅依 exit code 判定。

`summary.json` 的時間只作此次執行紀錄，尚非正式效能基準：沒有宣稱量測 peak RSS、跨硬體可比的速度、隨機演算法多次統計或最佳解。Lab02–04 的品質分項保存在 validator log；優化時應同時比較這些分項，不只比較時間。Lab02 預設不固定 seed，改跑一次可能有不同成本；工程版可指定 seed 與迭代預算。

## 來源與下載範圍

`resources.json` 列出每個整理後的檔案，可核對本地 Guide 原件或 pinned GitHub URL。Lab01、Lab02 的 PDF 文字與 hyperlink annotations 均檢查過，沒有另外的下載連結。Lab03、Lab04 由各自 Markdown 的 release/evaluator 連結取得：

- [Lab03 課程資源](https://github.com/coherent17/Optimizer-and-Legalizer-Co-optimization)：`testcase/` 全部 5 對正式輸入、input checker、Evaluator binary/source/Makefile/LICENSE/README，以及同版本 WebCanvas 的 5 個示範輸出。
- [Lab04 評估器](https://github.com/YubiYubi719/NYCU-PDA-Lab4-Evaluator)：Evaluator archive、README、完整 toycase。

Lab03 的其他視覺化資料副本、GIF、影片及一般論文參考連結不作為新增正式測資。Guide 提及的隱藏測資不在可取得資源裡。較大的 Lab03 data、下載 binary/archive 與生成結果加入 `.gitignore`，目前工作目錄已備妥，Git checkout 後使用下載器即可恢復；不需要 clone 巨大的上游完整歷史。

若下載 hash 不符，下載器會停止，避免默默更新測試基準。要刻意換 upstream 版本時，應更新 manifest、重新驗證，並記錄版本差异。Lab04 tar 僅解開 regular files，檢查目的地並拒絕符號連結／路徑逃逸。

## 工程回歸測試

`tests/regression.py --bin-root build` 已納入 CTest，使用 Python unittest 執行 10 組測試（各組包含多個案例／隨機 seed），不需下載的 evaluator binary。Release 與 ASan/UBSan 均需通過。效能測試獨立於 correctness tests，見 [benchmarks](../benchmarks/README.md)。

CMake 另外註冊 `lab1_core`、`lab2_core`、`lab4_oracle`：分別測完整 stitch 不變條件、B*-tree／packing 原語及分層 routing 最短路徑。預設 CTest 共四組，CI 的 GCC／Clang／sanitizer matrix 使用同一入口。官方大型 integrations 由手動 CI 選項獨立執行。
