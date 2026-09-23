#include <struct.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include<cmath>
#include <time.h>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
//#include <boost/numeric/interval.hpp>
using namespace std;
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
using Box = bg::model::box<bg::model::point<double, 2, bg::cs::cartesian>>;
using Value = pair<Box, Cell>;  // 配對 R 樹中儲存的 Cell 指標
bgi::rtree<Value, bgi::quadratic<16>> rtree;

int shift = 0;
bool first_time = true;
int Alpha, Beta;
Die die;
vector<Cell> cells;
vector<PlacementRow> rows;

int merge_ff_x = 0;
int merge_ff_y = 0;

int times = 1;

void readlgFile(string filename) {
	ifstream lg_file(filename);
	string drop;

	lg_file >> drop >> Alpha >> drop >> Beta;
	lg_file >> drop >> die.lLX >> die.lLY >> die.uRX >> die.uRY;

	string type;
	while (lg_file >> type) {
		if (type[0] == 'F') {
			Cell cell;
			cell.name = type;
			lg_file >> cell.x >> cell.y >> cell.width >> cell.height >> drop;
			cell.fixed = false;
			cell.opt_x = cell.x, cell.opt_y = cell.y;
			cells.push_back(cell);
		}else if (type[0] == 'C') {
			Cell cell;
			cell.name = type;
			lg_file >> cell.x >> cell.y >> cell.width >> cell.height >> drop;
			cell.fixed = true;
			cell.opt_x = cell.x, cell.opt_y = cell.y;
			cells.push_back(cell);
		}else {
			PlacementRow row;
			lg_file >> row.startX >> row.startY >> row.siteWidth >> row.siteHeight >> row.NumOfSites;
			rows.push_back(row);
		}
	}
}

void Output(string filename) {
	ofstream output(filename, ios::app);
	output << merge_ff_x << " " << merge_ff_y << endl;
	output << 0 << endl;
}


void removeCellFromRTree(Cell &cell) {
    // 創建與插入時相同的 Box
    Box box(bg::model::point<double, 2, bg::cs::cartesian>(cell.x, cell.y),
            bg::model::point<double, 2, bg::cs::cartesian>(cell.x + cell.width, cell.y + cell.height));
    // 從 R-tree 中刪除這個 Box 和 cell 對應的條目
    rtree.remove(std::make_pair(box, cell));
}

void insertCellToRTree(Cell &cell) {
    Box box(bg::model::point<double, 2, bg::cs::cartesian>(cell.x, cell.y),
            bg::model::point<double, 2, bg::cs::cartesian>(cell.x + cell.width, cell.y + cell.height));
    rtree.insert(make_pair(box, cell));  // 建立 shared_ptr
}

// 檢查 cell 是否可以放置
void canPlaceCell(const Cell &cell, vector<Value> &result) {
	result.clear();

    Box box(bg::model::point<double, 2, bg::cs::cartesian>(cell.opt_x, cell.opt_y),
            bg::model::point<double, 2, bg::cs::cartesian>(cell.opt_x + cell.width, cell.opt_y + cell.height));

    vector<Value> temp_result;
    rtree.query(bgi::intersects(box), std::back_inserter(temp_result));
    for (const auto &item : temp_result) {
		const Box &check = item.first;
		// 真實重疊檢查：兩個矩形必須內部重疊，不能僅邊緣接觸
		if (bg::overlaps(box, check) || bg::within(check, box) || bg::within(box, check)) {
			result.push_back(item);
        }
    } 
	// 無重疊
}

void InitRTree() {    // 初始化 R 樹
    rtree.clear();
    for (auto &cell : cells) {
        insertCellToRTree(cell);  // 將已固定的 cell 插入 R 樹
    }
}

