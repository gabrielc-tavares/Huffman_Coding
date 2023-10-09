#include "hzip.h"
#include <cmath>

using NodePtr_t = std::shared_ptr<HuffmanTreeNode>;

// Returns 'path' without file extension
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

// Returns 'path' file extension (without the '.' preceding extension)
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

bool is_hzip(const std::string& path) {
	return extension(path) == "hzip";
}

void zip(const std::string& srcPath) {
	// Build Huffman Tree
	HuffmanTree huffTree(srcPath);

	// Create RAII handle for input file
	InputFileRAII scopedInputFile(srcPath);
	std::ifstream& srcHandle = scopedInputFile.get();

	std::string destPath = stem(srcPath);
	destPath += ".hzip";

	// Create RAII handle for output file
	OutputFileRAII scopedOutputFile(destPath);
	std::ofstream& destHandle = scopedOutputFile.get();

	// Get Huffman Tree leaves
	std::vector<NodePtr_t> leaves = huffTree.getLeaves();

	// Write original file extension in the output file preceded by its size
	std::string ext = extension(srcPath);
	uint8_t extSize = static_cast<uint8_t>(ext.size());
	destHandle.put(extSize);
	destHandle.write(ext.c_str(), ext.size());

	// Get number of different characters in the original file and write it on the output file
	uint8_t uniqueBytes = (uint8_t)leaves.size();
	destHandle.put(uniqueBytes);

	// Get number of bytes required to store frequencies and write it on the output file
	uint8_t bytesForFreq = (uint8_t)std::ceil(std::log2((double)(huffTree.getBiggerFrequency())) / 8);
	destHandle.put(bytesForFreq);
	
	// Write at the output file each character present in the input file followed by its frequency 
	for (auto iter = leaves.begin(); iter != leaves.end(); ++iter) {
		destHandle.put((*iter)->getCharacter());

		size_t freqBuf = (*iter)->getFrequency();
		size_t mask = 0xFF;
		mask <<= ((size_t)(bytesForFreq - 1) * 8);
		uint8_t freq = (uint8_t)((freqBuf & mask) >> ((size_t)(bytesForFreq - 1) * 8));

		destHandle.put(freq);

		for (int i = 1; i < bytesForFreq; i++) {
			mask >>= 8;
			freq = (uint8_t)((freqBuf & mask) >> ((size_t)(bytesForFreq - 1 - i) * 8));
			destHandle.put(freq);
		}
	}
	const uint16_t maxSize = 256;

	// Get encoded version of the characters in the input file paired with its original form
	EncodedCharMap_t encodedCharMap = huffTree.getEncodedCharMap();

	EncodedChar_t destBitset(0, 0), newBitset(0, 0);
	uint8_t srcBuf[BUFFER_SIZE];

	// Read a chunk of data from the input file into the buffer
	srcHandle.read(reinterpret_cast<char*>(srcBuf), BUFFER_SIZE);

	// Get the number of bytes read in this chunk
	std::streamsize bytesRead = srcHandle.gcount();

	while(bytesRead == BUFFER_SIZE) {
		// Compress chunk read
		for (int i = 0; i < BUFFER_SIZE; i++) {
			newBitset = encodedCharMap[srcBuf[i]];
			int free = maxSize - destBitset.second;

			if (free > newBitset.second) {
				int shl = free - newBitset.second;

				newBitset.first <<= shl;
				destBitset.first |= newBitset.first;
				destBitset.second += newBitset.second;
			}
			else {
				EncodedChar_t auxBitset(newBitset);
				int shr = newBitset.second - free;

				auxBitset.first >>= shr;
				destBitset.first |= auxBitset.first;

				// Write 'outBitset' in output file
				std::vector<uint8_t> destBitsetVec;
				uint256_t tempBigInt;

				for (int k = 0; k < maxSize / CHAR_BIT; k++) {
					tempBigInt = UCHAR_MAX;
					tempBigInt &= destBitset.first;
					destBitsetVec.emplace_back(tempBigInt.convert_to<uint8_t>());
					destBitset.first >>= CHAR_BIT;
				}
				while (!destBitsetVec.empty()) {
					uint8_t destByte = destBitsetVec.back();
					destHandle.put(destByte);
					destBitsetVec.pop_back();
				}

				newBitset.first <<= (maxSize - shr);
				destBitset.first = newBitset.first;
				destBitset.second = shr;
			}
		}
		// Read a chunk of data from the input file into the buffer
		srcHandle.read(reinterpret_cast<char*>(srcBuf), BUFFER_SIZE);

		// Get the number of bytes read in this chunk
		bytesRead = srcHandle.gcount();
	}

	// If the number of bytes in a chunk is < BUFFER_SIZE and > 0
	if (bytesRead > 0) {
		// Compress chunk read
		for (int i = 0; i < bytesRead; i++) {
			newBitset = encodedCharMap[srcBuf[i]];
			int free = maxSize - destBitset.second;

			if (free > newBitset.second) {
				int shl = free - newBitset.second;

				newBitset.first <<= shl;
				destBitset.first |= newBitset.first;
				destBitset.second += newBitset.second;
			}
			else {
				EncodedChar_t auxBitset(newBitset);
				int shr = newBitset.second - free;

				auxBitset.first >>= shr;
				destBitset.first |= auxBitset.first;

				// Write 'outBitset' in output file
				std::vector<uint8_t> destBitsetVec;
				uint256_t tempBigInt;

				for (int k = 0; k < maxSize / CHAR_BIT; k++) {
					tempBigInt = UCHAR_MAX;
					tempBigInt &= destBitset.first;
					destBitsetVec.emplace_back(tempBigInt.convert_to<uint8_t>());
					destBitset.first >>= CHAR_BIT;
				}
				while (!destBitsetVec.empty()) {
					uint8_t destByte = destBitsetVec.back();
					destHandle.put(destByte);
					destBitsetVec.pop_back();
				}

				newBitset.first <<= (maxSize - shr);
				destBitset.first = newBitset.first;
				destBitset.second = shr;
			}
		}
	}

	if (destBitset.second > 0) {
		// There is still bits from the encoded character to write on the output file
		int bitsLeft = maxSize - destBitset.second;
		destBitset.first >>= bitsLeft;

		std::vector<uint8_t> destBitsetVec;
		uint256_t tempBigInt;

		for (int j = 0; j < (destBitset.second + CHAR_BIT) / CHAR_BIT; j++) {
			tempBigInt = UCHAR_MAX;
			tempBigInt &= destBitset.first;
			destBitsetVec.emplace_back(tempBigInt.convert_to<uint8_t>());
			destBitset.first >>= CHAR_BIT;
		}
		while (!destBitsetVec.empty()) {
			uint8_t destByte = destBitsetVec.back();
			destHandle.put(destByte);
			destBitsetVec.pop_back();
		}
	}

	std::cout << "File compressed successfully." << std::endl;
}

