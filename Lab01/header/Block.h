#ifndef BLOC_CREATING_H
#define BLOC_CREATING_H
#include <list>
#include <vector>
#include <iostream>

extern int outline_x;
extern int outline_y;

class Block{
public:
	Block(int idx, int x_pos, int y_pos, int w, int h, bool solid)
		: index(idx), x(x_pos), y(y_pos), width(w), height(h), isSolid(solid) { }
	~Block() {}
	int index = 0;
	int x, y;
	int width, height;
	bool isSolid = false;
	Block *rt = nullptr, *tr = nullptr, *bl = nullptr, *lb = nullptr;
};

int compare(const Block* a, const Block *b);

void Block_Creating (const int &idx, const int &x, const int &y,
					const int &width, const int &height, std::list<Block*> &blocks);
void UpdateCornerStitches(Block* newBlock, std::list<Block*> &blocks);

std::list<Block*> Neighbor_Finding (Block* startBlock); 

Block* Point_Finding (Block* start, const int &target_x, const int &target_y);

#endif