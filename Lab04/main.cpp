#include <iostream>
#include <algorithm>
#include <queue>
#include <cmath>
#include <unordered_map>
#include <climits>
#include <iostream>
#include <fstream>
#include <cfloat> 
#include "struct.hpp"
using namespace std;

RoutingAreaInfo RA_info;
GridInfo grid_info;
Chip chip1, chip2;
vector<Net> nets;
vector<vector<GCell>> gcells;
CostInfo cost_info;


void readFileGmp(string filename) {
	ifstream gmp_file(filename);
	string drop;
	// read routing area info
	gmp_file >> drop;
	gmp_file >> RA_info.Routing_Area_X >> RA_info.Routing_Area_Y >> RA_info.Routing_Area_Width >> RA_info.Routing_Area_Height;
	// read grid info
	gmp_file >> drop;
	gmp_file >> grid_info.GridWidth >> grid_info.GridHeight;
	// cal routing area row & col
	RA_info.num_rows = RA_info.Routing_Area_Height / grid_info.GridHeight;
	RA_info.num_cols = RA_info.Routing_Area_Width / grid_info.GridWidth;
	// read  chip 1 info 
	gmp_file >> drop;
	gmp_file >> chip1.x >> chip1.y >> chip1.width >> chip1.height;
	// read chip 1 bump info
	gmp_file >> drop;
	while (true) {
		gmp_file >> drop;
		if (drop != ".c") {
			Net net;
			int bump1_x, bump1_y;
			gmp_file >> bump1_x >> bump1_y;
			net.idx = stoi(drop);
			net.bump1_x = RA_info.Routing_Area_X + chip1.x + bump1_x;
			net.bump1_y = RA_info.Routing_Area_Y + chip1.y + bump1_y;
			nets.push_back(net);
		}else break;
	}
	// read chip 2 info
	gmp_file >> chip2.x >> chip2.y >> chip2.width >> chip2.height;
	// read chip 2 bump info
	gmp_file >> drop;
	while (gmp_file >> drop) {
		int idx = stoi(drop);
		int bump2_x, bump2_y;
		gmp_file >> bump2_x >> bump2_y;
		nets[idx-1].bump2_x = RA_info.Routing_Area_X + chip2.x + bump2_x;
		nets[idx-1].bump2_y = RA_info.Routing_Area_Y + chip2.y + bump2_y;
	}

}

void readFileGcl(string filename) {
	ifstream gcl_file(filename);
	string drop;
	gcl_file >> drop;
	int left_c, bottom_c;
	for (int row=0; row<RA_info.num_rows; row++) {
		vector<GCell> v;
		for (int col=0; col<RA_info.num_cols; col++) {
			gcl_file >> left_c >> bottom_c;
			GCell gcell;
			gcell.left_capacity = left_c;
			gcell.bottom_capacity = bottom_c;
			gcell.left_usage = gcell.bottom_usage = 0;
			gcell.x = col * grid_info.GridWidth + RA_info.Routing_Area_X;
			gcell.y = row * grid_info.GridHeight + RA_info.Routing_Area_Y;
			v.push_back(gcell);
		}
		gcells.push_back(v);
	}
	//reverse(gcells.begin(), gcells.end());
}

void readFileCost(string filename) {
	ifstream cost_file(filename);
	string drop;
	
	// read alpha, beta, gamma, delta
	cost_file >> drop >>cost_info.alpha 
			  >> drop >> cost_info.beta 
			  >> drop >> cost_info.gamma 
			  >> drop >> cost_info.delta;
	
	// read via cost
	cost_file >> drop >> cost_info.via_cost;

	// define layer size
	int num_rows = RA_info.num_rows;
	int num_cols = RA_info.num_cols;
	double max_cellcost = 0;

	// read layer 1
	cost_file >> drop;
	for (int row=0; row<num_rows; row++) {
		vector<double> v;
		for (int col=0; col<num_cols; col++) {
			double cost;
			cost_file >> cost;
			v.push_back(cost);
			max_cellcost = max(max_cellcost, cost);
		}
		cost_info.layer1_cost.push_back(v);
	}
	//reverse(cost_info.layer1_cost.begin(), cost_info.layer1_cost.end());

	// read layer 2
	cost_file >> drop;
	for (int row=0; row<num_rows; row++) {
		vector<double> v;
		for (int col=0; col<num_cols; col++) {
			double cost;
			cost_file >> cost;
			v.push_back(cost);
			max_cellcost = max(max_cellcost, cost);
		}
		cost_info.layer2_cost.push_back(v);
	}
	//reverse(cost_info.layer2_cost.begin(), cost_info.layer2_cost.end());

	cost_info.max_cellcost = max_cellcost;
}

