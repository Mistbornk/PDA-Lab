# 資源整理後的驗證紀錄

本次使用現有 Makefile 在獨立目錄強制重新編譯的程式執行，沒有修改 solver 原始碼。Ubuntu 22.04、G++ 11.4.0，Lab01 是 `-O0 -g`，Lab02–04 是 `-O3`，Lab03 保留 `-static`。

測試命令：

```bash
python3 tests/run.py --bin-root /tmp/pda-build-check-rg4p63yz \
  --jobs 4 --timeout 600 --output tests/results/resource-audit
```

**16/16 案通過。** 這是整合驗證，不是受控效能實驗：四個獨立 case 可同時執行；統一 timeout 600 秒與預設課程 timeout 不同；Lab02 只測 alpha=0.5 的一次隨機執行，尚未覆蓋 alpha 端點。表內為 solver wall time，不包含 evaluator。

| Lab | Case | 結果 | Wall seconds | 此次回報 objective |
|---|---|---|---:|---:|
| Lab01 | case0 | PASS | 0.009 | — |
| Lab01 | case1 | PASS | 0.009 | — |
| Lab01 | case2 | PASS | 0.017 | — |
| Lab01 | case7 | PASS | 0.166 | — |
| Lab02 | ami33 | PASS | 280.074 | 675894 |
| Lab02 | ami49 | PASS | 280.226 | 19876111 |
| Lab02 | vda317b | PASS | 280.218 | 17306454 |
| Lab03 | testcase1_16900 | PASS | 40.668 | 683367840.00 |
| Lab03 | testcase1_ALL0_5000 | PASS | 258.296 | 49475212800.00 |
| Lab03 | testcase1_MBFF_LIB_7000 | PASS | 170.930 | 603995292000.00 |
| Lab03 | testcase2_100 | PASS | 13.253 | 12566920200.00 |
| Lab03 | testcase3_4579 | PASS | 93.913 | 352683618000.00 |
| Lab04 | testcase0 | PASS | 0.017 | 1796.74 |
| Lab04 | testcase1 | PASS | 0.034 | 6872.82 |
| Lab04 | testcase2 | PASS | 0.366 | 174838.81 |
| Lab04 | example | PASS | 0.034 | 30526.50 |

Lab01 依 Guide 標準輸出比對；其他 Lab 由原課程驗證器判定。不同 Lab 的 objective 定義不同，不應橫向比較。Lab02–04 的成本顯示精度有限，不可把表格四捨五入值當作精確 reference。

完整當次輸出與 log 在工作目錄的 `tests/results/resource-audit/`（Git ignored）；可版控的 [`validation_snapshot.json`](validation_snapshot.json) 保存工具鏈、輸入／執行檔／輸出 hash、驗證器訊息及測試狀態。這份紀錄只證明列出的課程測資通過，不保證所有邊界輸入或全域最優解。

## 驗證器失敗辨識

刻意傳入損壞解進行負向檢查，新的 runner 均正確拒絕：

| 工具 | 損壞方式 | 官方 exit code | Runner |
|---|---|---:|---|
| Lab02 verifier | 將 cost 改成 -1 | 0 | 拒絕 |
| Lab03 Evaluator | 空輸出，banking 次數不符 | 0 | 拒絕 |
| Lab04 Evaluator | 移除 toycase 第一個 net | 1 | 拒絕 |

因此後續測試不能只依 verifier 的 process return code。相關 log 在 `tests/results/validator-negative-checks/`。

## 資源與輔助工具

- `fetch_resources.py --check`：66 個整理／下載資源及 Lab04 tar 解開內容全部通過校驗。
- Lab03 上游 Evaluator source：以原 Makefile 成功重建，並通過上游 testcase1_16900 示範解的檢查。
- Lab03 `testcase_checker.py`：testcase1_16900 的初始 placement 通過 boundary、overlap、on-site 檢查；未對全部五案重跑此額外 input checker。
- Lab03 五個上游示範解均通過官方 Evaluator；Lab04 toycase 原示範解亦通過。
- Lab01 繪圖程式使用 Agg backend 成功產生 PNG；檔案在 `tests/results/resource-audit/layout0.png`。
- Python wrapper 語法檢查與 `git diff --check` 通過。
