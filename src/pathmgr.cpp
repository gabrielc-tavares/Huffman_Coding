#include <iostream>
#include "pathmgr.h"

bool isHzip(const std::string& filePath) {
	std::string ext;

	try {
		ext = getExtension(filePath);
	}
	catch (std::runtime_error& e) {
		throw;
	}

	return ext == "hzip";
}

std::string removeExtension(const std::string& filePath) {
	std::string stem(filePath);

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

std::string getExtension(const std::string& filePath) {
	std::string temp(filePath);
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

std::string setCompressedFilePath(const std::string& originalFilePath) {
	std::string pathWithoutExt;

	try {
		pathWithoutExt = removeExtension(originalFilePath);
	} catch (std::runtime_error& e) {
		throw;
	}

	return pathWithoutExt + ".hzip";
}