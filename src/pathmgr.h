#pragma once

#include <string>

// Check if extension of the file in "path" is ".hzip"
bool isHzip(const std::string& path);

// Returns "path" without file extension
std::string removeExtension(const std::string& path);

// Returns "path" file extension (without the '.' preceding extension)
std::string getExtension(const std::string& path);

// Returns the path were the compressed version of the file on "originalFilePath" will be created
std::string setCompressedFilePath(const std::string& originalFilePath);