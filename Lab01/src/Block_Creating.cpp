#include "Block.h"
#include <algorithm>
using std::cout;

void Block_Creating(const int &idx, const int &x, const int &y, const int &width, const int &height, std::list<Block*> &blocks);
void UpdateCornerStitches(Block* newBlock, std::list<Block*> &blocks);
void UpdateNeighborBlocks(Block* newBlock, std::list<Block*> &blocks);
Block* FindBlockContainingTopEdge(Block* newBlock, std::list<Block*> &blocks);
void SplitTop(Block* newBlock, Block* spaceBlock, std::list<Block*> &blocks);
Block* FindBlockContainingBottomEdge(Block* newBlock, std::list<Block*> &blocks);
void SplitBottom(Block* newBlock, Block* spaceBlock, std::list<Block*> &blocks);

void Block_Creating(const int &idx, const int &x, const int &y, const int &width, const int &height, std::list<Block*> &blocks) {
	// establish a new block
	Block* newBlock = new Block(idx, x, y, width, height, true);
	// 1., 2. find the space tile containg the top edge and split into above & overlapping
	Block* spaceBlock = FindBlockContainingTopEdge(newBlock, blocks);			
	if (spaceBlock) SplitTop(newBlock, spaceBlock, blocks);
	// 3. do same things at bottom edge
	spaceBlock = FindBlockContainingBottomEdge(newBlock, blocks);
	if (spaceBlock) SplitBottom(newBlock, spaceBlock, blocks);

	// go along the left side edge of new tile
	int start_x = newBlock->x, start_y = newBlock->y + newBlock->height - 1;
	Block* currentBlock = Point_Finding(blocks.front(), start_x, start_y);
	std::vector<Block*> remain_internalBlock;
	while (currentBlock->y >= newBlock->y) {
		// initialize
		int width, height = currentBlock->height;
		Block* leftBlock = nullptr;
		Block* rightBlock = nullptr;
		Block* internalBlock = nullptr;

		// left tile
		if (currentBlock->x < newBlock->x) {
			width = newBlock->x - currentBlock->x;
			leftBlock = new Block(0, currentBlock->x, currentBlock->y, width, height, false);
			blocks.push_back(leftBlock);
			UpdateCornerStitches(leftBlock, blocks);
			UpdateNeighborBlocks(leftBlock, blocks);
		}


		// right tile
		if (currentBlock->x + currentBlock->width > newBlock->x + newBlock->width) {
			width = (currentBlock->x + currentBlock->width) - (newBlock->x + newBlock->width);
			rightBlock = new Block(0, newBlock->x + newBlock->width, currentBlock->y, width, height, false);
			blocks.push_back(rightBlock);
			UpdateCornerStitches(rightBlock, blocks);
			UpdateNeighborBlocks(rightBlock, blocks);
		}

		// internal tile
		internalBlock = new Block(-1, newBlock->x, currentBlock->y, newBlock->width, height, true);
		blocks.push_back(internalBlock);
		remain_internalBlock.push_back(internalBlock);
		UpdateCornerStitches(internalBlock, blocks);
		UpdateNeighborBlocks(internalBlock, blocks);		
		// merge left tile		
		if (leftBlock && leftBlock->rt && !leftBlock->rt->isSolid) {
			if (leftBlock->rt->x == leftBlock->x &&
				leftBlock->rt->width == leftBlock->width) {
				
				Block* mergeBlock = new Block(0, leftBlock->x, leftBlock->y, leftBlock->width, 
												 leftBlock->height + leftBlock->rt->height, false);
				blocks.push_back(mergeBlock);
				UpdateCornerStitches(mergeBlock, blocks);
				UpdateNeighborBlocks(mergeBlock, blocks);
				Block* tempL = leftBlock;
				blocks.remove(tempL->rt);
				blocks.remove(tempL);
				delete tempL->rt;
				delete tempL;
				if (mergeBlock && mergeBlock->lb && !mergeBlock->lb->isSolid) {
					if (mergeBlock->lb->x == mergeBlock->x &&
						mergeBlock->lb->width == mergeBlock->width) {
						
						Block* mergeBlock_temp = new Block(0, mergeBlock->lb->x, mergeBlock->lb->y, mergeBlock->width, 
														mergeBlock->height + mergeBlock->lb->height, false);
						blocks.push_back(mergeBlock_temp);
						UpdateCornerStitches(mergeBlock_temp, blocks);
						UpdateNeighborBlocks(mergeBlock_temp, blocks);
						Block* tempL = mergeBlock;
						blocks.remove(tempL->lb);
						blocks.remove(tempL);
						delete tempL->lb;
						delete tempL;
					}
				}
			}
		}
		if (leftBlock && leftBlock->lb && !leftBlock->lb->isSolid) {
			if (leftBlock->lb->x == leftBlock->x &&
				leftBlock->lb->width == leftBlock->width) {
				
				Block* mergeBlock = new Block(0, leftBlock->lb->x, leftBlock->lb->y, leftBlock->width, 
												 leftBlock->height + leftBlock->lb->height, false);
				blocks.push_back(mergeBlock);
				UpdateCornerStitches(mergeBlock, blocks);
				UpdateNeighborBlocks(mergeBlock, blocks);
				Block* tempL = leftBlock;
				blocks.remove(tempL->lb);
				blocks.remove(tempL);
				delete tempL->lb;
				delete tempL;
			}
		}
		// merge right tile	
		if (rightBlock && rightBlock->rt && !rightBlock->rt->isSolid) {
			if (rightBlock->rt->x == rightBlock->x &&
				rightBlock->rt->width == rightBlock->width) {
				Block* mergeBlock = new Block(0, rightBlock->x, rightBlock->y, rightBlock->width, 
												 rightBlock->height + rightBlock->rt->height, false);
				blocks.push_back(mergeBlock);
				UpdateCornerStitches(mergeBlock, blocks);
				UpdateNeighborBlocks(mergeBlock, blocks);
				Block* tempR = rightBlock;
				blocks.remove(tempR->rt);
				blocks.remove(tempR);
				delete tempR->rt;
				delete tempR;
				if (mergeBlock && mergeBlock->lb && !mergeBlock->lb->isSolid) {
					if (mergeBlock->lb->x == mergeBlock->x &&
						mergeBlock->lb->width == mergeBlock->width) {
						
						Block* mergeBlock_temp = new Block(0, mergeBlock->lb->x, mergeBlock->lb->y, mergeBlock->width, 
														mergeBlock->height + mergeBlock->lb->height, false);
						blocks.push_back(mergeBlock_temp);
						UpdateCornerStitches(mergeBlock_temp, blocks);
						UpdateNeighborBlocks(mergeBlock_temp, blocks);
						Block* tempR = mergeBlock;
						blocks.remove(tempR->lb);
						blocks.remove(tempR);
						delete tempR->lb;
						delete tempR;
					}
				}
			}
		}
		if (rightBlock && rightBlock->lb && !rightBlock->lb->isSolid) {
			if (rightBlock->lb->x == rightBlock->x &&
				rightBlock->lb->width == rightBlock->width) {
				
				Block* mergeBlock = new Block(0, rightBlock->lb->x, rightBlock->lb->y, rightBlock->width, 
												 rightBlock->height + rightBlock->lb->height, false);
				blocks.push_back(mergeBlock);
				UpdateCornerStitches(mergeBlock, blocks);
				UpdateNeighborBlocks(mergeBlock, blocks);
				Block* tempR = rightBlock;
				blocks.remove(tempR->lb);
				blocks.remove(tempR);
				delete tempR->lb;
				delete tempR;
			}
		}

		// remove overlapping tile
		Block* temp = currentBlock;
		blocks.remove(temp);
		start_y = start_y - temp->height;
		currentBlock = Point_Finding(blocks.front(), start_x, start_y);
		delete temp;

		if(!currentBlock) break;
	}
	// merge the internal tiles into new tile
	for (auto &block : remain_internalBlock) {
		blocks.remove(block);
		delete(block);
	}
	blocks.push_back(newBlock);
	UpdateCornerStitches(newBlock, blocks);
	UpdateNeighborBlocks(newBlock, blocks);
}

