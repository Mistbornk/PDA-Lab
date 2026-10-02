# Benchmark 方法與結果

所有效能主張都附合法性與解的品質檢查。以下是目前工作環境的一次量測紀錄，
不是跨硬體保證，也不是與其他 EDA 工具的比較。原始記錄在 [`results/`](results/)，
逐次保存 compiler、build flags、執行檔/input/output hash、參數、CPU time、wall time、
peak RSS、objective 與官方 validator log。

## 續作：空間索引、packing、rollback 與內部平行搜尋

以下新實驗以已完成前四輪的 `89a0239` 為工程基準；更早 `364870c` 的測量保留於後段，
不將不同階段的數字相乘作為未實測的總加速。

| 實驗 | 結果 | 證據 |
|---|---|---|
| Lab01 10,000 方塊，geometry scan / spatial | 5.82 → 0.52 秒，11.19× | [三次配對／已知答案](results/lab1-geometry.json) |
| Lab02 ami33，300,000 iterations | 2.222 → 1.346 秒，1.65× | [36 次固定工作量比較](results/lab2-engineering-fixed.json) |
| Lab02 ami49，同預算 | 8.059 → 2.709 秒，2.98× | [packing／rollback 消融](../docs/floorplanning-engineering.md) |
| Lab02 vda317b，同預算 | 19.370 → 5.473 秒，3.54× | [同上](results/lab2-engineering-fixed.json) |
| Lab02 8 起點，1 / 2 / 4 / 8 threads | 21.128 / 10.761 / 5.488 / 3.040 秒，6.95× | [12 次固定總工作量](results/lab2-scaling.json) |

Lab01 是 GNU time 秒數中位數；Lab02 是 wrapper wall time 中位數，固定工作量比較含三個配對
seeds（1/7/19），scaling 則每個執行緒數重複三次。每個變體序列執行、交替先後順序，
測量時不在本機同時跑 build 或其他 benchmark；未鎖 CPU 頻率／cache，沒有做顯著性推論。
所有 36 次 Lab02 解都通過官方 verifier，各配對解完全相同；12 次 scaling 的每個起點結果
及所選解也一致。原始 JSON 包含每筆 RSS，不將微小 RSS 波動解讀成穩定優勢。

[profile](results/gaps-profile.json) 先定位 Lab01 插入的全掃描與 Lab02 dense contour，
再量測 skyline 後的成本分布並加入增量 journal。沒有更動退火接受策略。
[設計、複雜度與限制](../docs/floorplanning-engineering.md) 及
[平行工作相依性／同步／CPU 預算](../docs/parallel-floorplanning.md) 可對照閱讀。

```bash
python3 benchmarks/build_revision.py 89a0239
python3 benchmarks/lab1_geometry.py
python3 benchmarks/lab2_comparison.py --suite fixed \
  --before benchmarks/work/revision-89a023969a2d/build \
  --output benchmarks/work/fixed.json
python3 benchmarks/lab2_scaling.py
# 10 seeds × 3 cases × 2 CPU budgets × 2 implementations
python3 benchmarks/lab2_comparison.py --suite time \
  --before benchmarks/work/revision-89a023969a2d/build \
  --output benchmarks/work/equal-cpu.json
python3 tests/stress.py --scale large --output benchmarks/work/stress.json
```

生成大型案例包含 10,000 方塊、499 macros／20 億 outline 座標、200,000 cells／5,000 banking
steps 與 600,000 個 GCells。它們用獨立答案、幾何／HPWL、逐步 occupied sites 與已知最短路徑
成本驗證；[large](results/stress-large.json)、[smoke](results/stress-smoke.json) 計時只作驗證紀錄，
不是隔離負載的效能比較。`stress_smoke` 已納入 CTest，large 納入選配官方 CI job。

等 CPU 時間的 120 次研究結果見 [品質研究](../docs/quality-study.md)；
[raw JSON](results/lab2-equal-cpu.json) 同時保留 75 個合法結果與 45 次預算耗盡。
共同合法 seed 的成本比較與整體合法率分開呈現，不丟棄失敗，也不跨資料集平均 objective。

