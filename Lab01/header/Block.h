#ifndef BLOC_CREATING_H
#define BLOC_CREATING_H
#include <iostream>
#include <list>
#include <memory>
#include <vector>

#include "TileList.h"

// Transfer retiring ownership until the caller finishes repairing stitches.
inline std::unique_ptr<Block> ExtractTile(TileList &tiles, Block *tile) {
    return tiles.extract(tile);
}

void Block_Creating(const int &idx, const int &x, const int &y, const int &width, const int &height,
                    TileList &blocks);
void UpdateCornerStitches(Block *newBlock, TileList &blocks);

std::list<Block *> Neighbor_Finding(Block *startBlock);

Block *Point_Finding(Block *start, const int &target_x, const int &target_y);

#endif