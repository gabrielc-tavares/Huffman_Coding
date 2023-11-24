#pragma once

#include "huffman_tree.hpp"

// Compress the file in the given path
void zip(const std::string& srcFilePath);

// Decompress the file in the given path
void unzip(const std::string& srcFilePath);