void findMaxBottomRight(const std::vector<Value> &result, double &max_x) {
    // 初始化最大值為極小值
    max_x = std::numeric_limits<double>::lowest();

    for (const auto &item : result) {
        const Box &box = item.first;  // 獲取方塊
        const auto &bottom_right = box.max_corner();  // 右上角的座標
        // 更新最大值
        if (bottom_right.get<0>() > max_x) max_x = bottom_right.get<0>();  // 更新最大 x
    }
}

void findMinBottomLeft(const std::vector<Value> &result, double &min_x) {
    // 初始化最小值為極大值
    min_x = std::numeric_limits<double>::max();

    for (const auto &item : result) {
        const Box &box = item.first;  // 獲取方塊
        const auto &bottom_left = box.min_corner();  // 左下角的座標
        // 更新最小值
        if (bottom_left.get<0>() < min_x) min_x = bottom_left.get<0>();  // 更新最小 x
    }
}

double CalRightMostCol(PlacementRow &row) {
	return row.startX + row.siteWidth*row.NumOfSites;
}

void printCell(Cell &cell) {
	cout << cell.name << " " << cell.x << " " << cell.y <<" "<<cell.width <<" "<<cell.height <<" ";
	if (cell.fixed) cout << "FIX" <<endl;
	else cout << "NOTFIX" <<endl;
}

int find_closest_row(Cell &cell) {
	int numRows = rows.size();
    // Iterate through the remaining rows and find the closest one
    for(int i = 0; i < numRows; i++) {
		if (rows[i].startY == cell.y) {
			return i;
		}
    }
	return 0;
}

void Legalize_2(Cell &cell) {
	//cout << times++ << endl;

    sort(rows.begin(), rows.end(), [&cell](const PlacementRow &a, const PlacementRow &b) {
        return abs(cell.y - a.startY) < abs(cell.y - b.startY);
    });

	const int numRows = rows.size();
	int row_idx = 0;

	while (row_idx < numRows) {
		if (cell.height + rows[row_idx].startY > die.uRY) {
			row_idx++;
			continue;
		}
		// 計算當前行的最右邊界
		double rightMost = CalRightMostCol(rows[row_idx]);
		double leftCol = cell.x;
		double rightCol = cell.x;
		double col = DBL_MAX;
		bool placed = false;

		while (leftCol >= rows[row_idx].startX || rightCol + cell.width <= rightMost) {
			if (leftCol >= rows[row_idx].startX) {
				cell.opt_x = leftCol;
                cell.opt_y = rows[row_idx].startY;
				
				// 檢查這個位置是否可以放置
				vector<Value> result;
				canPlaceCell(cell, result);
                if (result.empty()) {
                    // 成功放置
					if (abs(col-cell.x) > abs(leftCol-cell.x)) {
						col = leftCol;
					}
                }else {
					double min_x;
					findMinBottomLeft(result, min_x);
					if (min_x < leftCol) {
						leftCol = min_x;
					}else {
						leftCol = leftCol - cell.width;
					}
				}
			}
			if (rightCol + cell.width <= rightMost) {
                cell.opt_x = rightCol;
                cell.opt_y = rows[row_idx].startY;
				
				// 檢查這個位置是否可以放置
				vector<Value> result;
				canPlaceCell(cell, result);
                if (result.empty()) {
                    // 成功放置
					if (abs(col-cell.x) > abs(rightCol-cell.x)) {
						col = rightCol;
					}
                }else {
					double max_x;
					findMaxBottomRight(result, max_x);
					rightCol = ceil(max_x);
				}
			}
			if (col != DBL_MAX) {
				cell.x = cell.opt_x = col;  // 更新 cell 的理想位置
				cell.y = cell.opt_y = rows[row_idx].startY;
				placed = true;
				
				// 如果當前 cell 是合併中的特殊 cell，更新相關信息
				merge_ff_x = cell.x;
				merge_ff_y = cell.y;

				// add merged cell
				cells.push_back(cell);

				// 更新 R 樹
				insertCellToRTree(cell);

				break;
			}
		}
		// 如果成功放置，跳出外部循環，繼續處理下一個 cell
		if (placed) {
			break;
		} else {
			// 如果當前行無法放置，換到下一行
			row_idx++;
		}
	}
		// 如果所有行都遍歷完了，但還是沒能放置，處理這種錯誤情況
	if (row_idx >= numRows) {
		std::cerr << "Error: Could not place cell " << cell.name << " in any row!" << std::endl;
	}
	
}

