#pragma once

#include <iostream>
#include <fstream>
#include <string>

// Buffer for input/output files
constexpr int BUFFER_SIZE = 0xFFF;

class InputFileRAII {
public:
	InputFileRAII(const std::string& filename) : file(filename, std::ios::binary | std::ios::in) {
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file: " + filename);
		}
		file.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE); // Set buffer size
	}

	~InputFileRAII() {
		if (file.is_open()) {
			file.close();
		}
	}

	// Provide a method to check if the file is open
	bool isOpen() const {
		return file.is_open();
	}

	// Provide access to the underlying ifstream
	std::ifstream& get() {
		return file;
	}

private:
	std::ifstream file;
	char buffer[BUFFER_SIZE]; // Buffer for reading
};

class OutputFileRAII {
public:
	OutputFileRAII(const std::string& filename) : file(filename, std::ios::binary | std::ios::out) {
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file: " + filename);
		}
		file.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE); // Set buffer size
	}

	~OutputFileRAII() {
		if (file.is_open()) {
			file.close();
		}
	}

	// Provide a method to check if the file is open
	bool isOpen() const {
		return file.is_open();
	}

	// Provide access to the underlying ofstream
	std::ofstream& get() {
		return file;
	}

private:
	std::ofstream file;
	char buffer[BUFFER_SIZE]; // Buffer for writing
};