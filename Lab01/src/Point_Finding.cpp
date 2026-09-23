#include "Block.h"

Block* Point_Finding(Block* start, const int &target_x, const int &target_y);
bool isTargetBlock(Block* current, const int &target_x, const int &target_y);

Block* Point_Finding(Block* start, const int &target_x, const int &target_y) {
	if(!start) return nullptr;
	if(target_x >= outline_x || target_y >= outline_y || target_x < 0 || target_y < 0) return nullptr;
	Block* current = start;
	while (!isTargetBlock(current, target_x, target_y)) { //if not the target tile
		// move vertical (up or down)
		while (current) { 
			if (current->y <= target_y && target_y < current->y + current->height)
				break;
			current = (target_y < current->y) ? current->lb : current->rt;
			//std::cout << "vertical" << std::endl;
		}
		// move horizontal (left or right)
		while (current) {
			if (current->x <= target_x && target_x < current->x + current->width)
				break;
			current = (target_x < current->x) ? current->bl : current->tr;
			//std::cout << "horizental" << std::endl;
		}
	}
	return current;
}

// return wheather the tile contain target point or not
bool isTargetBlock(Block* current, const int &target_x, const int &target_y) {
	if (current->y <= target_y && target_y < current->y + current->height)
		if (current->x <= target_x && target_x < current->x + current->width)
			return true;
	return false;
}