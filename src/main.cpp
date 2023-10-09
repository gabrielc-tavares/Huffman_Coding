#include "hzip.h"

int main(int argc, char** argv) {
	std::string srcPath;
	// Check if the correct number of arguments has been passed
	if (argc == 2) {
		srcPath = argv[1];
	}
	else if (argc == 1) {
		// If the user didn't provided a file path, ask them to do it
		std::cout << "Enter the file that you want to compress or decompress:" << std::endl;
		std::cin >> srcPath;
	}
	else {
		throw std::runtime_error("Error: Too many arguments.");
	}

	// Check if input file must be compressed or decompressed
	if (is_hzip(srcPath)) {
		// If the input file has a ".hzip" extension, it will be decompressed. The decompressed
		// file will be created in the same directory as the input file.
		unzip(srcPath);
	}
	else {
		// If the input file doesn't have a ".hzip" extention, it will be compressed into a 
		// ".hzip" file, which will be created in the same directory as the input file.
		zip(srcPath);
	}

	return 0;
}