// Calculate heuristic function
double Heuristic(Pair src, Pair dest) {
	int x1 = gcells[src.first][src.second].x, y1 = gcells[src.first][src.second].y;
	int x2 = gcells[dest.first][dest.second].x, y2 = gcells[dest.first][dest.second].y;
	
    return abs(x1 - x2) + abs(y1 - y2);
}

// Function to calculate overflow cost
double calculateOverflow(const int &edgeUsage, const int &edgeCapacity, const double &maxCellCost) {
    if (edgeUsage > edgeCapacity) {
        return (edgeUsage - edgeCapacity) * 0.5 * maxCellCost;
    }
    return 0.0;
}

void updateCapacity(vector<Pair> &path_capacity) {
	for (size_t i = 0; i < path_capacity.size() - 1; i++) {
        int row1 = path_capacity[i].first;
        int col1 = path_capacity[i].second;
        int row2 = path_capacity[i + 1].first;
        int col2 = path_capacity[i + 1].second;
        
		if (row1 == row2 && col1 < col2) {
            // Moving right
            gcells[row2][col2].left_usage++;
        } else if (row1 == row2 && col1 > col2) {
            // Moving left
            gcells[row1][col1].left_usage++;
        } else if (col1 == col2 && row1 < row2) {
            // Moving up
            gcells[row2][col2].bottom_usage++;
        } else if (col1 == col2 && row1 > row2) {
            // Moving down
            gcells[row1][col1].bottom_usage++;
        }
	} 
}

void reconstructPath(vector<vector<Node>>& gcellDetails, Pair src , Pair dest, ofstream& output) {
	vector<Pair> path; // {x,y}
	vector<Pair> path_capacity;
	int row = dest.first, col = dest.second;
	while (row != src.first || col != src.second) {
		path.push_back({gcells[row][col].x, gcells[row][col].y});
		path_capacity.push_back({row, col});
		//cout << "(" <<gcells[row][col].x<<", "<<gcells[row][col].y<<") "<<gcellDetails[row][col].layer<<endl;
		int temprow = gcellDetails[row][col].parent_i;
		int tempcol = gcellDetails[row][col].parent_j;
		row = temprow;
		col = tempcol;
	}
	path.push_back({gcells[row][col].x, gcells[row][col].y});
	path_capacity.push_back({row, col});
	
	reverse(path.begin(), path.end());
	reverse(path_capacity.begin(), path_capacity.end());
	
	int currentLayer = 1;
	if (path[0].first != path[1].first) {
		output << "via" <<endl;
		currentLayer = 2;
	}
	int startX = path[0].first, startY = path[0].second;
	size_t i;
	for (i=1; i<path.size(); i++) {
		int layer = (path[i].first == path[i-1].first) ? 1 : 2;
		if (layer != currentLayer) {
			output << "M" << currentLayer << " " << startX << " " << startY << " " << path[i-1].first << " " << path[i-1].second << endl;
			output << "via" <<endl;
			
			startX = path[i-1].first, startY = path[i-1].second;
			currentLayer = layer;
		} 
	}
	output << "M" << currentLayer << " " << startX << " " << startY << " " << path[i-1].first << " " << path[i-1].second << endl;

	if (currentLayer != 1) {
		output << "via" <<endl;
	}
	//for (auto p : path_capacity) {
	//	cout << "("<<p.first<<", "<<p.second<<")"<<endl;
	//}
	//cout << endl;
	updateCapacity(path_capacity);
}

