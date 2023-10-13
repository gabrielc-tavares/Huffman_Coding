#pragma once

#include <string>

// Check if extension of the file in "path" is ".hzip"
bool is_hzip(const std::string& path);

// Returns 'path' without file extension
std::string stem(const std::string& path);

// Returns 'path' file extension (without the '.' preceding extension)
std::string extension(const std::string& path);