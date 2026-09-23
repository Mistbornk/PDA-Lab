#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <random> 
#include <string>
#include <queue>
#include <unordered_map>
#include <vector>
#define  ll long long
#define INF 2147483647
using namespace std;

class Block {
public:
	string name = "";
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	bool rotate = false;
};

class Cost {
public:
    int width;
    int height;
    ll int area;
    ll int wirelength;
	int cost = 0;
};

struct Node {
public:
    int parent = -1;
    int leftChild = -1;
    int rightChild = -1;
};

class Terminal {
public:
    string name = "";
    int x = 0;
    int y = 0;
};

class Net {
public:
	int degree = 0;
	std::vector<std::string> pins;
};

int outline_width;
int outline_height;
int numBlocks;
int numTerminals;
int numNets;
double alpha;

unordered_map<string, int> mapp;

int root_block = -1;
vector<Block> blocks;
vector<Node> bstartree;
Cost cost;

int best_root = -1;
vector<Block> bestblocks;
vector<Node> bestbstartree;
Cost bestcost;

vector<Terminal> terminals;
vector<vector<string>> nets;

clock_t start;
random_device rd;
mt19937 g(rd());
bool IsInOutline = false;

void OP3_Swap(int node1, int node2);


void readBlockFile(string filename) {
	ifstream block_file(filename);
	string drop;

	// read outline
	block_file >> drop >> outline_width >> outline_height; 
	// read blocks' number
	block_file >> drop >> numBlocks;
	// read terminals' number
	block_file >> drop >> numTerminals;
	
	// read blocks and terminal
	for (int i=0; i<numBlocks; i++) {
		Block block;
		block_file>> block.name >> block.width >> block.height;
		blocks.push_back(block);
        mapp[block.name] = i;
	}

	for (int i=0; i<numTerminals; i++) {
		Terminal terminal;
		block_file >> terminal.name >> drop >> terminal.x >> terminal.y;
		terminals.push_back(terminal);
        mapp[terminal.name] = numBlocks + i;
	}
	block_file.close();
}

void readNetFile(string filename) {
	ifstream nets_file(filename);
	string drop;
    int numPins;

	// read Net numbers
	nets_file >> drop >> numNets;
    nets.resize(numNets);
	for (int i=0; i<numNets; i++) {
		nets_file >> drop >> numPins; 
		for (int j=0; j<numPins; j++) {
			string name;
			nets_file >> name;
			nets[i].push_back(name);
		}
	}
	nets_file.close();
}

void updateContour(int current, vector<int> &contour, bool isLeft) {
    if (current < 0) return;

    int parent = bstartree[current].parent;
    
    // left or right child of parent
    blocks[current].x = isLeft  
        ? blocks[current].x = blocks[parent].x + blocks[parent].width 
        : blocks[current].x = blocks[parent].x;

    int x_start = blocks[current].x;
    int x_end = x_start + blocks[current].width;
    int y_max = 0;
	int contour_size = contour.size();
    if (x_end > contour_size) 
        contour.insert(contour.end(), x_end - contour.size(), 0);

    for (int i = x_start; i < x_end; i++) {
        if (contour[i] > y_max)
            y_max = contour[i];
    }

    blocks[current].y = y_max;

    y_max += blocks[current].height;
    for (int i = x_start; i < x_end; i++)
        contour[i] = y_max;

    // Recursively update the left and right children of the current block
    if (bstartree[current].leftChild != -1)
        updateContour(bstartree[current].leftChild, contour, true);
    if (bstartree[current].rightChild != -1)
        updateContour(bstartree[current].rightChild, contour, false);
}

void Packing2Floorplan() {

    vector<int> contour(max(outline_width, outline_height), 0);
    blocks[root_block].x = 0;
    blocks[root_block].y = 0;
    
    for (int i = 0; i < blocks[root_block].width; i++)
        contour[i] = blocks[root_block].height;

    if (bstartree[root_block].leftChild != -1)
        updateContour(bstartree[root_block].leftChild, contour, true);
    if (bstartree[root_block].rightChild != -1)
        updateContour(bstartree[root_block].rightChild, contour, false);
}

void OP1_Rotate(int current) {
	swap(blocks[current].height, blocks[current].width);
	blocks[current].rotate = (blocks[current].rotate) ? false : true;
}

