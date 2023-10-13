#include <iostream>
#include "pathmgr.h"

bool is_hzip(const std::string& path) {
	return extension(path) == "hzip";
}

std::string stem(const std::string& path) {
	std::string stem(path);

	while (!stem.empty()) {
		if (stem.back() == '.') {
			stem.pop_back();
			return stem;
		}
		stem.pop_back();
	}
	// If the file path doesn't contain an explicit extension, throw an exception
	throw std::runtime_error("Error: Invalid file path format (extension must be explicit).");
}

std::string extension(const std::string& path) {
	std::string temp(path);
	std::string ext;

	while (!temp.empty() && temp.back() != '/') {
		if (temp.back() == '.')
			return ext;

		ext.insert(ext.begin(), temp.back());
		temp.pop_back();
	}
	// If the file path doesn't contain an explicit extension, throw an exception
	throw std::runtime_error("Error: Invalid file path format (extension must be explicit).");
}