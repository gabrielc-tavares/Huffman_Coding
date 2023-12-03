#pragma once

#include <iostream>
#include <fstream>
#include <string>

constexpr int BUFFER_SIZE = 1024; ///< The size of the buffer used for reading/writing files.

/**
 * @class InputFileRAII
 * @brief RAII wrapper for input file handling.
 *
 * This class ensures the safe and automatic management of an input file resource.
 */
class InputFileRAII {
public:
	/**
	 * @brief Constructor for InputFileRAII.
	 *
	 * @param filename The name of the input file.
	 * @throws std::runtime_error if the file fails to open.
	 */
	InputFileRAII(const std::string& filename) 
		: file(filename, std::ios::binary | std::ios::in) {
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file: " + filename);
		}
		file.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE); // Set buffer size
	}

	/**
	 * @brief Destructor for InputFileRAII.
	 *
	 * Closes the file if it is still open.
	 */
	~InputFileRAII() {
		if (file.is_open()) {
			file.close();
		}
	}

	/**
	 * @brief Checks if the input file is open.
	 *
	 * @return true if the file is open, false otherwise.
	 */
	bool isOpen() const {
		return file.is_open();
	}

	/**
	 * @brief Provides access to the underlying ifstream.
	 *
	 * @return Reference to the underlying ifstream.
	 */
	std::ifstream& get() {
		return file;
	}

private:
	std::ifstream file; ///< Underlying input file stream.
	char buffer[BUFFER_SIZE]; ///< Buffer for reading.
};

/**
 * @class OutputFileRAII
 * @brief RAII wrapper for output file handling.
 *
 * This class ensures the safe and automatic management of an output file resource.
 */
class OutputFileRAII {
public:
	/**
	 * @brief Constructor for OutputFileRAII.
	 *
	 * @param filename The name of the output file.
	 * @throws std::runtime_error if the file fails to open.
	 */
	OutputFileRAII(const std::string& filename) 
		: file(filename, std::ios::binary | std::ios::out) {
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file: " + filename);
		}
		file.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE); // Set buffer size
	}

	/**
	 * @brief Destructor for OutputFileRAII.
	 *
	 * Closes the file if it is still open.
	 */
	~OutputFileRAII() {
		if (file.is_open()) {
			file.close();
		}
	}

	/**
	 * @brief Checks if the output file is open.
	 *
	 * @return true if the file is open, false otherwise.
	 */
	bool isOpen() const {
		return file.is_open();
	}

	/**
	 * @brief Provides access to the underlying ofstream.
	 *
	 * @return Reference to the underlying ofstream.
	 */
	std::ofstream& get() {
		return file;
	}

private:
	std::ofstream file; ///< Underlying output file stream.
	char buffer[BUFFER_SIZE]; ///< Buffer for writing
};