void AstarSearch(Pair src, Pair dest, ofstream& output) {
	int NUM_ROW = RA_info.num_rows, NUM_COL = RA_info.num_cols;
    vector<vector<Node>> gcellDetails(NUM_ROW, vector<Node>(NUM_COL));
    vector<vector<bool>> closedList(NUM_ROW, vector<bool>(NUM_COL, false));

    for (int i = 0; i < NUM_ROW; i++) {
        for (int j = 0; j < NUM_COL; j++) {
            gcellDetails[i][j].f = DBL_MAX;
            gcellDetails[i][j].g = DBL_MAX;
            gcellDetails[i][j].h = DBL_MAX;
            gcellDetails[i][j].parent_i = -1;
            gcellDetails[i][j].parent_j = -1;
			gcellDetails[i][j].layer = 0;
        }
    }

	// Priority queue for open list,  pPair = pair<double, pair<int, int>>
	priority_queue<pPair, vector<pPair>, greater<>> openList;

    // Start node initialization
    int srcRow = src.first, srcCol = src.second;
    gcellDetails[srcRow][srcCol].f = 0.0;
    gcellDetails[srcRow][srcCol].g = 0.0;
    gcellDetails[srcRow][srcCol].h = 0.0;
    gcellDetails[srcRow][srcCol].parent_i = srcRow;
    gcellDetails[srcRow][srcCol].parent_j = srcCol;
	gcellDetails[srcRow][srcCol].layer = 1; // Start on M1

	openList.push({0.0, {srcRow, srcCol}});
	bool foundDest = false;

	while (!openList.empty()) {
		pPair current = openList.top();
		openList.pop();

		int i = current.second.first;
		int j = current.second.second;
		int currentLayer = gcellDetails[i][j].layer;

		if (closedList[i][j]) continue;
		closedList[i][j] = true;

		// If reach destination
		if (i == dest.first && j == dest.second) {
			// do cinstruct path
			foundDest = true;
			//cout << "find!!!"<<endl;
			reconstructPath(gcellDetails, src, dest, output);
            break;
		}

		// Explore neighbors for M1 (Layer 1 - vertical movement)
		vector<pair<int, int>> directionsM1 = {{1, 0}, {-1, 0}}; // up and down
		for (auto &dir : directionsM1) {
			int newRow = i + dir.first, newCol = j + dir.second;
			if (newRow >= 0 && newRow < NUM_ROW && newCol >= 0 && newCol < NUM_COL) {
				double wireLength = grid_info.GridHeight;
				double cellCost = (currentLayer == 2) ? (cost_info.layer1_cost[newRow][newCol] + cost_info.layer2_cost[newRow][newCol] )/ 2 
													  : cost_info.layer1_cost[newRow][newCol];
				
				double overflowCost;
				if (dir.first == 1) // moving up
					overflowCost = calculateOverflow(gcells[newRow][newCol].bottom_usage, gcells[newRow][newCol].bottom_capacity, cost_info.max_cellcost);
				else				// moving down
					overflowCost = calculateOverflow(gcells[newRow+1][newCol].bottom_usage, gcells[newRow+1][newCol].bottom_capacity, cost_info.max_cellcost);

				double viaCost = (currentLayer == 2) ? cost_info.via_cost : 0.0; // Add viaCost if switching from M2 to M1
				double gNew = gcellDetails[i][j].g +  cost_info.alpha*wireLength + cost_info.beta*overflowCost + cost_info.gamma*cellCost + cost_info.delta*viaCost; 
				double hNew = Heuristic({newRow, newCol}, dest);
				double fNew = gNew + hNew;

				if (!closedList[newRow][newCol] && fNew < gcellDetails[newRow][newCol].f) {
					openList.push({fNew, {newRow, newCol}});
					gcellDetails[newRow][newCol].g = gNew;
					gcellDetails[newRow][newCol].h = hNew;
					gcellDetails[newRow][newCol].f = fNew;
					gcellDetails[newRow][newCol].parent_i = i;
					gcellDetails[newRow][newCol].parent_j = j;
					gcellDetails[newRow][newCol].layer = 1; // Update layer to M1
				}
			}
		}

		// Explore neighbors for M2 (Layer 2 - horizontal movement)
		vector<pair<int, int>> directionsM2 = {{0, -1}, {0, 1}}; // Left and Right
		for (auto &dir : directionsM2) {
			int newRow = i + dir.first, newCol = j + dir.second;
			if (newRow >= 0 && newRow < NUM_ROW && newCol >= 0 && newCol < NUM_COL) {
				double wireLength = grid_info.GridWidth;
				double cellCost = (currentLayer == 1) ? (cost_info.layer1_cost[newRow][newCol] + cost_info.layer2_cost[newRow][newCol] )/ 2 
													  : cost_info.layer2_cost[newRow][newCol];
				double overflowCost;
				if (dir.second == 1) // moving right
					overflowCost = calculateOverflow(gcells[newRow][newCol].left_usage, gcells[newRow][newCol].left_capacity, cost_info.max_cellcost);
				else				 // moving left
					overflowCost = calculateOverflow(gcells[newRow][newCol+1].left_usage, gcells[newRow][newCol+1].left_capacity, cost_info.max_cellcost);

				double viaCost = (currentLayer == 1) ? cost_info.via_cost : 0.0; // Add viaCost if switching from M1 to M2
				double gNew = gcellDetails[i][j].g + cost_info.alpha*wireLength + cost_info.gamma*cellCost + cost_info.beta*overflowCost + cost_info.delta*viaCost;
				double hNew = Heuristic({newRow, newCol}, dest);
				double fNew = gNew + hNew;

				if (!closedList[newRow][newCol] && fNew < gcellDetails[newRow][newCol].f) {
					openList.push({fNew, {newRow, newCol}});
					gcellDetails[newRow][newCol].g = gNew;
					gcellDetails[newRow][newCol].h = hNew;
					gcellDetails[newRow][newCol].f = fNew;
					gcellDetails[newRow][newCol].parent_i = i;
					gcellDetails[newRow][newCol].parent_j = j;
					gcellDetails[newRow][newCol].layer = 2; // Update layer to M2
				}
			}
		}
	}

    if (!foundDest) {
        cout << "No path found" << endl;
    }	
	//cout<<"start: " << src.first <<" "<<src.second<<endl;
	//cout <<"end: " << dest.first<<" "<<dest.second<<endl;
	//reverse(gcellDetails.begin(), gcellDetails.end());
	//for (auto i : gcellDetails) {
	//	for (auto j : i) {
	//		cout << j.layer << " ";
	//	}
	//	cout << endl;
	//}
}

