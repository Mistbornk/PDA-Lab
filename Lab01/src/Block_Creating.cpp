#include "Block.h"
#include "pda/io.hpp"
#include <algorithm>
using std::cout;

void Block_Creating(const int &idx, const int &x, const int &y, const int &width, const int &height,
                    TileList &blocks);
void UpdateCornerStitches(Block *newBlock, TileList &blocks);
void UpdateNeighborBlocks(Block *newBlock, TileList &blocks);
Block *FindBlockContainingTopEdge(Block *newBlock, TileList &blocks);
void SplitTop(Block *newBlock, Block *spaceBlock, TileList &blocks);
Block *FindBlockContainingBottomEdge(Block *newBlock, TileList &blocks);
void SplitBottom(Block *newBlock, Block *spaceBlock, TileList &blocks);

void Block_Creating(const int &idx, const int &x, const int &y, const int &width, const int &height,
                    TileList &blocks) {
    // Check legality before mutating any stitch or index.
    pda::require(!blocks.overlaps_solid(x, y, width, height),
                 "Inserted block overlaps an existing solid tile");
    // establish a new block
    auto newBlock_owner = std::make_unique<Block>(idx, x, y, width, height, true);
    Block *newBlock = newBlock_owner.get();
    // 1., 2. find the space tile containg the top edge and split into above & overlapping
    Block *spaceBlock = FindBlockContainingTopEdge(newBlock, blocks);
    if (spaceBlock)
        SplitTop(newBlock, spaceBlock, blocks);
    // 3. do same things at bottom edge
    spaceBlock = FindBlockContainingBottomEdge(newBlock, blocks);
    if (spaceBlock)
        SplitBottom(newBlock, spaceBlock, blocks);

    // go along the left side edge of new tile
    int start_x = newBlock->x, start_y = newBlock->y + newBlock->height - 1;
    Block *currentBlock = Point_Finding(blocks.front().get(), start_x, start_y);
    std::vector<Block *> remain_internalBlock;
    pda::require(currentBlock != nullptr, "Invalid insertion location");
    while (currentBlock->y >= newBlock->y) {
        pda::require(!currentBlock->isSolid && currentBlock->x <= newBlock->x &&
                         currentBlock->x + currentBlock->width >= newBlock->x + newBlock->width,
                     "Inserted block overlaps an existing solid tile");
        // initialize
        int width, height = currentBlock->height;
        Block *leftBlock = nullptr;
        Block *rightBlock = nullptr;
        Block *internalBlock = nullptr;

        // left tile
        if (currentBlock->x < newBlock->x) {
            width = newBlock->x - currentBlock->x;
            auto leftBlock_owner =
                std::make_unique<Block>(0, currentBlock->x, currentBlock->y, width, height, false);
            leftBlock = leftBlock_owner.get();
            blocks.push_back(std::move(leftBlock_owner));
            UpdateCornerStitches(leftBlock, blocks);
            UpdateNeighborBlocks(leftBlock, blocks);
        }

        // right tile
        if (currentBlock->x + currentBlock->width > newBlock->x + newBlock->width) {
            width = (currentBlock->x + currentBlock->width) - (newBlock->x + newBlock->width);
            auto rightBlock_owner = std::make_unique<Block>(0, newBlock->x + newBlock->width,
                                                            currentBlock->y, width, height, false);
            rightBlock = rightBlock_owner.get();
            blocks.push_back(std::move(rightBlock_owner));
            UpdateCornerStitches(rightBlock, blocks);
            UpdateNeighborBlocks(rightBlock, blocks);
        }

        // internal tile
        auto internalBlock_owner = std::make_unique<Block>(-1, newBlock->x, currentBlock->y,
                                                           newBlock->width, height, true);
        internalBlock = internalBlock_owner.get();
        blocks.push_back(std::move(internalBlock_owner));
        remain_internalBlock.push_back(internalBlock);
        UpdateCornerStitches(internalBlock, blocks);
        UpdateNeighborBlocks(internalBlock, blocks);
        // merge left tile
        if (leftBlock && leftBlock->rt && !leftBlock->rt->isSolid) {
            if (leftBlock->rt->x == leftBlock->x && leftBlock->rt->width == leftBlock->width) {

                auto mergeBlock_owner =
                    std::make_unique<Block>(0, leftBlock->x, leftBlock->y, leftBlock->width,
                                            leftBlock->height + leftBlock->rt->height, false);
                Block *mergeBlock = mergeBlock_owner.get();
                blocks.push_back(std::move(mergeBlock_owner));
                UpdateCornerStitches(mergeBlock, blocks);
                UpdateNeighborBlocks(mergeBlock, blocks);
                Block *tempL = leftBlock;
                auto retired_upper_space = ExtractTile(blocks, tempL->rt);
                auto retired_left_space = ExtractTile(blocks, tempL);
                leftBlock = nullptr;
                if (mergeBlock && mergeBlock->lb && !mergeBlock->lb->isSolid) {
                    if (mergeBlock->lb->x == mergeBlock->x &&
                        mergeBlock->lb->width == mergeBlock->width) {

                        auto mergeBlock_temp_owner = std::make_unique<Block>(
                            0, mergeBlock->lb->x, mergeBlock->lb->y, mergeBlock->width,
                            mergeBlock->height + mergeBlock->lb->height, false);
                        Block *mergeBlock_temp = mergeBlock_temp_owner.get();
                        blocks.push_back(std::move(mergeBlock_temp_owner));
                        UpdateCornerStitches(mergeBlock_temp, blocks);
                        UpdateNeighborBlocks(mergeBlock_temp, blocks);
                        Block *tempL = mergeBlock;
                        auto retired_lower_space = ExtractTile(blocks, tempL->lb);
                        auto retired_merged_left = ExtractTile(blocks, tempL);
                    }
                }
            }
        }
        if (leftBlock && leftBlock->lb && !leftBlock->lb->isSolid) {
            if (leftBlock->lb->x == leftBlock->x && leftBlock->lb->width == leftBlock->width) {

                auto mergeBlock_owner =
                    std::make_unique<Block>(0, leftBlock->lb->x, leftBlock->lb->y, leftBlock->width,
                                            leftBlock->height + leftBlock->lb->height, false);
                Block *mergeBlock = mergeBlock_owner.get();
                blocks.push_back(std::move(mergeBlock_owner));
                UpdateCornerStitches(mergeBlock, blocks);
                UpdateNeighborBlocks(mergeBlock, blocks);
                Block *tempL = leftBlock;
                auto retired_lower_space = ExtractTile(blocks, tempL->lb);
                auto retired_left_space = ExtractTile(blocks, tempL);
            }
        }
        // merge right tile
        if (rightBlock && rightBlock->rt && !rightBlock->rt->isSolid) {
            if (rightBlock->rt->x == rightBlock->x && rightBlock->rt->width == rightBlock->width) {
                auto mergeBlock_owner =
                    std::make_unique<Block>(0, rightBlock->x, rightBlock->y, rightBlock->width,
                                            rightBlock->height + rightBlock->rt->height, false);
                Block *mergeBlock = mergeBlock_owner.get();
                blocks.push_back(std::move(mergeBlock_owner));
                UpdateCornerStitches(mergeBlock, blocks);
                UpdateNeighborBlocks(mergeBlock, blocks);
                Block *tempR = rightBlock;
                auto retired_upper_space = ExtractTile(blocks, tempR->rt);
                auto retired_right_space = ExtractTile(blocks, tempR);
                rightBlock = nullptr;
                if (mergeBlock && mergeBlock->lb && !mergeBlock->lb->isSolid) {
                    if (mergeBlock->lb->x == mergeBlock->x &&
                        mergeBlock->lb->width == mergeBlock->width) {

                        auto mergeBlock_temp_owner = std::make_unique<Block>(
                            0, mergeBlock->lb->x, mergeBlock->lb->y, mergeBlock->width,
                            mergeBlock->height + mergeBlock->lb->height, false);
                        Block *mergeBlock_temp = mergeBlock_temp_owner.get();
                        blocks.push_back(std::move(mergeBlock_temp_owner));
                        UpdateCornerStitches(mergeBlock_temp, blocks);
                        UpdateNeighborBlocks(mergeBlock_temp, blocks);
                        Block *tempR = mergeBlock;
                        auto retired_lower_space = ExtractTile(blocks, tempR->lb);
                        auto retired_merged_right = ExtractTile(blocks, tempR);
                    }
                }
            }
        }
        if (rightBlock && rightBlock->lb && !rightBlock->lb->isSolid) {
            if (rightBlock->lb->x == rightBlock->x && rightBlock->lb->width == rightBlock->width) {

                auto mergeBlock_owner = std::make_unique<Block>(
                    0, rightBlock->lb->x, rightBlock->lb->y, rightBlock->width,
                    rightBlock->height + rightBlock->lb->height, false);
                Block *mergeBlock = mergeBlock_owner.get();
                blocks.push_back(std::move(mergeBlock_owner));
                UpdateCornerStitches(mergeBlock, blocks);
                UpdateNeighborBlocks(mergeBlock, blocks);
                Block *tempR = rightBlock;
                auto retired_lower_space = ExtractTile(blocks, tempR->lb);
                auto retired_right_space = ExtractTile(blocks, tempR);
            }
        }

        // remove overlapping tile
        Block *temp = currentBlock;
        auto retired_current_tile = ExtractTile(blocks, temp);
        start_y = start_y - temp->height;
        currentBlock = Point_Finding(blocks.front().get(), start_x, start_y);

        if (!currentBlock)
            break;
    }
    // merge the internal tiles into new tile
    for (auto &block : remain_internalBlock) {
        auto retired_internal_tile = ExtractTile(blocks, block);
    }
    blocks.push_back(std::move(newBlock_owner));
    UpdateCornerStitches(newBlock, blocks);
    UpdateNeighborBlocks(newBlock, blocks);
}