## 前四輪歷史實驗的比較條件


- GCC 11.4.0、C++17、`-O3 -DNDEBUG`，動態連結；舊版從 revision `364870c` 取 source 編譯。
  Lab01 原 Makefile 的 `-O0` 不拿來作演算法 speedup 對照。
- 每個 solver 單執行緒；主 before/after 量測 `jobs=1`，各變體順序執行。
  使用系統當時的負載，未鎖 CPU 頻率或清除 page cache，JSON 保存所有樣本而不挑最快值。
- 表格使用 GNU `/usr/bin/time` 的 elapsed seconds 中位數；每次新建 output directory。
  wrapper 的 `wall_seconds` 另含 process 啟動／等待開銷。短案例應注意 GNU time 的 0.01 秒解析度。
- Peak RSS 是 `/usr/bin/time` 的 KiB 中位數換算成 MiB，沒有把 evaluator 的記憶體算進 solver。
- Lab02 比較 **同一工程版中的 strings 與 ids HPWL 路徑**，seed=1、1000000 iterations、alpha=0.5；
  兩者皆完整執行相同的 perturbation/packing，不是縮短時間預算。這不是與不可重現原始退火執行的公平配對。
- Lab03/04 比較歷史演算法與工程版；同一案例所有輸出 byte-identical。
  Lab02 除 runtime 行之外的解也相同。所有量測樣本都通過官方 validator。

## 代表案例結果

| Lab | Dataset | 重複次數 | 中位 elapsed 秒：前 → 後 | 倍率 | Peak RSS MiB：前 → 後 | 兩版 objective |
|---|---|---:|---|---:|---|---:|
| lab2 | ami33 | 3 | 23.17 → 7.26 | 3.19× | 4.12 → 3.99 | 663,037.00 |
| lab3 | testcase1_16900 | 3 | 38.07 → 7.37 | 5.17× | 35.19 → 28.29 | 683,367,840.00 |
| lab4 | testcase2 | 5 | 0.38 → 0.15 | 2.53× | 4.80 → 4.79 | 174,838.81 |

Lab03 的 reference、最佳化版本 objective 都是 683367840，並非用位移品質換速度。
Lab02/04 小幅 RSS 差異不作為穩定記憶體優勢的宣稱。此表只代表所列 dataset；
全部公開案例的 correctness 另見 [工程驗證紀錄](../docs/validation.md)。

## Profiling 與改變理由

[原始 Lab03 gprof 摘要](../docs/profile-baseline.txt)：`testcase1_16900` 有 10,918,065 次
candidate spatial query。約 56.71% samples 在 query、20.09% 在結果 vector 處理、
11.54% 在碰撞判定。first-fit 每列一次查詢的工程版降為 109,579 次查詢，
另外移除 cell/string 查詢結果複製與每步全表移除。

Lab02 的固定 10000 次 trial 探索取樣約 73.68% 在 HPWL，21.05% 在 dense contour；
該短探索尚未產生合法解，**只用來定位 hotspot**。正式速度表用 100 萬次 trial 的合法解。
因此先做 pin 名稱到 ID 的預解析，保留 dense contour；没有無量測地更換 contour 演算法。
Lab04 testcase2 的短 gprof 取樣落在 A* search（取樣數有限），並在此迴圈消除 direction vector
反覆配置、將 workspace 改為連續重用。沒有變更 queue tie-breaking 或宣稱 routing 最優性。

Lab01 第一階段主要是 correctness/ownership 工作：原 ASan 的 case7 回報 38,528 bytes / 688 objects
洩漏；工程版的小型 oracle 與公開資料通過 sanitizer。新增 overlap 檢查付出一次 tile scan，
該階段不宣稱演算法加速。第二輪邊界索引實驗如下。

## 重現

