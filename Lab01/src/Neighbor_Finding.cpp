#include "Block.h"

std::list<Block*> Neighbor_Finding(Block* startBlock);

std::list<Block*> Find_Right_Neighbor(Block* startBlock);
std::list<Block*> Find_Left_Neighbor(Block* startBlock);
std::list<Block*> Find_Top_Neighbor(Block* startBlock);
std::list<Block*> Find_Bottom_Neighbor(Block* startBlock);

std::list<Block*> Neighbor_Finding(Block* startBlock) {
	std::list<Block*> neighbors{Find_Right_Neighbor(startBlock)};
	std::list<Block*> left_neighbors{Find_Left_Neighbor(startBlock)};
	std::list<Block*> top_neighbors{Find_Top_Neighbor(startBlock)};
	std::list<Block*> bottom_neighbors{Find_Bottom_Neighbor(startBlock)};
	neighbors.splice(neighbors.end(), left_neighbors);
	neighbors.splice(neighbors.end(), top_neighbors);
	neighbors.splice(neighbors.end(), bottom_neighbors);
	return neighbors;
}

std::list<Block*> Find_Right_Neighbor(Block* startBlock) {
	std::list<Block*> neighbors{};
	// find top right(tr) neighbor
	Block* currentBlock = startBlock->tr;
	if (!currentBlock) return neighbors;
	neighbors.push_back(currentBlock);
	currentBlock = currentBlock->lb;
	while (currentBlock) {
		if (currentBlock->y + currentBlock->height <= startBlock->y) {
			break;
		}
		if (!(currentBlock->y >= startBlock->y + startBlock->height)) {
			neighbors.push_back(currentBlock);
		}
		currentBlock = currentBlock->lb;
	}
	return neighbors;
}

std::list<Block*> Find_Left_Neighbor(Block* startBlock) {
	std::list<Block*> neighbors{};
	Block* currentBlock = startBlock->bl;
	if (!currentBlock) return neighbors;
	neighbors.push_back(currentBlock);
	currentBlock = currentBlock->rt;	
	while (currentBlock) {
		if (currentBlock->y >= startBlock->y + startBlock->height) {
			break;
		}
		if (!(currentBlock->y + currentBlock->height <= startBlock->y)) {
			neighbors.push_back(currentBlock);
		}
		currentBlock = currentBlock->rt;
	}
	return neighbors;
}

std::list<Block*> Find_Top_Neighbor(Block* startBlock) {
	std::list<Block*> neighbors{};
	Block* currentBlock = startBlock->rt;
	if (!currentBlock) return neighbors;
	neighbors.push_back(currentBlock);
	currentBlock = currentBlock->bl;
	while (currentBlock) {
		if (currentBlock->x + currentBlock->width <= startBlock->x) {
			break;
		}
		if (!(currentBlock->x >= startBlock->x + startBlock->width)) {
			neighbors.push_back(currentBlock);
		}	
		currentBlock = currentBlock->bl;
	}
	return neighbors;
}

std::list<Block*> Find_Bottom_Neighbor(Block* startBlock) {
	std::list<Block*> neighbors{};
	Block* currentBlock = startBlock->lb;
	if (!currentBlock) return neighbors;
	neighbors.push_back(currentBlock);
	currentBlock = currentBlock->tr;
	while (currentBlock) {
		if (currentBlock->x >= startBlock->x +startBlock->width) {
				break;
		}
		if (!(currentBlock->x + currentBlock->width <= startBlock->x )) {
			neighbors.push_back(currentBlock);
		}
		currentBlock = currentBlock->tr;	
	}
	return neighbors;
}

void Update_Right_Neightbor(Block* startBlock) {
	std::list<Block*> right_neightbor{Find_Right_Neighbor(startBlock)};
	
	if (right_neightbor.size() == 0) return;
	
	for(auto &rightBlock : right_neightbor) {
		if (rightBlock->y < startBlock->y + startBlock->height &&
			rightBlock->y + rightBlock->height >= startBlock->y + startBlock->height) {
			startBlock->tr = rightBlock;
		}
		if (rightBlock->y < startBlock->y + startBlock->height &&
			rightBlock->y >= startBlock->y) {
			rightBlock->bl = startBlock;
		}
	}
}

void Update_Left_Neightbor(Block* startBlock) {
	std::list<Block*> left_neightbor{Find_Left_Neighbor(startBlock)};
	
	if (left_neightbor.size() == 0) return;
	
	for(auto &leftBlock : left_neightbor) {
		if (leftBlock->y <= startBlock->y &&
			leftBlock->y + leftBlock->height > startBlock->y) {
			startBlock->bl = leftBlock;
		}
		if (leftBlock->y + leftBlock->height > startBlock->y &&
			leftBlock->y + leftBlock->height <= startBlock->y + startBlock->height) {
			leftBlock->tr = startBlock;
		}
	}
}

void Update_Top_Neightbor(Block* startBlock) {
	std::list<Block*> top_neightbor{Find_Top_Neighbor(startBlock)};
	
	if (top_neightbor.size() == 0) return;
	
	for(auto &topBlock : top_neightbor) {
		if (topBlock->x < startBlock->x + startBlock->width &&
			topBlock->x + topBlock->width >= startBlock->x + startBlock->width) {
			startBlock->rt = topBlock;
		}
		if (topBlock->x < startBlock->x + startBlock->width &&
			topBlock->x >= startBlock->x ) {
			topBlock->lb = startBlock;
		}
	}
}

void Update_Bottom_Neightbor(Block* startBlock) {
	std::list<Block*> bottom_neightbor{Find_Bottom_Neighbor(startBlock)};
	
	if (bottom_neightbor.size() == 0) return;
	
	for(auto &bottomBlock : bottom_neightbor) {
		if (bottomBlock->x <= startBlock->x &&
			bottomBlock->x + bottomBlock->width >startBlock->x) {
			startBlock->lb = bottomBlock;
		}
		if (bottomBlock->x + bottomBlock->width <= startBlock->x + startBlock->width &&
			bottomBlock->x + bottomBlock->width > startBlock->x) {
			bottomBlock->rt = startBlock;
		}
	}
}