void UpdateCornerStitches(Block *newBlock, TileList &blocks) {
    if (blocks.uses_index()) {
        blocks.repair(newBlock);
        return;
    }
    ++blocks.stitch_queries;
    for (auto &block : blocks) {
        ++blocks.candidate_visits;
        // check rt (top)
        if (block->y == newBlock->y + newBlock->height) {
            if (block->x < newBlock->x + newBlock->width &&
                newBlock->x + newBlock->width <= block->x + block->width) {
                newBlock->rt = block.get();
            }
        }
        // check tr (right)
        if (block->x == newBlock->x + newBlock->width) {
            if (block->y < newBlock->y + newBlock->height &&
                newBlock->y + newBlock->height <= block->y + block->height) {
                newBlock->tr = block.get();
            }
        }
        // check lb (bottom)
        if (block->y + block->height == newBlock->y) {
            if (block->x <= newBlock->x && newBlock->x < block->x + block->width) {
                newBlock->lb = block.get();
            }
        }
        // check bl (left)
        if (block->x + block->width == newBlock->x) {
            if (block->y <= newBlock->y && newBlock->y < block->y + block->height) {
                newBlock->bl = block.get();
            }
        }
    }
}

void UpdateNeighborBlocks(Block *newBlock, TileList &blocks) {
    std::list<Block *> neighbor = Neighbor_Finding(newBlock);
    for (auto &block : neighbor) {
        UpdateCornerStitches(block, blocks);
    }
}