```bash
python3 tests/fetch_resources.py
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
python3 benchmarks/build_baseline.py --revision 364870c

python3 benchmarks/run.py --lab Lab03 --case testcase1_16900 \
  --bin-root benchmarks/work/original-release --repeat 3 \
  --label original --output benchmarks/work/before.json
python3 benchmarks/run.py --lab Lab03 --case testcase1_16900 --repeat 3 \
  --solver-arg=--stats --label interval --output benchmarks/work/after.json

python3 benchmarks/run.py --lab Lab02 --case ami33 --repeat 3 \
  --solver-arg=--seed --solver-arg=1 --solver-arg=--iterations --solver-arg=1000000 \
  --solver-arg=--hpwl --solver-arg=strings --label strings --output benchmarks/work/strings.json
# 改 strings 為 ids 並使用不同 output，得到最佳化路徑

python3 benchmarks/run.py --lab Lab04 --case testcase2 --repeat 5 \
  --label router --output benchmarks/work/router.json
```

亦可用 `--solver-arg=--search --solver-arg=point` 比較 Lab03 的逐候選查詢。
請勿直接拿 Lab02 預設 280 秒的 time-budget 模式宣稱執行時間變短：在該模式下效能改善
主要是讓相同時間內能探索更多 trial，仍需多 seed 的品質研究才能量化好處。

## 並行測試的界線

前四輪僅平行執行獨立 solver process，每案有自己的 parser/solver 狀態與 output directory。
父程序在 futures 完成後匯總 JSON，沒有共用 mutable solver data，沒有需要鎖住的
capacity、placement 或 annealer state。這是工作流吞吐量測試，不是 solver 內部多執行緒。

Scaling 使用相同 16 次 Lab04 testcase2，全部包含 solver、輸出校驗與 evaluator；
1、2、4、8 個 workers 各一次 batch，資料集很短，不能外推至其他大小的 case。
原始資料為 `results/scaling-*.json`。所有 64 次解都通過官方驗證且 output hash 相同。

| Workers | 16 案 batch 秒 | 相對吞吐倍率 |
|---:|---:|---:|
| 1 | 3.389 | 1.00× |
| 2 | 1.684 | 2.01× |
| 4 | 0.866 | 3.91× |
| 8 | 0.489 | 6.93× |

```bash
for jobs in 1 2 4 8; do
  python3 benchmarks/run.py --lab Lab04 --case testcase2 --repeat 16 --jobs "$jobs" \
    --label "batch-$jobs" --output "benchmarks/work/scaling-$jobs.json"
done
```

Profiler 命令可參照原 source 快照，額外用 `-g -pg` 建置，於獨立工作目錄執行後
`gprof executable gmon.out`。不要混用 profiling binary 的時間來计算 release speedup。
大型 outputs/profile 原件保留在 ignored `benchmarks/work/`；可版控 JSON 僅包含小型 metadata 與 log。

## 第二輪：Lab01 邊界索引

[gprof 紀錄](../docs/profile-lab1.txt) 的 3,600-block 合成案例，65.82% samples 在全掃描 stitch repair。新版本依四種邊界座標找候選，保留最後匹配順序，並以指標索引移除 live tile；保留 `--stitches scan` 對照。

每模式／資料集三次，前版是同為 `-O3 -DNDEBUG` 的第一輪工程版；所有 36 次輸出通過獨立答案比對。索引增加記憶體，換取較少的全表走訪。

| Dataset | 前版秒 | 索引版秒 | 倍率 | 前版／索引版 RSS MiB |
|---|---:|---:|---:|---:|
| case7 | 0.03 | 0.02 | 1.50× | 3.65 / 3.99 |
| grid-400 | 0.03 | 0.02 | 1.50× | 3.68 / 3.84 |
| grid-1600 | 0.50 | 0.31 | 1.61× | 3.89 / 4.90 |
| grid-3600 | 2.63 | 1.35 | 1.95× | 4.27 / 6.47 |