void UpdateCornerStitches(Block* newBlock, std::list<Block*> &blocks) {
	for (auto &block : blocks) {
		// check rt (top)
		if (block->y == newBlock->y + newBlock->height) {
			if (block->x < newBlock->x + newBlock->width &&
			    newBlock->x + newBlock->width <= block->x + block->width) {
				newBlock->rt = block;
			}
		}
		// check tr (right)
		if (block->x == newBlock->x + newBlock->width) {
			if (block->y < newBlock->y + newBlock->height &&
				newBlock->y + newBlock->height <= block->y + block->height) {
				newBlock->tr = block;
			}
		}
		// check lb (bottom)
		if (block->y + block->height == newBlock->y) {
			if (block->x <= newBlock->x &&
				newBlock->x < block->x + block->width) {
				newBlock->lb = block;
			}
		}
		// check bl (left)
		if (block->x + block->width == newBlock->x) {
			if (block->y <= newBlock->y &&
				newBlock->y < block->y + block->height) {
				newBlock->bl = block;
			}
		}
	}
}

void UpdateNeighborBlocks(Block* newBlock, std::list<Block*> &blocks) {
	std::list<Block*> neighbor = Neighbor_Finding(newBlock);
	for (auto &block : neighbor) {
		UpdateCornerStitches(block, blocks);
	}
}




