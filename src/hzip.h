#pragma once

#include "huffman_tree.hpp"

// Compress file
void zip(const std::string& srcPath);

// Decompress file
void unzip(const std::string& srcPath);