[原始 JSON](results/lab1-stitches.json) 包含舊版、scan、indexed 三路。scan 路徑新增計數與 ownership 索引，不當成舊版時間；主表直接比較保存的前版 executable。小案例只有 0.01 秒精度，不以其倍率作強結論。此生成器是分離矩形，不代表所有幾何分布，最壞情況仍可能線性掃描邊界 bucket。當時 overlap 與 top/bottom 掃描尚未改成空間索引；續作已實作並另外量測。

```bash
python3 benchmarks/lab1_stitches.py --before /path/to/previous/Lab1
```

## 第三輪：Lab04 分層搜尋的品質／時間取捨

同一 Release build，兩模式各資料集三次，24 次皆通過官方 evaluator；輸出解不同，應比較合法性與成本，不要求相同 hash。layered 統計成本總和與官方加權成本吻合至輸出小數精度。

| Dataset | legacy cost | layered cost | 降低 | 時間秒：legacy → layered | RSS MiB：legacy → layered |
|---|---:|---:|---:|---:|---:|
| testcase0 | 1,796.74 | 1,780.09 | 0.93% | 0.00 → 0.00 | 3.77 → 3.60 |
| testcase1 | 6,872.82 | 6,341.41 | 7.73% | 0.00 → 0.01 | 3.75 → 3.85 |
| testcase2 | 174,838.81 | 136,686.48 | 21.82% | 0.15 → 0.32 | 4.80 → 4.89 |
| example | 30,526.50 | 29,101.68 | 4.67% | 0.01 → 0.02 | 3.90 → 3.99 |

短案例顯示 0.00 秒代表低於 GNU time 解析度，不代表無執行時間。layered 的 testcase2 明顯較慢；四組解的成本都降低，但這不是其他資料的品質保證，也不是多 net 全域最佳解保證。保留 legacy 為預設，可透過 `--router layered` 選擇新方案。

[legacy JSON](results/round3-legacy.json)、[layered JSON](results/round3-layered.json)、[圖模型及獨立 oracle](../docs/layered-routing.md)。

```bash
python3 benchmarks/run.py --lab Lab04 --repeat 3 --solver-arg=--router \
  --solver-arg=layered --solver-arg=--stats --label layered \
  --output benchmarks/work/layered.json
```

## 第四輪：Lab02 多 seed／資料／預算配對研究

`lab2_study.py` 使用三個事先選定的 seeds（1、7、19）、三組資料、兩個固定工作量（100,000／300,000 iterations）、兩條 HPWL 路徑，共 36 次。每對交換執行先後順序，全部序列執行。每 seed/預算/模式一次，這是擴展資料與 seed 的配對觀察，不是完整的硬體噪音統計。alpha=0.5，GCC 11.4 Release。

| Dataset | 迭代數 | 各模式合法 seeds | 合法 cost：min / median / max | strings 秒 | IDs 秒 | 配對倍率 |
|---|---:|---:|---|---:|---:|---:|
| ami33 | 100,000 | 0/3 | 無合法解 | 2.43 | 0.83 | 3.04× |
| ami33 | 300,000 | 3/3 | 663,037 / 675,750 / 691,967 | 7.20 | 2.28 | 3.16× |
| ami49 | 100,000 | 0/3 | 無合法解 | 6.54 | 2.72 | 2.40× |
| ami49 | 300,000 | 3/3 | 19,649,819 / 19,950,913 / 20,237,987 | 19.23 | 7.88 | 2.44× |
| vda317b | 100,000 | 2/3 | 19,295,928 / 20,520,326 / 21,744,724 | 6.39 | 6.36 | 1.00× |
| vda317b | 300,000 | 3/3 | 18,175,135 / 18,520,516 / 20,541,237 | 19.67 | 18.98 | 1.01× |