Block *FindBlockContainingTopEdge(Block *newBlock, TileList &blocks) {
    return blocks.find_space_edge(newBlock->x, newBlock->width, newBlock->y + newBlock->height);
}

void SplitTop(Block *newBlock, Block *spaceBlock, TileList &blocks) {
    int edge_y_pos = newBlock->y + newBlock->height;
    // split the above tile
    int aboveheight = spaceBlock->y + spaceBlock->height - edge_y_pos;
    auto aboveBlock_owner = std::make_unique<Block>(0, spaceBlock->x, edge_y_pos, spaceBlock->width,
                                                    aboveheight, false);
    Block *aboveBlock = aboveBlock_owner.get();
    // split the overlapping tile
    int overlappingheight = spaceBlock->height - aboveheight;
    auto overlappingBlock_owner = std::make_unique<Block>(
        0, spaceBlock->x, spaceBlock->y, spaceBlock->width, overlappingheight, false);
    Block *overlappingBlock = overlappingBlock_owner.get();
    // delete the oringinal tile
    auto retired_split_space = ExtractTile(blocks, spaceBlock);

    // put slpit tiles into list
    blocks.push_back(std::move(aboveBlock_owner));
    blocks.push_back(std::move(overlappingBlock_owner));

    // update tiles adjoining the newtiles
    UpdateCornerStitches(aboveBlock, blocks);
    UpdateCornerStitches(overlappingBlock, blocks);
    UpdateNeighborBlocks(aboveBlock, blocks);
    UpdateNeighborBlocks(overlappingBlock, blocks);
    if (aboveBlock->rt && !aboveBlock->rt->isSolid) {
        if (aboveBlock->rt->x == aboveBlock->x && aboveBlock->rt->width == aboveBlock->width) {

            auto mergeBlock_owner =
                std::make_unique<Block>(0, aboveBlock->x, aboveBlock->y, aboveBlock->width,
                                        aboveBlock->height + aboveBlock->rt->height, false);
            Block *mergeBlock = mergeBlock_owner.get();
            Block *tempL = aboveBlock;
            auto retired_upper_space = ExtractTile(blocks, tempL->rt);
            auto retired_above_tile = ExtractTile(blocks, tempL);
            blocks.push_back(std::move(mergeBlock_owner));
            UpdateCornerStitches(mergeBlock, blocks);
            UpdateNeighborBlocks(mergeBlock, blocks);
        }
    }
}

