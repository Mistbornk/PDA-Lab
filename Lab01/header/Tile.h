#pragma once
class Block {
  public:
    Block(int idx, int x_pos, int y_pos, int w, int h, bool solid)
        : index(idx), x(x_pos), y(y_pos), width(w), height(h), isSolid(solid) {}
    const int index;
    const int x, y;
    const int width, height;
    const bool isSolid;
    Block *rt = nullptr, *tr = nullptr, *bl = nullptr, *lb = nullptr;
};
