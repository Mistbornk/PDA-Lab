# 四輪工程紀錄

## 第一輪：Lab02 模組邊界

完成 model、parser、tree、packing、annealer、report 拆分，`main.cpp` 由約 600 行縮為 66 行 CLI，CMake 可獨立連結 `lab2_core`。保留退火順序與原有 libc 隨機序列。

- Release 與 ASan／UBSan CTest 皆通過；新的核心測試檢查 20,000 次樹擾動，並以獨立矩形 oracle 核對 packing。
- ami33、ami49、vda317b 各 seed=1、100 萬次迭代，拆分前後解完全一致（排除 runtime 行），六次官方 verifier 均通過。
- 原有 Makefile 在隔離目錄編譯通過。
- [原始比對結果](../benchmarks/results/round1-equivalence.json)。此輪是架構改善，單次時間僅作紀錄，不宣稱加速。
- 為保留解，libc `rand()` 暫留；同 process 呼叫 `solve` 須序列化，多案例用獨立 process。

## 第二輪：Lab01 局部候選修補

Profiler 確認 65.82% samples 在 `UpdateCornerStitches`。新增 immutable tile 幾何、四向邊界索引、以插入順序選取匹配者，以及 O(1) 平均 list-position lookup；保留 scan reference。未更換 corner-stitch split/merge 演算法。

- 100 seeds × 兩種模式，每次插入後核對所有 stitches、全部鄰居與網格查詢；Release／ASan／UBSan 通過。
- 4 組正式 regression 及 36 次三路 benchmark 均通過，輸出一致。
- 3,600 矩形案例中位數 2.63 → 1.35 秒，1.95×。其餘資料、候選次數及記憶體取捨見 [benchmark](../benchmarks/README.md#第二輪lab01-邊界索引)；不是對全部幾何的速度保證。
- [原始 JSON](../benchmarks/results/lab1-stitches.json)、[profile](profile-lab1.txt)。

## 第三輪：Lab04 layer-state 搜尋

新增 `--router layered`，保留 legacy 預設。新圖分離 arrival layer，正確處理 Guide 的 via 平均 cell cost、起終點和 marginal overflow；weighted Manhattan 是非負轉移成本的一致 heuristic。

- 80 組生成圖、400 條 sequential nets：獨立四狀態 Python Dijkstra、輸出合法性及成本核對通過。
- Release 與 ASan／UBSan CTest 四組全部通過，原 Makefile 隔離編譯通過。
- 兩變體 × 4 正式／toy datasets × 3 次：24 次官方驗證全過。
- testcase2 成本 174,838.81 → 136,686.48（改善 21.82%），layered 花更多搜尋時間；完整速度／品質表見 [benchmark](../benchmarks/README.md)。
- 每 net 最佳性僅針對固定先前容量的圖模型，沒有宣稱多 net 全域最佳。參見 [模型與證明界線](layered-routing.md)。

## 第四輪：多 seed 研究與 CI

完成 36 次序列配對實驗（3 datasets × 3 seeds × 2 budgets × 2 HPWL modes），18 對的合法性一致，合法解完全相同。22 次合法、14 次預算用盡；沒有非預期失敗，失敗也保留在 JSON。

- 300,000 iterations 的 ami33、ami49 有約 3.16×、2.44× 配對中位加速；零 net 的 vda317b 沒有顯著收益。
- 100,000 iterations 並非這些 seeds 的普遍足夠預算，不能把早停無解當作加速。完整合法率與品質分布見 [研究](../benchmarks/README.md#第四輪lab02-多-seed資料預算配對研究)。
- 新增 GitHub Actions：GCC Release、Clang 14 Release、GCC ASan／UBSan 三種組合；預設四組 core CTest。手動選項可另跑完整官方測試與 layered router。
- 本機三種 matrix 組合通過，actionlint 1.7.12 通過，66 個資源校驗通過。workflow 尚未推送，沒有宣稱遠端 Actions 已執行。
- [原始研究 JSON](../benchmarks/results/lab2-study.json)、[CI 契約與本機驗證](ci.md)。

最終三種建置的 CTest log、核心 source SHA256 與各輪 evidence 索引保存在 [four-round-validation.json](four-round-validation.json)。