int main(int argc, char* argv[]){
	readFileGmp(argv[1]);
	readFileGcl(argv[2]);
	readFileCost(argv[3]);
	ofstream output(argv[4]);

	//for (auto i : gcells) {
	//	for (auto j : i) {
	//		cout << "(" <<j.x<<", "<<j.y<<") ";
	//	}
	//	cout << endl;
	//}

	for (auto n : nets) {
		Pair start = {(n.bump1_y-RA_info.Routing_Area_Y)/grid_info.GridHeight, (n.bump1_x-RA_info.Routing_Area_X)/grid_info.GridWidth};
		Pair end = {(n.bump2_y-RA_info.Routing_Area_Y)/grid_info.GridHeight, (n.bump2_x-RA_info.Routing_Area_X)/grid_info.GridWidth};
		ofstream output(argv[4], ios::app);
		output << "n" << n.idx <<endl;
		//cout<<"start: " << n.bump1_x <<" "<<n.bump1_y<<endl;
		//cout <<"end: " << n.bump2_x<<" "<<n.bump2_y<<endl;
		AstarSearch(start, end, output);
		output << ".end" <<endl;
	}
	//Net n = nets[0];
	//Pair start = {(n.bump1_y-RA_info.Routing_Area_Y)/grid_info.GridHeight, (n.bump1_x-RA_info.Routing_Area_X)/grid_info.GridWidth};
	//Pair end = {(n.bump2_y-RA_info.Routing_Area_Y)/grid_info.GridHeight, (n.bump2_x-RA_info.Routing_Area_X)/grid_info.GridWidth};
	//cout<<"start: " << start.first <<" "<<start.second<<endl;
	//cout <<"end: " << end.first<<" "<<end.second<<endl;
	//AstarSearch(start, end);
}