void OP2_Move(int from, int to) {
	// delete the node
	if (bstartree[from].leftChild == -1 && bstartree[from].rightChild == -1 ) {
		// if no child then directly remove
		int fromParent = bstartree[from].parent;
		if (fromParent != -1) {
			(bstartree[fromParent].leftChild == from) 
				? bstartree[fromParent].leftChild = -1 : bstartree[fromParent].rightChild = -1;
		}
	}else if (bstartree[from].leftChild != -1 && bstartree[from].rightChild != -1) {
		// if has two child
		while (true) {
			bool swapLeft = false; 
			if (bstartree[from].leftChild != -1 && bstartree[from].rightChild != -1)
				swapLeft = (rand() % 2 == 0);
			else if (bstartree[from].leftChild != -1) 
				swapLeft = true;

			if (swapLeft) {
				OP3_Swap(from, bstartree[from].leftChild);
			}else {
				OP3_Swap(from, bstartree[from].rightChild);
			}
			if (bstartree[from].leftChild == -1 && bstartree[from].rightChild == -1) break;
		}
		int fromParent = bstartree[from].parent;
		if (fromParent != -1) {
			(bstartree[fromParent].leftChild == from) 
				? bstartree[fromParent].leftChild = -1 : bstartree[fromParent].rightChild = -1;
		}
	}else {
		// if only one child
		int fromParent = bstartree[from].parent;
		int fromChild = (bstartree[from].leftChild != -1) 
			? bstartree[from].leftChild : bstartree[from].rightChild;
		
		bstartree[fromChild].parent = fromParent;
		if (fromParent != -1) {
			(bstartree[fromParent].leftChild == from) 
				? bstartree[fromParent].leftChild = fromChild : bstartree[fromParent].rightChild = fromChild;
		}
        bstartree[fromChild].parent = fromParent;
		if (root_block == from) root_block = fromChild;
	}

	// insert the node
	int op = rand() % 2;
	int toChild = (op == 0) ? bstartree[to].leftChild : bstartree[to].rightChild;
    switch (op) {
        case 0: {
            bstartree[to].leftChild = from;
            break;
        }
        case 1: {
            bstartree[to].rightChild = from;
            break;
        }
    }
    op = rand() % 2;
    switch (op) {
        case 0: {
            bstartree[from].leftChild = toChild;
            bstartree[from].rightChild = -1;
            break;
        }
        case 1: {
            bstartree[from].rightChild = toChild;
            bstartree[from].leftChild = -1;
            break;
        }
    }
    bstartree[from].parent = to;
    if (toChild != -1)
        bstartree[toChild].parent = from;
}


void swapParent(int node1, int node2) {
    // swap parent
    int node1Parent = bstartree[node1].parent;
	int node2Parent = bstartree[node2].parent;
    if(node1Parent != -1){
        (bstartree[node1Parent].leftChild == node1) 
			? bstartree[node1Parent].leftChild = node2 : bstartree[node1Parent].rightChild = node2;
    }
    if(node2Parent != -1){
        (bstartree[node2Parent].leftChild == node2) 
			? bstartree[node2Parent].leftChild = node1 : bstartree[node2Parent].rightChild = node1;
    }
    swap(bstartree[node1].parent, bstartree[node2].parent);
}

void swapChild(int node1, int node2) {
    // swap children
    swap(bstartree[node1].leftChild, bstartree[node2].leftChild);
    swap(bstartree[node1].rightChild, bstartree[node2].rightChild);

    if(bstartree[node1].leftChild != -1) bstartree[bstartree[node1].leftChild].parent = node1;
    if(bstartree[node1].rightChild != -1) bstartree[bstartree[node1].rightChild].parent = node1;

    if(bstartree[node2].leftChild != -1) bstartree[bstartree[node2].leftChild].parent = node2;
    if(bstartree[node2].rightChild != -1) bstartree[bstartree[node2].rightChild].parent = node2;
}

void OP3_Swap(int node1, int node2) {
	// swap parent
	swapParent(node1, node2);

	// swap child
	swapChild(node1, node2);

    // relationship of node1 and node2 are parent and child
    if (bstartree[node1].parent == node1) bstartree[node1].parent = node2;
    else if (bstartree[node1].leftChild == node1) bstartree[node1].leftChild = node2;
    else if (bstartree[node1].rightChild == node1) bstartree[node1].rightChild = node2;

    if (bstartree[node2].parent == node2) bstartree[node2].parent = node1;
    else if (bstartree[node2].leftChild  == node2) bstartree[node2].leftChild  = node1;
    else if (bstartree[node2].rightChild == node2) bstartree[node2].rightChild = node1;

    // change root 
    if(root_block == node1) root_block = node2;
    else if(root_block == node2) root_block = node1;
}

