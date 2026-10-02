#pragma once
#include "model.hpp"
#include <istream>
namespace lab2 {
Problem parse(std::istream &blocks, std::istream &nets);
Problem parse(const std::string &blocks, const std::string &nets);
} // namespace lab2
