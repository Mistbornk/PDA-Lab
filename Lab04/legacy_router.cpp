#include "input.hpp"
#include "pda/io.hpp"
#include "router.hpp"
#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdint>
#include <iostream>
#include <queue>

using namespace std;
namespace {
template <class T> struct FlatGrid {
    std::size_t columns;
    std::vector<T> values;
    FlatGrid(int rows, int cols)
        : columns(static_cast<std::size_t>(cols)),
          values(static_cast<std::size_t>(rows) * columns) {}
    T *operator[](int row) { return values.data() + static_cast<std::size_t>(row) * columns; }
};
class Router {
    RoutingAreaInfo RA_info;
    GridInfo grid_info;
    vector<Net> nets;
    vector<vector<GCell>> gcells;
    CostInfo cost_info;
    FlatGrid<Node> gcellDetails;
    FlatGrid<std::uint8_t> closedList;
    // Neighbor arithmetic uses signed coordinates; all accesses follow grid bounds
    // checks (or validated input endpoints). Convert once at the storage boundary.
    GCell &cell(int row, int col) {
        return gcells[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
    }
    double layer_cost(int layer, int row, int col) const {
        const auto &costs = layer == 1 ? cost_info.layer1_cost : cost_info.layer2_cost;
        return costs[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
    }
    double Heuristic(Pair src, Pair dest) {
        int x1 = cell(src.first, src.second).x, y1 = cell(src.first, src.second).y;
        int x2 = cell(dest.first, dest.second).x, y2 = cell(dest.first, dest.second).y;

        return abs(static_cast<double>(x1) - x2) + abs(static_cast<double>(y1) - y2);
    }

    double calculateOverflow(const int &edgeUsage, const int &edgeCapacity,
                             const double &maxCellCost) {
        if (edgeUsage > edgeCapacity) {
            return (edgeUsage - edgeCapacity) * 0.5 * maxCellCost;
        }
        return 0.0;
    }

    void updateCapacity(vector<Pair> &path_capacity) {
        for (size_t i = 0; i + 1 < path_capacity.size(); i++) {
            int row1 = path_capacity[i].first;
            int col1 = path_capacity[i].second;
            int row2 = path_capacity[i + 1].first;
            int col2 = path_capacity[i + 1].second;

            if (row1 == row2 && col1 < col2) {
                cell(row2, col2).left_usage++;
            } else if (row1 == row2 && col1 > col2) {
                cell(row1, col1).left_usage++;
            } else if (col1 == col2 && row1 < row2) {
                cell(row2, col2).bottom_usage++;
            } else if (col1 == col2 && row1 > row2) {
                cell(row1, col1).bottom_usage++;
            }
        }
    }

    void reconstructPath(Pair src, Pair dest, ostream &output) {
        vector<Pair> path; // {x,y}
        vector<Pair> path_capacity;
        int row = dest.first, col = dest.second;
        while (row != src.first || col != src.second) {
            path.push_back({cell(row, col).x, cell(row, col).y});
            path_capacity.push_back({row, col});
            int temprow = gcellDetails[row][col].parent_i;
            int tempcol = gcellDetails[row][col].parent_j;
            row = temprow;
            col = tempcol;
        }
        path.push_back({cell(row, col).x, cell(row, col).y});
        path_capacity.push_back({row, col});

        reverse(path.begin(), path.end());
        reverse(path_capacity.begin(), path_capacity.end());

        int currentLayer = 1;
        if (path.size() > 1 && path[0].first != path[1].first) {
            output << "via" << '\n';
            currentLayer = 2;
        }
        int startX = path[0].first, startY = path[0].second;
        size_t i;
        for (i = 1; i < path.size(); i++) {
            int layer = (path[i].first == path[i - 1].first) ? 1 : 2;
            if (layer != currentLayer) {
                output << "M" << currentLayer << " " << startX << " " << startY << " "
                       << path[i - 1].first << " " << path[i - 1].second << '\n';
                output << "via" << '\n';

                startX = path[i - 1].first, startY = path[i - 1].second;
                currentLayer = layer;
            }
        }
        output << "M" << currentLayer << " " << startX << " " << startY << " " << path[i - 1].first
               << " " << path[i - 1].second << '\n';

        if (currentLayer != 1) {
            output << "via" << '\n';
        }
        updateCapacity(path_capacity);
    }

    void AstarSearch(Pair src, Pair dest, ostream &output) {
        int NUM_ROW = RA_info.num_rows, NUM_COL = RA_info.num_cols;
        std::fill(closedList.values.begin(), closedList.values.end(), 0);

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

        priority_queue<pPair, vector<pPair>, greater<>> openList;

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

            if (closedList[i][j])
                continue;
            closedList[i][j] = true;

            if (i == dest.first && j == dest.second) {
                foundDest = true;
                reconstructPath(src, dest, output);
                break;
            }

            static constexpr std::array<Pair, 2> directionsM1 = {{{1, 0}, {-1, 0}}}; // up and down
            for (auto &dir : directionsM1) {
                int newRow = i + dir.first, newCol = j + dir.second;
                if (newRow >= 0 && newRow < NUM_ROW && newCol >= 0 && newCol < NUM_COL) {
                    double wireLength = grid_info.GridHeight;
                    double cellCost =
                        (currentLayer == 2)
                            ? (layer_cost(1, newRow, newCol) + layer_cost(2, newRow, newCol)) / 2
                            : layer_cost(1, newRow, newCol);

                    double overflowCost;
                    if (dir.first == 1) // moving up
                        overflowCost = calculateOverflow(gcells[static_cast<std::size_t>(newRow)]
                                                               [static_cast<std::size_t>(newCol)]
                                                                   .bottom_usage,
                                                         gcells[static_cast<std::size_t>(newRow)]
                                                               [static_cast<std::size_t>(newCol)]
                                                                   .bottom_capacity,
                                                         cost_info.max_cellcost);
                    else // moving down
                        overflowCost =
                            calculateOverflow(gcells[static_cast<std::size_t>(newRow + 1)]
                                                    [static_cast<std::size_t>(newCol)]
                                                        .bottom_usage,
                                              gcells[static_cast<std::size_t>(newRow + 1)]
                                                    [static_cast<std::size_t>(newCol)]
                                                        .bottom_capacity,
                                              cost_info.max_cellcost);

                    double viaCost = (currentLayer == 2)
                                         ? cost_info.via_cost
                                         : 0.0; // Add viaCost if switching from M2 to M1
                    double gNew = gcellDetails[i][j].g + cost_info.alpha * wireLength +
                                  cost_info.beta * overflowCost + cost_info.gamma * cellCost +
                                  cost_info.delta * viaCost;
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

            static constexpr std::array<Pair, 2> directionsM2 = {
                {{0, -1}, {0, 1}}}; // Left and Right
            for (auto &dir : directionsM2) {
                int newRow = i + dir.first, newCol = j + dir.second;
                if (newRow >= 0 && newRow < NUM_ROW && newCol >= 0 && newCol < NUM_COL) {
                    double wireLength = grid_info.GridWidth;
                    double cellCost =
                        (currentLayer == 1)
                            ? (layer_cost(1, newRow, newCol) + layer_cost(2, newRow, newCol)) / 2
                            : layer_cost(2, newRow, newCol);
                    double overflowCost;
                    if (dir.second == 1) // moving right
                        overflowCost = calculateOverflow(gcells[static_cast<std::size_t>(newRow)]
                                                               [static_cast<std::size_t>(newCol)]
                                                                   .left_usage,
                                                         gcells[static_cast<std::size_t>(newRow)]
                                                               [static_cast<std::size_t>(newCol)]
                                                                   .left_capacity,
                                                         cost_info.max_cellcost);
                    else // moving left
                        overflowCost =
                            calculateOverflow(gcells[static_cast<std::size_t>(newRow)]
                                                    [static_cast<std::size_t>(newCol + 1)]
                                                        .left_usage,
                                              gcells[static_cast<std::size_t>(newRow)]
                                                    [static_cast<std::size_t>(newCol + 1)]
                                                        .left_capacity,
                                              cost_info.max_cellcost);

                    double viaCost = (currentLayer == 1)
                                         ? cost_info.via_cost
                                         : 0.0; // Add viaCost if switching from M1 to M2
                    double gNew = gcellDetails[i][j].g + cost_info.alpha * wireLength +
                                  cost_info.gamma * cellCost + cost_info.beta * overflowCost +
                                  cost_info.delta * viaCost;
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

        pda::require(foundDest, "No path found");
    }

  public:
    explicit Router(lab4::Input input)
        : RA_info(input.area), grid_info(input.grid), nets(std::move(input.nets)),
          gcells(std::move(input.cells)), cost_info(std::move(input.costs)),
          gcellDetails(RA_info.num_rows, RA_info.num_cols),
          closedList(RA_info.num_rows, RA_info.num_cols) {}
    void route(ostream &output) {
        for (const auto &net : nets) {
            const Pair start{(net.bump1_y - RA_info.Routing_Area_Y) / grid_info.GridHeight,
                             (net.bump1_x - RA_info.Routing_Area_X) / grid_info.GridWidth};
            const Pair end{(net.bump2_y - RA_info.Routing_Area_Y) / grid_info.GridHeight,
                           (net.bump2_x - RA_info.Routing_Area_X) / grid_info.GridWidth};
            output << "n" << net.idx << '\n';
            AstarSearch(start, end, output);
            output << ".end\n";
        }
    }
};
} // namespace
namespace lab4 {
void route_legacy(Input input, std::ostream &output) { Router(std::move(input)).route(output); }
} // namespace lab4