vector<int> initBlockID(){
    vector<int> BlocksID(numBlocks);
    for(int i = 0; i < numBlocks; i++){
        BlocksID[i] = i;
    }
    return BlocksID;
}

void BuildBstarTree() {
    bstartree = vector<Node>(numBlocks);
    int current = root_block = 0;
    bstartree[root_block].parent = -1;

    vector<int> BlocksID = initBlockID();

    shuffle(BlocksID.begin() + 1, BlocksID.end(), g);

    for (int i = 1; i < numBlocks; i++) {
        int next = BlocksID[i];  
        bstartree[current].rightChild = next;
        bstartree[next].parent = current;
        current = next;
    }   
}

ll CalWireLenth() {
	int HPWL = 0;
    for (const vector<string>& net : nets){
        int x_min = INF, x_max = -1;
        int y_min = INF, y_max = -1;
        for (const string& pin : net) {
            int idx = mapp[pin];
            if (idx >= numBlocks) {
                Terminal& t = terminals[idx - numBlocks];
                x_min = min(x_min, t.x), y_min = min(y_min, t.y);
                x_max = max(x_max, t.x), y_max = max(y_max, t.y);
            } else if (idx < numBlocks){
                int x_center = blocks[idx].x + blocks[idx].width / 2;
                int y_center = blocks[idx].y + blocks[idx].height / 2;
                x_min = min(x_min, x_center), y_min = min(y_min, y_center);
                x_max = max(x_max, x_center), y_max = max(y_max, y_center);
            }
        }
        HPWL += x_max - x_min + y_max - y_min;
    }
    return HPWL;
}

int FindFloorplanWidth() {
	int max_width = 0;
	for (int i = 0; i < numBlocks; i++) {
		max_width = max(max_width, blocks[i].x + blocks[i].width);
	}
	return max_width;
}

int FindFloorplanHeight() {
	int max_height = 0;
	for (int i = 0; i < numBlocks; i++) {
		max_height = max(max_height, blocks[i].y + blocks[i].height);
	}
	return max_height;
}

int CalPenalty(int width, int height) {
	int penalty = 0;
	if (width > outline_width) penalty += (width - outline_width);
	if (height > outline_height) penalty += (height - outline_height);
	
	return penalty;
}

bool OutlineCheck(int width, int height) {
    if (width <= outline_width && height <= outline_height)
        return true;
    else return false;
}

Cost CalCost(bool findOutline) {
    Packing2Floorplan();

	Cost c;
	c.width = FindFloorplanWidth();
	c.height = FindFloorplanHeight();
	c.area = c.width * c.height;
	c.wirelength = CalWireLenth();
    IsInOutline = OutlineCheck(c.width, c.height);
	int penalty = CalPenalty(c.width, c.height);

	if (findOutline) {
		c.cost =  penalty;
	}else {
		c.cost = alpha * (double)c.area + (1.0 - alpha) * (double)c.wirelength;
	}
	return c;
}

bool isAccept(const double &T, const double &diff) {
	double RandNum = (double)rand() / (RAND_MAX);
    return exp(-diff/T) > RandNum;
}

Cost Perturb(bool findOutline) {
	int op = rand() % 3;		
	switch (op) {
		case 0: {
			int randID = rand() % numBlocks;
			OP1_Rotate(randID);
			break;
		}
		case 1: {
			int randID1 = rand() % numBlocks;
			int randID2 = rand() % numBlocks;
			while (randID1 == randID2) randID2 = rand() % numBlocks;
			OP2_Move(randID1, randID2);
			break;
		}
		case 2: {
			int randID1 = rand() % numBlocks;
			int randID2 = rand() % numBlocks;
			while (randID1 == randID2) randID2 = rand() % numBlocks;
			OP3_Swap(randID1, randID2);
			break;
		}
	}
	Cost current_cost = CalCost(findOutline);
	return current_cost;
}

bool checkTime(int &seconds_to_fit_outline, int &max_seconds_to_fit_outline, bool in_fixed_outline){
    if (seconds_to_fit_outline >= max_seconds_to_fit_outline && in_fixed_outline == false)
        return true;
    else return false;
}

