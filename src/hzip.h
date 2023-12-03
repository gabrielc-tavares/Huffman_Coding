#pragma once

#include "huffman_tree.hpp"

// Compresses the content of the given file using Huffman coding.
void zip(const std::string& srcFilePath);

// Decompress the given file using Huffman coding.
void unzip(const std::string& srcFilePath);