Block* FindBlockContainingTopEdge(Block* newBlock, std::list<Block*> &blocks) {
	for (auto &block : blocks) {
		// skip solid tiles
		if(block->isSolid) continue;

		// find the space tile containg the top edge 
		if (block->y < newBlock->y + newBlock->height && 
			newBlock->y + newBlock->height < block->y + block->height) {
			if (block->x <= newBlock->x &&
				newBlock->x + newBlock->width <= block->x + block->width) {
					return block;
			}
		}
	}
	return nullptr;
}

void SplitTop(Block* newBlock, Block* spaceBlock, std::list<Block*> &blocks) {
	int edge_y_pos = newBlock->y + newBlock->height;
	// split the above tile
	int aboveheight = spaceBlock->y + spaceBlock->height - edge_y_pos;
	Block* aboveBlock = new Block(0, spaceBlock->x, edge_y_pos,
									 spaceBlock->width, aboveheight, false);
	// split the overlapping tile
	int overlappingheight = spaceBlock->height - aboveheight;
	Block* overlappingBlock = new Block(0, spaceBlock->x, spaceBlock->y,
										   spaceBlock->width, overlappingheight, false);
	// delete the oringinal tile
	blocks.remove(spaceBlock);
	delete spaceBlock;
				
	// put slpit tiles into list
	blocks.push_back(aboveBlock);
	blocks.push_back(overlappingBlock);	

	// update tiles adjoining the newtiles
	UpdateCornerStitches(aboveBlock, blocks);
	UpdateCornerStitches(overlappingBlock, blocks);
	UpdateNeighborBlocks(aboveBlock, blocks);
	UpdateNeighborBlocks(overlappingBlock, blocks);
	if (aboveBlock->rt && !aboveBlock->rt->isSolid) {
		if (aboveBlock->rt->x == aboveBlock->x &&
			aboveBlock->rt->width == aboveBlock->width) {
				
			Block* mergeBlock = new Block(0, aboveBlock->x, aboveBlock->y, aboveBlock->width, 
												 aboveBlock->height + aboveBlock->rt->height, false);
			Block* tempL = aboveBlock;
			blocks.remove(tempL->rt);
			blocks.remove(tempL);
			blocks.push_back(mergeBlock);
			UpdateCornerStitches(mergeBlock, blocks);
			UpdateNeighborBlocks(mergeBlock, blocks);
			delete tempL->rt;
			delete tempL;
		}
	}
}

Block* FindBlockContainingBottomEdge(Block* newBlock, std::list<Block*> &blocks) {
	for (auto &block : blocks) {
		// skip solid tiles
		if (block->isSolid) continue;

		// find the space tile containg the bottom edge
		if (block->y < newBlock->y &&
			newBlock->y < block->y + block->height) {
			if (block->x <= newBlock->x &&
				newBlock->x + newBlock->width <= block->x + block->width) {
				return block;
			}
		}
	}
	return nullptr;
}

void SplitBottom(Block* newBlock, Block* spaceBlock, std::list<Block*> &blocks) {
	int edge_y_pos = newBlock->y;
	// split the above tile
	int belowheight = edge_y_pos - spaceBlock->y;
	Block* belowBlock = new Block(0, spaceBlock->x, spaceBlock->y, 
													spaceBlock->width, belowheight, false);

	
	int overlappingheight = spaceBlock->height - belowheight;
	Block* overlappingBlock = new Block(0, spaceBlock->x, edge_y_pos, 
										   spaceBlock->width, overlappingheight, false);

	// delete the oringinal tile
	blocks.remove(spaceBlock);
	delete spaceBlock;

	// put slpit tiles into list
	blocks.push_back(belowBlock);
	blocks.push_back(overlappingBlock);	

	
	// update tiles adjoining the newtiles
	UpdateCornerStitches(belowBlock, blocks);
	UpdateCornerStitches(overlappingBlock, blocks);
	UpdateNeighborBlocks(belowBlock, blocks);
	UpdateNeighborBlocks(overlappingBlock, blocks);

	
	if (belowBlock && belowBlock->lb && !belowBlock->lb->isSolid) {
		if (belowBlock->lb->x == belowBlock->x &&
			belowBlock->lb->width == belowBlock->width) {
			Block* mergeBlock = new Block(0, belowBlock->lb->x, belowBlock->lb->y, belowBlock->lb->width, 
												 belowBlock->height + belowBlock->lb->height, false);
			blocks.push_back(mergeBlock);
			UpdateCornerStitches(mergeBlock, blocks);
			UpdateNeighborBlocks(mergeBlock, blocks);
			Block* temp = belowBlock;
			blocks.remove(temp->lb);
			blocks.remove(temp);
			delete temp->lb;
			delete temp;
		}
	}
}