void SimulatedAnneling()
{
    BuildBstarTree();
    bool random_expanded = true;
    bool in_fixed_outline = false;
    bestcost = CalCost(random_expanded );
    
    bestblocks = blocks;

    double P = 0.95;
    double r = 0.9;
    int N = 20 * numBlocks;
    double T0 = -bestcost.cost * numBlocks / log(P);
    
    double T = T0;
    int total_move = 0;
    int uphill = 0;
    Cost old_cost = bestcost;

    clock_t init_time = clock();
    clock_t seconds_to_fit_outline_time = init_time;
    clock_t round_count_start = clock();
    int max_seconds_to_fit_outline = numBlocks/2;
    int TIME_LIMIT = 280;
    int seconds_to_fit_outline = 0, runtime = 0;
    int i = 1;

    total_move = 0;
    uphill = 0;

    while (runtime < TIME_LIMIT && (clock()-start)/CLOCKS_PER_SEC < 280) {
        total_move = 0;
        uphill = 0;

        while (uphill <= N && total_move <= 2 * N) {
            vector<Block> tempblocks(blocks);
            vector<Node> tempbstartree(bstartree);
            int prev_root_block = root_block;

            Cost cur_cost;
            
            int count_time = 0;
            
            if (random_expanded) {
                count_time = (clock() - round_count_start) / CLOCKS_PER_SEC;
                if (count_time > 1.5) {
                    i = (i + 1) % 5;
                    round_count_start = clock();
                } 
                int tmp = i;
                while(tmp--) cur_cost = Perturb(random_expanded ); 
            } else {
                cur_cost = Perturb(random_expanded );
            }

            total_move++;
            
            double delta_cost = cur_cost.cost - old_cost.cost;
            bool in_outline_after_perturb = false;
            bool acceptPoor = isAccept(T, delta_cost);
            if (delta_cost <= 0 || old_cost.cost == 0 ||
                ((acceptPoor) && random_expanded)) {

                if (cur_cost.width <= outline_width && cur_cost.height <= outline_height) {
                    
                    in_outline_after_perturb = true;

                    if (in_fixed_outline && cur_cost.cost <= bestcost.cost && cur_cost.cost != 0) {
                        bestcost = cur_cost;
                        bestblocks = blocks;
                        bestbstartree = bstartree;    
                    } else {
                        in_fixed_outline = true;
                        random_expanded  = false;

						bestcost = cur_cost;
						bestblocks = blocks;
						bestbstartree = bstartree;

                        Cost tmp_cost = CalCost(random_expanded);
                        bestcost.cost = tmp_cost.cost;
                        cur_cost.cost = tmp_cost.cost;
                    }
                }

                if (cur_cost.cost <= bestcost.cost && random_expanded ) {
                    
					bestcost = cur_cost;
					bestblocks = blocks;
					bestbstartree = bstartree;
                }

                if (random_expanded  || 
                    (!random_expanded  && in_outline_after_perturb) ||
                    cur_cost.cost != 0) {

                    old_cost = cur_cost;
                }
                if (delta_cost > 0) uphill++;
            } else {
                root_block = prev_root_block;
                blocks = tempblocks;
                bstartree = tempbstartree;
            }
        }

        T *= r;

        seconds_to_fit_outline = (clock() - seconds_to_fit_outline_time) / CLOCKS_PER_SEC;
        runtime = (clock() - init_time) / CLOCKS_PER_SEC;

        if (checkTime(seconds_to_fit_outline, max_seconds_to_fit_outline, in_fixed_outline)) {
            seconds_to_fit_outline = 0;
            seconds_to_fit_outline_time = clock();
            T = T0;
        }
        
    }
}

int main(int argc, char* argv[])
{	
	start = clock();
	srand(time(NULL));
	
	// read data
	alpha = stod(argv[1]);
	readBlockFile(argv[2]);
	readNetFile(argv[3]);
	ofstream output_file(argv[4]);

	SimulatedAnneling();

	clock_t end = clock();
	double duration = double(end - start) / CLOCKS_PER_SEC;

	output_file <<  bestcost.cost <<endl;
	output_file << bestcost.wirelength <<endl;
	output_file << bestcost.area <<endl;
	output_file << bestcost.width << " " << bestcost.height <<endl;
	output_file << duration << endl;
	for (auto i : bestblocks) {
		output_file << i.name << " " << i.x <<" " << i.y<< " " <<i.x + i.width <<" "<<i.y+i.height<<endl;
	}
}