18 個配對的可行性完全相同；合法配對的 objective 與 placement hash 全部一致（排除 runtime 行）。22 次通過官方 verifier，14 次在工作量用盡時回報找不到合法解，沒有非預期失敗。失敗未被丟棄，也不把無解的短 runtime 描述為品質改善。表中的 runtime 包含所有嘗試；成本分布明確只取合法 seeds，不能拿不同合法率的列直接比較平均品質。

300,000 iterations 時，ami33 的 HPWL IDs 配對中位加速約 3.16×，ami49 約 2.44×；vda317b 沒有 nets，因此沒有 HPWL 查找可省，不宣稱顯著加速。前兩者在 100,000 iterations 的三個 seeds 都還沒找到合法解，300,000 才全部合法。vda317b 在較小預算的 seed=19 也無解。這些有限樣本支持使用明確的工作預算與記錄失敗率，不能推論成功率保證或跨平台結果。

[原始 JSON](results/lab2-study.json) 記錄所有合法性、seed、預算、compiler、flags、input/executable hash、objective/area/HPWL、時間、peak RSS 與 validator logs。`summary` 為可重算匯總；`pairs` 記錄配對等價性；只有 `complete=true` 才是完整研究。

```bash
python3 benchmarks/lab2_study.py --seeds 1 7 19 --iterations 100000 300000
# 自行擴大研究（會增加執行時間）
python3 benchmarks/lab2_study.py --seeds 1 7 19 31 43 \
  --iterations 300000 1000000 --output benchmarks/work/extended-study.json
```

CI 只負責正確性，不在共享 runner 上設硬性時間門檻；詳見 [CI 文件](../docs/ci.md)。

## Interview roadmap studies

The consolidated [report](../docs/experiments/README.md) links complete machine-readable
records and standalone SVG plots. New experiments preserve invalid/budget-exhausted
attempts and distinguish timing repetitions from solver-seed variation. Defaults
are not silently changed when a policy wins only a subset of workloads.

```bash
python3 benchmarks/routing_study.py --output benchmarks/work/routing.json
python3 benchmarks/routing_study.py --generated --budgets 1 --output benchmarks/work/congestion.json
python3 benchmarks/policy_study.py --suite tuning --output benchmarks/work/tuning.json
python3 benchmarks/policy_study.py --suite evaluation --output benchmarks/work/policies.json
python3 benchmarks/policy_study.py --suite instrumentation --output benchmarks/work/instrumentation.json
python3 benchmarks/legalizer_study.py --output benchmarks/work/legalization.json
python3 benchmarks/legalizer_study.py --generated --output benchmarks/work/legalization-generated.json
python3 benchmarks/lab1_distributions.py --output benchmarks/work/distributions.json
# Aggregate the checked-in final study filenames; matplotlib is needed only here.
python3 benchmarks/report.py
```

`policy_study.py` declares tuning seeds 1/7 on ami33; evaluation seeds 101/211/307/409
on three official and two known-feasible generated floorplans. `generated.py` uses
self-authored deterministic generators; no extra downloaded benchmark licensing is
assumed. `routing_study.py` uses four generated congestion seeds. Lab03 uses 40/70/90%
occupancy targets and single/double-height cells, with 80 sequential banking steps.
Raw outputs and validator logs remain in ignored per-run directories; hashes, metrics,
commands, failure reasons and solver diagnostic records are retained in JSON.

Run studies serially without concurrent builds. Cooperative routing wall budgets
include initial routing but exclude parse/report; floorplanning budgets are per-restart
CPU seconds. Wrapper wall and GNU user/system CPU time are separate fields. The host
is shared: reported ranges are samples, not calibrated performance gates.

For local GCC/Clang/ASan/UBSan/TSan/fuzz/static-analysis validation:

```bash
python3 scripts/validate_portfolio.py --jobs 4
```

TSan requires a supported runtime and is not considered passed if skipped. Fuzz
corpora are copied before mutation; four parser targets have explicit input, grid,
RSS, mutation and time bounds. Compiler/runtime tools are development dependencies
(`clang-14`, `libclang-rt-14-dev` on Ubuntu 22.04), not production dependencies.
