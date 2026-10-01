# IEE PDA_Lab4 Die-to-Die Global Routing
:::warning 
:::spoiler Update log  
2024 11/23 18:45 GMT+8: Update input format [GridMap](#GridMap)  
2024 11/25 01:13 GMT+8: Update input format [Cost](#Cost)  
2024 11/25 02:11 GMT+8: Update [Problem Formulation](#Problem-Formulation) - Definition of $OV$  
2024 11/25 12:22 GMT+8: Update [Output Format](#Output-Format) - Description  
2024 11/25 16:22 GMT+8: Update [Q&A](#QampA) - Q1, Q2  
2024 11/25 17:05 GMT+8: Update [Q&A](#QampA) - Q3  
2024 11/27 13:18 GMT+8: Update [Q&A](#QampA) - Q4  
2024 11/27 15:41 GMT+8: Update [Q&A](#QampA) - Q5  
2024 11/27 22:29 GMT+8: Update [Q&A](#QampA) - Q6  
2024 11/28 15:55 GMT+8: Update [Q&A](#QampA) - Q7  
2024 12/1  03:00 GMT+8: Update [Q&A](#QampA) - Q8, Q9  
2024 12/4  01:04 GMT+8: Update [Evaluator](#Evaluator)  
2024 12/8  17:04 GMT+8: Update runtime limit up to ==600 seconds==  
2024 12/9  19:43 GMT+8: 
The largest hidden case scale will be:  
    $$\frac{RoutingArea_{width}}{GCell_{width}} \times \frac{RoutingArea_{height}}{GCell_{height}} = 1500 \times 1000$$  
    *Note*: The aspect ratio of $RoutingArea$ may change  
2024 12/19  09:34 GMT+8: Update [Q&A](#QampA) - Q10, Q11  
2024 12/22  15:33 GMT+8: Update runtime limit up to ==1200 seconds (20 minutes)==  
:::

## Table of Contents
- [Introduction](#Introduction)
    - [GCell](#GCell)
    - [A* Search](#A-Search-4)
- [Problem Formulation](#Problem-Formulation)
- [Routing Constraint](#Routing-Constraint)
- [Input Format](#Input-Format)
    - [GridMap](#GridMap)
    - [GCell](#GCell14)
    - [Cost](#Cost)
- [Output Format](#Output-Format)
- [Execution](#Execution)
- [Grading](#Grading)
- [Submit](#Submit)
- [Evaluator](#Evaluator)
- [Q&A](#QampA)
- [Contacts](#Contacts)



## Introduction
Global Routing (GR) is an important step in the IC design flow used to determine the approximate paths for signal nets across the design space. This process mainly focuses on assigning "routing resources" and providing a "coarse routing solution" for subsequent Detailed Routing (DR). Its primary goals include:

* **Path Planning**:  
    Determine a global connection path for each signal net *without specifying the exact placement of wires.*
    
* **Resource Allocation**:  
    Ensure that routing resources (such as metal layers and routing tracks) are utilized efficiently across the design to avoid congestion.

* **Feasibility Check**:  
    Provide a congestion map to evaluate whether the design is routable and offer optimization suggestions.

* **Performance Optimization**:
    Minimize routing length and total delay while meeting requirements for timing, power, and reliability.


---

### GCell
Here's a brief introduction to GCell:  
![image](https://hackmd.io/_uploads/HySK-crMke.png)


Each cell has four surrounding edges, and each of edge has its own capacity, which implies the maximum number of nets that current edge can accept. There also exists GCell that use cell capacity (or even both) to simulate the routing conjestion.  
There exists many types of GCell shapes, including rectangular, triangular[^triangularGCell], Hanan-Grid based [^hananWiki] [^hananPaper], etc.

---

### A* Search [^AstarSearch]
A* Search is a heuristic search algorithm used to find the shortest path in graphs or grids. It combines the strengths of breadth-first search and heuristic search, making it efficient and widely applicable.
#### Core Concept
A* uses an evaluation function $\hat{f}(n)$ to select the most optimal node for expansion.
$$\hat{f}(n) = \hat{g}(n) + \hat{h}(n)$$
* $\hat{g}(n)$: The actual cost from the start node to the current node $n$
* $\hat{h}(n)$:  The estimated cost from the current node $n$ to the goal node (heuristic function).
#### Key Features
1. **Optimality**: If the heuristic function $\hat{h}(n)$ is consistent or *admissible*, A* search is guarantees finding the shortest path
2. **Efficiency**: By using $\hat{h}(n)$ to guide the search, A* is faster than pure breadth-first search (i.e. Maze route, or Lee's Algorithm)
3. **Flexibility**: The design of the heuristic function affects both the speed and the outcome of the search  
* **Note**: We call an search algorithm *admissible* if it is guaranteed to find an optimal path from source to a preferred goal node 

#### Algorithm
:::info
![image](https://hackmd.io/_uploads/H1lFnRNifJe.png)
* $T$ is the set of terminal points of source $s$
* Define $e_{pq}$ as an arc from node $n_p$ to $n_q$. If $e_{pq}$ exists, we call $n_q$ a successor of $n_p$
* $\mathit{\Gamma}(n)$ is a function which returns all successors of node $n$
* By graph theory, we can prove that it is impossible for a "closed" successor to have a smaller $\hat{f}$ than the current point. Thus, the $4^{th}$ step can be rewritten as:  
<mark> Otherwise, mark $n$ "closed" and apply $\mathit{\Gamma}(n)$, mark each successor as "open" if it is not already marked as "closed". Finally, go to step 2.


<div style="text-align: center;">

![Astar_Animation](https://upload.wikimedia.org/wikipedia/commons/5/5d/Astar_progress_animation.gif)

*Source: [Wikipedia](https://en.wikipedia.org/wiki/A*_search_algorithm), licensed under CC BY-SA 3.0.*

</div>
:::



#### Advantages
* Effectively finds the shortest path
* Incorporates heuristic information to reduce the search space

#### Disadvantages
* Memory consumption can be high when the search space is large
* Poorly designed heuristic functions $\hat{h}(n)$ may lead to inefficiency
    * The smaller $\hat{h}(n)$, the more nodes need to be calculated, and will results in low efficiency of the algorithm
    * When $\hat{h}(n)=0$, A* algorithm is same as Dijkstra's algorithm [^Dijkstra-Algorithm]







## Problem Formulation
During the GR step, we use GCells to approximate wire allocation. In this lab, we choose a rectangular grid as the shape of the GCells for simplicity.  
![image](https://hackmd.io/_uploads/ryZjypSGke.png)


There will be **two chips** with several bumps, while each bump in the current chip has another corresponding bump in the other chip. In this lab, your task is to complete a die-to-die global routing using the A*-search algorithm[^AstarSearch](i.e. to find a minimum-cost path between each pair of bumps).  

Your original cost can be calculated by using following formula:  


$$ oriCost = \alpha\cdot WL_{total} + \beta\cdot OV_{total} + \gamma\cdot cellCost_{total} + \delta \cdot viaCost_{total}$$

where $WL$ is wire length, $OV$ is overflow (i.e. the number of the nets on the edge exceeds its capacity). 

Also define the timing factor as below:
$$ T = 0.02 * log_2(\frac{T_{yours}}{T_{medium}})$$
$$ runtime\_factor = min(0.1, max(-0.1, T))$$

where $T_{yours}$ and $T_{medium}$ is the runtime of yours and medium runtime of all submitted global routers from contestants for current testcase, respectively


$$ finCost = (1 + runtime\_factor)\cdot oriCost$$




* The lower your $finCost$, the higher your rank.
* $\alpha$, $\beta$, $\gamma$ and $\delta$ will be given in [*.cst](#Cost) with $cellCost$ as .alpha, .beta, .gamma and .delta, respectively
* Define $OV=(netNum-edge_{capacity})*0.5*max(cellCost_{initial})$  
Ex: Consider a placement given by testcase with the largest cell cost 30 and an edge $e$ with capacity 3 passed by 5 net. We get 
$$OV_e = (5-3)*0.5*30=30$$



## Routing Constraint
1. In this lab, you can only route in 2 metal layers with restricted direction:  
    * M1: Vertical
    * M2: Horizontal
2. Do not route outside the routing area
3. All of the nets should be routed



## Input Format
<!--=====================================================-->
<!--===================    *.gmp    =====================-->
<!--=====================================================-->
### GridMap
Note that coordinates of chips and bumps are relative to the lower-left of the routing area and corresponding chips, respectively  

* Example:    
$routingArea_{corrdinate} = (15,40), Chip = (20,0), bump_{coordinate} = (30,110)$  
then the real chip position is $(15+20,40+0) = (35,40)$,  
real bump position is $(15+20+30,40+0+110) = (65,150)$

:::info
:::spoiler gridMap.gmp
```
.ra               // routing area
15 40 410 310     // <lowerleft X> <lowerleft Y> <width> <height>
.g                // grid information
10 10             // <grid width> <grid height>
.c                // chip1 information  
20 0 100 160      // <lowerleft X> <lowerleft Y> <width> <height>
.b                // bump information of chip1
1 50 110          // <bumpIdx> <lowerleft X> <lowerleft Y>
...        
6 90 130
                  // an empty line
.c                // chip2 information
...        
.b                // bump information of chip2
...
:::



<!--=====================================================-->
<!--==================      *.gcl     ===================-->
<!--=====================================================-->
### GCell
![image](https://hackmd.io/_uploads/Bk3DyePMkl.png)

* Each GCell will be given its left and bottom edge capacity
* GCell will be given in raster scan order  
Ex: $GCell_1$ will share the horizontal edge capacity with $GCell_2$ and $GCell_{42}$ will share the vertical edge capacity with $GCell_1$
* TA won't give you the top and right boundary capacity of whole routing area, since it is invalid to route outside routing area

:::info
:::spoiler GCell.gcl

```
.ec        // edge capacity 
3 3        // gcell 1  <leftEdgeCapacity> <bottomEdgeCapacity>
4 3        // gcell 2  ..
.
.
.
3 3        // gcell 41 
4 5        // gcell 42 
.
.
6 4        // gcell 1271

```
:::


<!--=====================================================-->
<!--================       *.cst       ==================-->
<!--=====================================================-->
### Cost
* All cell costs will be given as following rules:
    * From left to right
    * From bottom to top
    
:::info
:::spoiler cost.cst
```
.alpha 0.7
.beta 1.1
.gamma 1.0
.delta 1.0
.v            // via cost
10
.l            // layer 1
10 13 ... 8   // costs of bottom grids (total 410/10 = 41 unsigned int) 
...
(another 30 rows of costs)
.l            // layer 2
...
(31 rows of costs)
```
:::




## Output Format
* Please print the net name as $nx$, where $x$ is the corresponding bump index.
    * If current net is used to connect $bump_1$, then print $n1$
* When changing layer, remember to place a via
* Every net should start from $M1$, and end at $M1$
* Print the path from $Chip_1$ to $Chip_2$ 
* The position in *.lg should be 
    * real position, not the relative position
    * the lower-left corner of the start/end GCell

Following is the format and an example of *.lg:
```
nx
<layer> <start x> <start y> <end x> <end y>
via
```

:::info 
:::spoiler Example of output.lg
```
n1
via                 // if there's a via, you should print it
M2 65 150 355 150   // a wire in M2 from (65,150) to (355,150)
via                 
M1 355 150 355 210
.end                // print .end to finish the output of n1
n2                  // start of next net
...
```
:::



## Execution
Your binary file should be executed by following command:
```
$ ./D2DGRter *.gmp *.gcl *.cst *.lg
```
where *.gmp, *.gcl and *.cst are inputs from different testcases, *.lg is your routing result.  
Ex:  
```
$ ./D2DGRter testcase1.gmp testcase1.gcl testcase1.cst testcase1.lg
```
* **Note**: If parallelization is implemented, only up to **4** threads are allowed. Make sure to limit the number of threads in your program; otherwise, TAs have the right to terminate your program directly



## Grading
* Three public cases (60%) and two hidden cases (40%).  
    * For each case, correctness (70%) and performance (30%).
    * Performance is based on your [$finCost$](#Problem-Formulation) for the current case. Only students with a valid routing result will be included in the ranking.

* Cheating or plagiarism will result in a score of 0 for this lab.
* Runtime should less than ==1200 seconds (20 minutes)==; otherwise, you'll fail the case
* For other routing constraints, please refer to [Routing Constraint](#Routing-Constraint)



## Submit
* <mark> **Duration**: From **2024/11/25 12:00** to **2024/12/16 23:59**
* The top directory should be named as $studentID$
* The directory should contain
    * *.h/ *.hpp (optional)
    * `readme.txt`/ `README.md` (optional)
    * *.cpp
    * `Makefile`  
    
    Your directory may looks something like:  
    ```
    📁312510224/  
    ├── 📁inc/  
    │    └── globalRouting.h  
    ├── 📁src/  
    │    └── globalRouting.cpp  
    ├── main.cpp
    ├── Makefile
    └── README.md  
    ```
* Please tar the file by following command:
    ```
    $ tar cvf studentID.tar studentID
* Remember to submit your `studentID.tar` to [newE3](https://portal.nycu.edu.tw/#/login)

## Evaluator
### Preparation
1. Put all input files and your `.lg` file into a directory
2. Make sure that all of your files have the same names

    For example:
    ```
    📁publicCase/  
        └── testcase0.gmp  
        └── testcase0.gcl
        └── testcase0.cst
        └── testcase0.lg
    ```

### Execution
    ```
    $ git clone https://github.com/YubiYubi719/NYCU-PDA-Lab4-Evaluator.git
    $ cd NYCU-PDA-Lab4-Evaluator
    $ tar xvf Evaluator.tar
    $ cd Evaluator
    $ chmod 755 Evaluator
    $ ./Evaluator <fileDirPath> <testcaseName>

    // Ex:
    // ./Evaluator ~/publicCase testcase0
    ```

### Output Result
If you see a pretty table like this, congratulations!  
(Note: If the color of Overflow in Item column is red, don't worry!)  
![image](https://hackmd.io/_uploads/BJJjTn2mkx.png)


### Bonus
<mark>If you find any bugs, feel free to contact us! Once your observation is verified, you will receive 1 bonus point for this lab.



## Q&A
:::spoiler 1. Edge capacity 定義
**Q**:  
助教您好,  
關於作業的描述，$GCell_1$ 跟 $GCell_2$ 會share horizontal capacity，想跟助教確認一下，您的意思是，$GCell_1$的right edge capacity $=GCell_2$ 的left edge capacity嗎?  
  
**A**:  
沒錯!  
:::

:::spoiler 2. Evaluator
**Q**:  
助教您好  
想請問Lab4會提供Evaluator嗎？
  
**A**:  
會喔
:::

:::spoiler 3. cost.cst 勘誤
**Q**:  
助教您好，  
關於 [Cost](#Cost) 的章節裡提供了示意用的 cost.cst 範例。但根據前後文，整個 routing area 應該只有 31 個 rows，那麼範例中：  

(another 40 rows of costs)  
是否應為  
(another 30 rows of costs)  

（同理，  

(41 rows of costs)  
是否應改為  
(31 rows of costs)  

**A**:  
同學您好，謝謝告知，這邊會再進行更正!  
:::

:::spoiler 4. 繞線時是否可以直接跨越其他bump所在之GCell?
**Q**:  
助教您們好，想詢問一下bump在route其他bump的時候是可以經過的嗎？還是需要繞過呢？(route bump 2是否可經過bump1)，謝謝  

**A**:  
同學您好，可以直接經過!  
:::

:::spoiler 5. GCell cost 與 via cost 之計算方式
**Q**:  
助教您好  
想跟您確認cst檔案的問題  
```
cost.cst
.alpha 0.7
.beta 1.1
.gamma 1.0
.delta 1.0
.v            // via cost
10
.l            // layer 1
10 13 ... 8   // costs of bottom grids (total 410/10 = 41 unsigned int) 
...
(another 30 rows of costs)
.l            // layer 2
...
(31 rows of costs)
```

1. layer1 layer2指的是當Ｍ1 M2穿越時所造成的成本嗎？
比如說當M1穿過左下第一個的話看的就是layer1在左下第一個的成本(10)嗎？  
2. 然後via佔據的cell就是算via的成本不算cost，這樣理解對嗎？  

**A**:  
同學您好，謝謝您重要的發問!  
第一點是正確的，第二點則需要更正，請參考下列範例:  
$M2$:  
![image](https://hackmd.io/_uploads/HyuUrBN7Je.png)  
$M1$:  
![image](https://hackmd.io/_uploads/Hy0PBr4Qke.png)  
其中橘色為起點，黃色為終點，暫不考慮$WL$與$OV$之cost。  
定義參數如下:  
$$
\begin{cases} 
    \gamma = 0.7 \\
    \\
    \delta = 1.3
\end{cases}
$$

因此$M1$之cost可以計算為:  
$$ \gamma\cdot (2+2+6+4+1) = 0.7 \cdot 15 = 10.5 $$
然後需要一個via進行layer轉換，我們定義有via之GCell的cost為:  
$$cost_{GCell\_with\_via} = \gamma\cdot \frac {cost_{M1} + cost_{M2} }{2} + \delta\cdot viaCost$$
因此綠色格子的cost為:  
$$ \gamma \cdot \frac{7+3}{2} + \delta \cdot 3.5 = 0.7 \cdot 5 + 1.3 \cdot 3.5 = 8.05 $$  
接下來是$M2$之cost:  
$$\gamma \cdot (4+2+1) = 0.7 \cdot 7 = 4.9 $$
最終在終點處(黃色)透過via回到$M1$:  
$$\gamma \cdot \frac{5+4}{2} + \delta \cdot 3.5 = 0.7 \cdot 4.5 + 1.3 \cdot 3.5 = 7.7 $$  
:::


:::spoiler 6. 起點、終點之via疑問
**Q**:  
想請問這段話  
[Every net should start from $M1$, and end at $M1$](#Output-Format)  
我的理解是M1是垂直線M2是水平線  
這段話的意思是在指說，每一個net最一開始都是垂直線，如果要切換到水平線就算是一開始都要直接via嗎？  
而如果終點是用水平線插入的話是不被允許的只能用垂直線插入嗎？如圖所示  
![image](https://hackmd.io/_uploads/rk0EijV7Jl.png)  

**A**:  
同學您好，是可以允許是水平線進入終點的 (亦可水平由起點出發，如同[Output Format](#Output-Format) 中 output.lg 的 $n1$ )! 但是由於水平走線是在$M2$，因此需要再打一個via回到$M1$。  
以您的截圖為例，第一種走線方式出發是走水平線($M2$)，因此需要在起點打上一顆via，進入終點是鉛直走線($M1$)，因此就不再需要via以回到$M1$。  
第二種走線則相反，出發時不需要via(鉛直走線，即由M1出發)，水平進入終點時，需要一個via連結$M2$的metal與終點之bump。  
:::


:::spoiler 7. 有關Q5之疑問
**Q**:  
助教們好  
想請問有關你們針對第五題的回答  
M1 cost和M2 cost具體的計算方式好像不太一樣  
一個是2+2+6+4+1(有包含橘色的位置)，另一個是4+2+1(沒有包含顏色區域)。  
M1和M2的cost是只要算圖片中沒有被標上顏色的區域?還是有特別的規定  
具體是要如何計算?謝謝助教們  

**A1**:  
同學您好，標示不同顏色想要表達的是計算cell cost的時候，若當前cell有via，則需要使用Q5中的公式計算。  
因此圖中的綠色格子與終點之黃色格子都是有via的，其cell cost需要另外計算。  
而起點雖然被標示橘色，但是該處並沒有via，因此可以直接將cost納入M1中一併計算。  
**A2**:  
同學你好，可以不用想得太複雜。  
簡單來說就是當metal走在$M1$時就只需要計算$M1$的gcell cost，走$M2$就只需要計算$M2$的gcell cost，唯一的例外就是當走線換層時(也就是via的位置)的gcell cost 是 $M1+M2$的gcell cost 除以2。  

:::


:::spoiler 8. 是否一定要使用A* search演算法?
**Q**:  
助教你好，  
想問這次作業有規定一定要用A* search的方式做routing嗎?  
附圖為題目敘述的problem formulation有寫用A* search。  
![image](https://hackmd.io/_uploads/SJPNxyKQkl.png)  
**A**:  
同學您好，老師是希望藉由此次lab讓同學們熟悉A* search之演算法。  
但是並不一定強制要使用，同學也可以自行設計routing演算法!  
:::

:::spoiler 9. 測資之座標是否為整數?
**Q**:  
助教你好，  
想請教座標值有一定是整數嗎?    
![image](https://hackmd.io/_uploads/SJPNxyKQkl.png)  
**A**:  
是的，所有除了cell cost以外的資料都是整數型態  
:::

:::spoiler 10. *.gmp 中 bump index 之順序
**Q**:  
助教們好  
想請問.gmp檔裡兩個chip的bump information都是照ascending的順序由index為1開始給值，且每次index的數值都遞增1嗎？  
**A**:  
同學您好，是的!  
都是由1開始，且不會有跳號情況發生。
:::

:::spoiler 11. Overflow Cost計算疑問
**Q**:  
助教們好～  
想請問Overflow計算的細節：  
$max(cellCost_{initial})$是指兩層全部的gcell中cost的最大值嗎？還是是同一層Metal中gcell中cost的最大值？  
**A**:  
同學你好， 是兩層全部cell中cost的最大值。
:::


## Contacts
**Reminder**: Please send your questions (or maybe some requests) to both TAs, we'll try our best to answer you within 24 hours. Questions will also be updated in [Q&A](#QampA)
* TA1: 林煜睿, yrlin719.ee12@nycu.edu.tw
* TA2: 陳煥沅, ryan.chen.1104@gmail.com




[^triangularGCell]: https://ieeexplore.ieee.org/abstract/document/10247899
[^hananWiki]: https://en.wikipedia.org/wiki/Hanan_grid
[^hananPaper]: https://ieeexplore.ieee.org/abstract/document/10652868
[^AstarSearch]: https://ieeexplore.ieee.org/document/4082128
[^Dijkstra-Algorithm]: https://zh.wikipedia.org/wiki/%E6%88%B4%E5%85%8B%E6%96%AF%E7%89%B9%E6%8B%89%E7%AE%97%E6%B3%95