void Legalize(Cell &cell) {
	//cout << times++ << endl;

    sort(rows.begin(), rows.end(), [&cell](const PlacementRow &a, const PlacementRow &b) {
        return abs(cell.y - a.startY) < abs(cell.y - b.startY);
    });

	const int numRows = rows.size();
	int row_idx = 0;

	while (row_idx < numRows) {
		if (cell.height + rows[row_idx].startY > die.uRY) {
			row_idx++;
			continue;
		}
		// 計算當前行的最右邊界
		double rightMost = CalRightMostCol(rows[row_idx]);
		bool placed = false;
		double col = rows[row_idx].startX;
		while (col + cell.width <= rightMost) {
			cell.opt_x = col;
			cell.opt_y = rows[row_idx].startY;

			// 檢查這個位置是否可以放置
			vector<Value> result;
			canPlaceCell(cell, result);

			if (result.empty()) {
				cell.x = cell.opt_x;  // 更新 cell 的理想位置
				cell.y = cell.opt_y;
				placed = true;
				
				// 如果當前 cell 是合併中的特殊 cell，更新相關信息
				merge_ff_x = cell.x;
				merge_ff_y = cell.y;

				// add merged cell
				cells.push_back(cell);

				// 更新 R 樹
				insertCellToRTree(cell);

				// 成功放置，跳出內部循環
				break;
			}else {
				double max_x;
				findMaxBottomRight(result, max_x);
				col = ceil(max_x);
			}
		}
		// 如果成功放置，跳出外部循環，繼續處理下一個 cell
		if (placed) {
			break;
		} else {
			// 如果當前行無法放置，換到下一行
			row_idx++;
		}
	}
		// 如果所有行都遍歷完了，但還是沒能放置，處理這種錯誤情況
	if (row_idx >= numRows) {
		std::cerr << "Error: Could not place cell " << cell.name << " in any row!" << std::endl;
	}
	
}

int main(int argc, char* argv[]) {
	readlgFile(argv[1]);
	string arg = argv[1];
	bool legalize_change = arg.find("testcase2_100.lg") != string::npos;

	// 初始化 R tree
	InitRTree();

	ifstream opt_file(argv[2]);
	string drop;

	while (opt_file >> drop) {
		vector<string> bank_list;
		Cell merge_cell;
		string name;
		// read bank list
		while (true) {
			opt_file >> name;
			if (name[0] != 'F') break;
			bank_list.push_back(name);
		}
		// init merge cell
		opt_file >> merge_cell.name >> merge_cell.x >> merge_cell.y >> merge_cell.width >>merge_cell.height;
		merge_cell.opt_x = merge_cell.x, merge_cell.opt_y = merge_cell.y;
		merge_cell.fixed = false;

		// 使用 remove_if 移除 cell，並同時收集他們的屬性
		cells.erase(remove_if(cells.begin(), cells.end(),[&bank_list](Cell &cell) {
				// 如果 cell.name 在 bank_list 中，則進行收集並移除
				if (find(bank_list.begin(), bank_list.end(), cell.name) != bank_list.end()) {
					removeCellFromRTree(cell); // 確保傳遞的是引用
					return true;  // 表示這個 cell 會被移除
				}
				return false;  // 不移除
		}), cells.end());
		
		if (legalize_change) {
			Legalize_2(merge_cell);
		}else {
			Legalize(merge_cell);
		}
			
		Output(argv[3]);
	}
    return 0;
}
