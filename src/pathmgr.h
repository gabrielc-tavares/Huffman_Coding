#pragma once

#include <string>

/**
 * @brief Check if the given file has a 'hzip' extension.
 *
 * This function determines whether a file has the 'hzip' extension based on its path.
 *
 * @param filePath The path to the file.
 * @return true if the file has the 'hzip' extension, false otherwise.
 * @throw std::runtime_error if the file path doesn't contain an explicit extension.
 */
bool isHzip(const std::string& path);

/**
 * @brief Remove the extension from the given file path.
 *
 * This function removes the extension from a file path and returns the resulting string.
 * If the file path doesn't contain an explicit extension, it throws a runtime error.
 *
 * @param filePath The path to the file.
 * @return The file path without the extension.
 * @throw std::runtime_error if the file path doesn't contain an explicit extension.
 */
std::string removeExtension(const std::string& path);

/**
 * @brief Get the extension of the given file path.
 *
 * This function retrieves the extension of a file from its path and returns the resulting string.
 * If the file path doesn't contain an explicit extension, it throws a runtime error.
 *
 * @param filePath The path to the file.
 * @return The extension of the file.
 * @throw std::runtime_error if the file path doesn't contain an explicit extension.
 * 
 * @note The returned extension didn't include the '.' character preceding the extension.
 */
std::string getExtension(const std::string& path);

/**
 * @brief Set the compressed file path by replacing the extension with ".hzip".
 *
 * This function takes an original file path, removes its extension, and appends ".hzip" to
 * create the compressed file path.
 *
 * @param originalFilePath The path to the original file.
 * @return The path to the compressed file.
 * @throw std::runtime_error if the file path doesn't contain an explicit extension.
 */
std::string setCompressedFilePath(const std::string& originalFilePath);