Block *FindBlockContainingBottomEdge(Block *newBlock, TileList &blocks) {
    return blocks.find_space_edge(newBlock->x, newBlock->width, newBlock->y);
}

void SplitBottom(Block *newBlock, Block *spaceBlock, TileList &blocks) {
    int edge_y_pos = newBlock->y;
    // split the above tile
    int belowheight = edge_y_pos - spaceBlock->y;
    auto belowBlock_owner = std::make_unique<Block>(0, spaceBlock->x, spaceBlock->y,
                                                    spaceBlock->width, belowheight, false);
    Block *belowBlock = belowBlock_owner.get();

    int overlappingheight = spaceBlock->height - belowheight;
    auto overlappingBlock_owner = std::make_unique<Block>(
        0, spaceBlock->x, edge_y_pos, spaceBlock->width, overlappingheight, false);
    Block *overlappingBlock = overlappingBlock_owner.get();

    // delete the oringinal tile
    auto retired_split_space = ExtractTile(blocks, spaceBlock);

    // put slpit tiles into list
    blocks.push_back(std::move(belowBlock_owner));
    blocks.push_back(std::move(overlappingBlock_owner));

    // update tiles adjoining the newtiles
    UpdateCornerStitches(belowBlock, blocks);
    UpdateCornerStitches(overlappingBlock, blocks);
    UpdateNeighborBlocks(belowBlock, blocks);
    UpdateNeighborBlocks(overlappingBlock, blocks);

    if (belowBlock && belowBlock->lb && !belowBlock->lb->isSolid) {
        if (belowBlock->lb->x == belowBlock->x && belowBlock->lb->width == belowBlock->width) {
            auto mergeBlock_owner = std::make_unique<Block>(
                0, belowBlock->lb->x, belowBlock->lb->y, belowBlock->lb->width,
                belowBlock->height + belowBlock->lb->height, false);
            Block *mergeBlock = mergeBlock_owner.get();
            blocks.push_back(std::move(mergeBlock_owner));
            UpdateCornerStitches(mergeBlock, blocks);
            UpdateNeighborBlocks(mergeBlock, blocks);
            Block *temp = belowBlock;
            auto retired_lower_space = ExtractTile(blocks, temp->lb);
            auto retired_below_tile = ExtractTile(blocks, temp);
        }
    }
}
