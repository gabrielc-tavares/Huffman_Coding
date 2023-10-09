#pragma once

#include "huffman_tree.hpp"

// Check if file path has a ".hzip" extension
bool is_hzip(const std::string& path);

// Compress file
void zip(const std::string& srcPath);

// Decompress file
void unzip(const std::string& srcPath);