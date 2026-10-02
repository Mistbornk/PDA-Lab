# EDA 軟體工程面試作品：下一階段規劃

規劃日期：2026-10-02。檢視基準：`616d52e`。
狀態：本文件保留規劃時的問題與驗收標準；實作進度及證據見 [interview-progress.md](interview-progress.md)，
本輪之前的成果見 [completion.md](completion.md)。

建議定位為「可驗證、可重現實驗的 C++ 實體設計演算法工具組」。主要展示
Lab02 的效能與平行搜尋工程，再把 Lab04 深化成可改善多 net 壅塞的演算法案例。
Lab01 展示計算幾何與生命週期管理；Lab03 展示增量狀態更新與 legalization。
四個領域維持自己的問題模型，以共同的工程品質與實驗方式連結。

這份優先順序是依本專案現況與職缺需求做出的工程判斷，並非公司的面試評分表。
Synopsys 台北 Physical Verification R&D 職缺列出 C/C++、Unix/Linux、資料結構、
除錯、效能與品質，以及依需求撰寫設計文件；Cadence 的相關軟體職缺也強調
C++、演算法、計算數學及功能規格。這些能力適合作為作品的驗收方向。
來源：[Synopsys 職缺](https://careers.synopsys.com/job/taipei/r-and-d-engineering-sr-engineer/44408/100796259376)、
[Cadence 職缺](https://cadence.wd1.myworkdayjobs.com/en-US/External_Careers/job/Software-Engineer_R52506-1)。

## 現況與優先順序

已有 C++17 / CMake、GCC / Clang strict warnings、ASan / UBSan、8 組 CTest、
官方測資、獨立 oracle、生成壓力案例、profile 與可重現 benchmark。
Lab02 已有 skyline、undo journal、私有亂數和平行 multi-start；這些是可保留的基礎。

| 優先級 | 工作 | 目前缺口 | 預期面試證據 |
|---|---|---|---|
| P0 | 核心 API 與資料契約 | Lab03 / 04 核心直接輸出文字，尚非獨立 CMake library | 同一核心可被 CLI、測試與報告程式直接使用 |
| P0 | 狀態不變量與失敗處理 | 新增可回復操作前，需要更直接的狀態測試、fuzzing、race 檢查 | 可重現錯誤、rollback 正確性、併發檢查紀錄 |
| P1，主功能 | Lab04 多 net 重繞 | layered A* 仍依序只繞一次，已繞路徑不再調整 | 壅塞、線長、via、原始成本與時間的取捨 |
| P1，次功能 | Lab02 搜尋政策與品質 | 短預算仍有找不到合法解的情況；時間影響搜尋政策 | 首次合法解時間、成功率、anytime 品質曲線 |
| P1，視目標職缺投入 | Lab03 位移品質與局部修復 | 最近 row 不代表總位移最小；目前不移動其他 cells | 每步合法、Move Times / Distance / Total 改善 |
| P1，貫穿各輪 | 實驗與作品展示 | 既有證據多為個別實驗，需組成可快速審閱的案例 | 可重跑的比較報告、英文技術摘要與簡短 demo |
| P2 | Lab01 容量研究、外部格式 adapter | 索引最壞情況、產業資料介接仍可研究 | 規模曲線、記憶體分析、明確格式邊界 |

P0 是後續功能的基礎；P1 依序完成並逐項驗收；P2 只有在 profile、職缺或使用情境
提供明確理由後投入。研究型功能不預先承諾加速或品質改善幅度。

## 1. 架構與 C++ 程式碼：P0

目前 [Lab03 API](../Lab03/include/legalizer.hpp) 以 `ostream` 接收操作結果；
[Lab04 API](../Lab04/inc/router.hpp) 也直接輸出 routing 文字。
而 [layered router](../Lab04/layered_router.cpp) 的 `emit()` 同時寫出路徑、更新容量。
這些邊界不利於重繞、rollback、結果比較與無檔案測試。

建議調整：

- 建立 `lab3_core`、`lab4_core` CMake targets，延續既有 Lab01 / 02 的方式。
- 各 Lab 保有自己的 `Problem / Options / Result / Stats`；解析、求解、
  合法性檢查與輸出分離。共同層只放確實共用的診斷、計時與檔案工具。
- Lab03 提供逐步 legalization 的結構化結果，包含新 cell 位置與其他 moved cells。
  可逐步接收結果，避免因 API 改善而必須儲存整段大型操作歷史。
- Lab04 將 path search、route commit / remove、成本計算、格式輸出分離。
  `RoutingState` 管理容量使用量及每 net 路徑；查詢或輸出不隱含修改使用量。
- 在容易誤用的邊界加入 `CellId / NetId / NodeId` 等領域型別與明確的無效值。
  座標、grid index、成本不混用；保留 Lab03 的浮點輸入語義，重要乘加檢查溢位。
- 區分輸入錯誤、預算耗盡、找不到合法候選與內部不變量失敗。定義失敗後的
  solver 狀態，以及 CLI 是否產生完整結果，讓呼叫端可可靠判讀。
- 保留課程相容 CLI；新的 Lab03 API 必須明確指定策略，檔名分支只留在相容入口。

驗收：舊相容路徑通過既有 regression 與官方 verifier；純重構的確定性結果相同
（排除 runtime 欄）。Lab03 / 04 可直接用記憶體模型測試核心，不需開 subprocess。
容量更新、輸出及驗證有獨立測試。效能與 peak RSS 不出現無法解釋的退步。

## 2. 正確性與可除錯性：P0，後續持續擴充

新增的測試應保護狀態轉換，而非複製實作邏輯：

- Lab03：banking 移除、插入與局部修復的交易；失敗回復後所有 cells、名稱索引、
  R-tree 皆一致；FIX cells 永遠不變。
- Lab04：由完整 paths 獨立重算 edge usage，與增量更新比較；remove / reinsert
  對稱；重繞失敗保留原路徑；同一結果重算成本與 verifier 一致。
- 延伸既有生成測試，針對邊界接觸、非零原點、極端尺寸、零權重、容量不足、
  大量同分候選。使用滿足各 Lab 前提的平移、重新命名等 metamorphic 測試。
- 以 [libFuzzer](https://llvm.org/docs/LibFuzzer.html) 建立 parser 的有界 fuzz targets，
  保留最小失敗輸入及回歸測試；成功 parse 後仍需通過模型約束檢查。
- 為 Lab02 平行搜尋建立獨立的 [ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html)
  建置與 CI 工作，涵蓋正常完成、部分失敗、全失敗。維持 1 / 2 / 4 / 8 threads
  的固定工作量結果一致；TSan 測試是動態檢查證據，與 ownership 分析一起使用。
- 對自有程式逐步加入靜態分析；coverage 用來找缺少分支，不以任意覆蓋率取代 oracle。

驗收：在固定 runner 上完成可重跑的 fuzz / sanitizer 工作；記錄工具與執行預算。
若 runtime 不支援 TSan，必須明確標示檢查未執行，不能把 skip 記為通過。
大型長測試可獨立執行，日常 PR 保留快速的核心回歸。

## 3. Lab04：多 net 壅塞改善，P1 主功能

既有 layered A* 的最短路徑保證是「其他 net 的容量使用固定」下的單 net 問題。
下一步應研究已繞 net 對彼此的影響。

1. 實作保存每條路徑及其 edge usage 的 `RoutingState`。
2. 找出壅塞邊與相關 nets，依明確且可重現的規則排序。
3. 移除選定 net 的舊 usage，再搜尋替代路徑，原子地提交或恢復。
4. 先建立單純 rip-up and reroute 對照，再研究 history / congestion penalty。
5. 加入最大輪數、停滯停止、預算控制與 best-so-far 完整解。

OpenROAD 的 global routing 介面提供 congestion iterations 與逐輪 congestion report，
可作為功能與診斷設計的參考；本專案依自己的兩層、兩端點及課程成本契約實作。
參考：[OpenROAD GRT](https://openroad.readthedocs.io/en/latest/main/src/grt/README.html)。

驗收：單次 layered 路徑仍通過 Dijkstra oracle；每輪完整路徑通過獨立 usage / cost
檢查及官方 verifier。記錄原始總成本、total / max overflow、線長、via 數、runtime、
peak RSS 與迭代曲線。history penalty 只引導搜尋，不能冒充原始 objective。

以相同總時間預算比較新舊演算法，並納入初始解計算時間。保存初始及歷次完整解，
以原始 objective 選擇輸出；不能把增加計算時間隱藏成免費改善。容量溢出依課程成本
處理，不能自行把所有非零 overflow 判成非法。需涵蓋路徑衝突明顯及改善有限的案例。
預設策略的變更必須有跨案例證據。

## 4. Lab02：搜尋品質與資料規模，P1 次功能

[既有 equal-CPU 實驗](quality-study.md) 中，ami49 在 8 秒預算有 6/10 次找到合法解，
3 秒則是 0/10。這提供下一步研究方向；只是當時設定下的樣本，不是普遍成功率。

- 先量測首次合法解所需時間、outline 超出量、acceptance ratio、packing / HPWL
  時間占比、目前最佳成本；診斷採固定間隔取樣，量測自身開銷。
- 將溫度、擾動選擇、outline penalty 與停止政策抽離為簡單可設定的 policy。
  既有政策留作對照，新增以進度或迭代驅動的可重現政策。
- 比較 feasibility-first 與 quality refinement 的兩階段搜尋，評估初始化與 reheating；
  每次只改一項政策，避免不清楚改善來自哪裡。
- 用首次合法解時間、成功率、固定時間的最佳合法成本，以及品質對時間曲線驗收。
  固定工作量適合比較實作速度；政策改變時，不要求與舊演算法產生相同解。
- 若新的 profile 仍顯示 HPWL 佔主要時間，先評估連續 net-pin 儲存、不可變名稱與
  可變幾何分離。只有在受影響 nets 的識別與維護成本值得時，才做增量 HPWL。
  B*-tree 擾動可能移動很多 macros，不能只更新被交換的兩個 blocks。

驗收：事先固定案例、seeds、預算、調參集合與保留評估集合；失敗嘗試完整保留。
比較 paired 成功率與合法成本分布，提供變異範圍。多起點比較同時報告總 CPU 工作量、
wall time 與 worker 數，避免用多倍資源冒充演算法優勢。

## 5. Lab03：位移品質與有限局部修復，P1 延伸

目前最近 row 優先的策略不保證新 cell 的總曼哈頓位移最小；現有輸出也永遠回報
0 個其他 moved cells。[Guide 與 evaluator 契約](../Lab03/TESTING.md) 允許移動
非 FIX cells，成本同時考慮 Move Times 與 Total Distance。

先做較小且可驗證的功能：對合法 row intervals 求最近可行 site，跨 rows 比較總位移，
用位移下界提早結束搜尋。以小型全部 site 枚舉 oracle 檢查，在不移動其他 cells 的
子問題內是否選到最低位移位置；平手規則固定。

之後才加入有限 window 的局部修復：限制移動 cells 數、搜尋範圍與預算；比較單純
遠距安置和局部推移的官方成本；失敗時完整 rollback。需要正確輸出 moved-cell 清單，
並保留逐步驗證。任何新 score 計算先與固定版本 evaluator 對照。

驗收：五組公開資料及不同利用率、障礙分布、跨 row cells 的生成案例，每一步合法。
記錄 Move Times、Total Distance、Total、最差位移、runtime 與 RSS；不得只以新 cell
位移下降主張整體品質改善。局部修復的實作風險較高，排在 Lab04 主功能之後。

## 6. Lab01：以測量決定後續投入，P2

空間索引與生命週期管理已有完整案例。下一步先擴充密集、長條、碎片化、共邊等資料
分布，量測 candidate visits、tiles 數與每次插入耗時，而非只增加均勻 grid 大小。
只有在 profile 證明配置成本或記憶體區域性成為瓶頸後，才比較 tile arena / stable handle。
只有在邊界候選集合過大時，才評估更細的 interval index。

任何新結構保留 half-open 幾何、stitch 與鄰接 oracle；同時報告 memory 與 runtime。
若應徵 computational geometry / physical verification 團隊，可提高這項優先級。

## 7. 實驗與面試交付，P1

沿用現有 benchmarks 與資源 hash；新增內容聚焦於比較完整性與可審閱性：

- 相同配置輸出統一實驗摘要：revision、dirty state、input / executable hash、參數、
  工具鏈、CPU、threads、wall / CPU time、RSS、合法性、objective 與失敗原因。
- 除官方資料外，加入可重現的不同規模與困難分布；分開計時重複與不同 seed 的演算法
  變異。固定調參集，保留未參與調參的評估集，避免只對公開課程案例有效。
- 每项重大變更保留 baseline 與單項消融，輸出品質對時間、規模對時間 / RSS 曲線。
  效能門檻以專用環境的噪音估計為前提；共享 CI 優先檢查正確性與結果結構。
- 建立小型離線 demo：一個 floorplan 與一個 routing 案例，輸出可核對數值的 SVG 或
  HTML 視圖，顯示 macros / nets / congestion。圖由求解結果產生，不重做求解邏輯。
- 新增英文一頁技術摘要與兩份精簡設計紀錄：問題、替代方案、採用原因、複雜度、
  不變量、測量與失敗案例。沿用現有技術文件，整理入口，避免重複大量敘述。
- 定義可重跑的展示指令與環境配置；在乾淨 checkout 驗證建置、快速測試及 demo。
  清楚區分課程資產、既有演算法與自己的工程貢獻，保留來源與授權資訊。

完成後應能演示三件事：5 分鐘理解問題與結果、15 分鐘說明一個重要設計取捨、
實際從乾淨環境重跑支撐該取捨的實驗。

## 建議實作里程碑

| 順序 | 可獨立交付的成果 | 完成門檻 |
|---|---|---|
| M1 | Lab03 / 04 core libraries、Result / Stats、狀態與輸出分離 | 相容性回歸、直接核心測試、完整失敗契約 |
| M2 | 交易不變量、parser fuzz targets、Lab02 TSan | 有界可重跑測試與失敗重現；既有 CI 持續通過 |
| M3 | Lab04 rip-up / reroute，逐輪指標與 best-so-far | usage / cost 獨立驗證、官方合法性、等預算品質比較 |
| M4 | Lab02 搜尋政策與首次合法解診斷 | 保留評估集上的成功率與品質曲線；單項消融 |
| M5 | Lab03 最小位移候選，再評估有限局部修復 | 小型枚舉 oracle、逐步合法性、官方總成本比較 |
| M6 | 整合實驗報告、離線 demo、英文技術摘要 | 乾淨環境可重跑；所有展示數字都有原始紀錄 |

M1 → M2 → M3 為第一條主線；實驗腳本與設計紀錄隨各里程碑交付，不延後到 M6。
時間有限時，先交付 M1–M3 與精簡展示，Lab02 可用既有效能與平行化成果支撐第二個故事。
M4 / M5 依應徵職缺安排深度。每輪回報變更、合法性、品質、時間 / 記憶體、限制與下一步。

完整 LEF / DEF 相容、timing-driven optimization、GPU、分散式執行與大型 GUI 放在
後續需求清單。若要接 OpenROAD / OpenDB，先限定一個 adapter 的單位、pin、障礙及
支援範圍，通過小型轉換驗證，再擴展格式；不能把目前四個不同問題直接視為完整 P&R flow。
