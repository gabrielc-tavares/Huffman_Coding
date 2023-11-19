#pragma once

#include "huffman_tree.hpp"

// Compress file
void zip(const std::string& srcFilePath);

// Decompress file
void unzip(const std::string& srcFilePath);