void unzip(const std::string& srcPath) {
	// Create RAII handle for input file
	InputFileRAII scopedInputFile(srcPath);
	std::ifstream& srcHandle = scopedInputFile.get();

	// Set destiny file path
	std::string destPath = stem(srcPath);
	destPath += '.';

	uint8_t extSize;
	srcHandle.read(reinterpret_cast<char*>(&extSize), 1);

	// Get original file extension
	char* ext = new char[extSize + 1];
	srcHandle.read(ext, extSize);
	ext[extSize] = '\0';
	destPath += ext;

	// Create RAII handle for output file
	OutputFileRAII scopedOutputFile(destPath);
	std::ofstream& destHandle = scopedOutputFile.get();

	// Get number of different characters of the original file
	uint8_t uniqueBytes;
	srcHandle.read(reinterpret_cast<char*>(&uniqueBytes), 1);

	// Get number of bytes required to represent the characters frequencies
	uint8_t bytesForFreq;
	srcHandle.read(reinterpret_cast<char*>(&bytesForFreq), 1);

	size_t serialInfoSize = (size_t)uniqueBytes * ((size_t)bytesForFreq + 1);
	uint8_t* serialInfoBuf = new uint8_t[serialInfoSize];

	// Read information required to restore the original file data
	srcHandle.read(reinterpret_cast<char*>(serialInfoBuf), serialInfoSize);

	std::vector<NodePtr_t> leaves;
	uint8_t charBuf;

	// Get characters of the original file along with its frequencies
	for (int i = 0; i < uniqueBytes; i++) {
		size_t index = i * (bytesForFreq + 1);
		charBuf = serialInfoBuf[index];

		size_t freq = static_cast<size_t>(serialInfoBuf[index + 1]);

		for (int j = 1; j < bytesForFreq; j++) {
			freq <<= 8;
			freq |= serialInfoBuf[index + j + 1];
		}
		NodePtr_t newNodePtr = std::make_shared<HuffmanTreeNode>(HuffmanTreeNode(charBuf, freq));
		leaves.push_back(newNodePtr);
	}
	delete[] serialInfoBuf;

	// Build Huffman Tree from leaf nodes
	HuffmanTree huffTree(leaves);
	// Get Huffman Tree root
	NodePtr_t nodePtr = huffTree.getRoot();
	// Get number of encoded "bytes" of the compressed file
	size_t encodedBytes = nodePtr->getFrequency();

	// Buffer to read from input file
	uint8_t srcBuf[BUFFER_SIZE];
	size_t encodedBytesRead = 0;
	std::streamsize bytesRead;

	do {
		// Read a chunk of data from the input file into the buffer
		srcHandle.read(reinterpret_cast<char*>(srcBuf), BUFFER_SIZE);

		// Get the number of bytes read in this chunk
		bytesRead = srcHandle.gcount();

		for (int i = 0; i < bytesRead; i++) {
			uint8_t srcByte = srcBuf[i];
			uint8_t mask = 0x80;

			do {
				if ((srcByte & mask) == 0)
					nodePtr = nodePtr->getLeft();
				else
					nodePtr = nodePtr->getRight();

				if (nodePtr->isLeaf()) {
					// Get character from leaf node
					uint8_t destByte = nodePtr->getCharacter();

					// Write character in the decompressed file
					destHandle.put(destByte);
					encodedBytesRead++;

					// Check if end of file was reached
					if (encodedBytesRead == encodedBytes) {
						// Input file was completely decompressed
						std::cout << "File decompressed successfully." << std::endl;
						return;
					}
					// Make 'ptrNode' point to the root again
					nodePtr = huffTree.getRoot();
				}
				mask >>= 1;
			} while (mask != 0);
		}
	} while (bytesRead > 0);

	throw std::runtime_error("Error: File could not be decompressed correctly.");
}