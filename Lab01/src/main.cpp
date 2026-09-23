#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <string>
#include "Block.h"
int outline_x = 0;
int outline_y = 0;
using namespace std;

int compare(const Block* a, const Block *b);

int compare(const Block* a, const Block *b) {
	return a->index < b->index;
}

int main(int argc, char* argv[]){
	// tile list
	list<Block*> tiles;
	vector<pair<int, int>> points;
	vector<Block*> solid_tiles;

	// read input case{num}.txt
	ifstream input_file(argv[1]);
	ofstream output_file(argv[2]);

	string line;
	getline(input_file, line);
	istringstream iss(line);

	// initialize a outline space
	iss >> outline_x >> outline_y;
	tiles.push_back(new Block(0, 0, 0, outline_x, outline_y, false));

	// read input by line and word by word
	while(getline(input_file, line)){
		istringstream iss(line);
		string word;
		iss >> word;
		if(word == "P"){ // point_finding
			int x_pos, y_pos;
			iss >> x_pos >> y_pos;
			Block* findBlock = Point_Finding(tiles.front(), x_pos, y_pos);
			points.push_back({findBlock->x, findBlock->y});
		}else{ 			 // block_creating
			int idx = stoi(word), x_pos, y_pos, width, height;
			iss >> x_pos >> y_pos >> width >> height;
			Block_Creating(idx, x_pos, y_pos, width, height, tiles);
		}
	}

	// select solid tiles for output
	for (auto &block : tiles) {
		if (block->index > 0) {
			solid_tiles.push_back(block);
		}
	}

	// sort output by index
	std::sort(solid_tiles.begin(), solid_tiles.end(), compare);

	// print tiles number with in outline
	output_file << tiles.size() << endl;
	// print solid tiles
	int adj_solid_tiles = 0, adj_space_tiles = 0;
	list<Block*> adjacent;
	for(auto &block : solid_tiles) {
		adjacent = Neighbor_Finding(block);
		for (auto &adj : adjacent) {
			if (adj->isSolid) 
				adj_solid_tiles ++;
			else adj_space_tiles++;
		}
		output_file  << block->index << " " <<adj_solid_tiles << " " << adj_space_tiles << endl;
		adj_solid_tiles = 0;
		adj_space_tiles = 0;
	}
	
	// print point we find
	for (auto &p : points) {
		output_file  << p.first <<" "<< p.second << endl;
	}

	/*
	cout << tiles.size()<<endl;
	cout << outline_x << " "<<outline_y<<endl;
	for(auto &block : tiles) {
			cout<< block->index << " " << block->x<<" " <<block->y<<" "<<block->width<<" "<<block->height<<"\n";
	}*/
	return 0;
}