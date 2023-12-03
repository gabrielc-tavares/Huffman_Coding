#pragma once

#include "huffman_tree.hpp"

/**
 * @brief Compresses the content of the given file using Huffman coding.
 *
 * This function compresses the content of the source file located at `srcFilePath`
 * using Huffman coding. The compressed file is written to a destination file, that
 * will be created in the same directory and with the same name of the source file,
 * but with a '.hzip' extension.
 *
 * @param srcFilePath The path to the source file to be compressed.
 * @throws std::exception if an error occurs during the compression process.
 *
 * @details
 * The compression process involves the following steps:
 * - Building a Huffman Tree based on the frequency of each byte in the source file.
 * - Writing Huffman coding metadata at the beginning of the compressed file.
 * - Encoding the content of the source file using Huffman coding and writing it to the compressed file.
 *
 * @note This function assumes that the source file is accessible and properly formatted.
 * @note The function does not handle file stream errors or exceptions.
 */
void zip(const std::string& srcFilePath);

/**
 * @brief Decompress the given file using Huffman coding.
 *
 * This function decompresses a file that was previously compressed using Huffman
 * coding. It reads the compressed file, extracts Huffman coding metadata, builds
 * Huffman tree, and then decodes the original file content.
 *
 * @param srcFilePath The path to the compressed file to be decompressed.
 * @throws std::runtime_error if there is an issue during the decompression process.
 *
 * @details
 * The function starts by opening the compressed file and creating an RAII handle for input.
 * Huffman coding metadata is read from the compressed file, and Huffman tree leaves
 * are constructed. The Huffman tree is then built using these leaves.
 * The function proceeds to read the encoded bytes from the compressed file, traverse
 * the Huffman tree, and decode the original file content. The decompressed file is
 * created, and the content is written to it.
 * The decompressed file is created in the same directory and with the same name
 * as the compressed file, but with the original file extension (that was compressed along
 * with its data).
 *
 * @note This function assumes that the compressed file contains valid Huffman coding metadata.
 * @note The function does not handle file stream errors or exceptions.
 */
void unzip(const std::string& srcFilePath);