#include "hzip.h"
#include <filesystem>
#include <cmath>

using NodePtr_t = std::shared_ptr<HuffmanTreeNode>;

void zip(const std::string& srcPath) {
	// Build Huffman Tree
	HuffmanTree huffTree(srcPath);

	// Create RAII handle for input file
	InputFileRAII scopedInputFile(srcPath);
	std::ifstream& srcHandle = scopedInputFile.get();

	// Compressed file will be created in the same directory and with the 
	// same name of the original file, but with a ".hzip" extension
	std::string destPath = stem(srcPath);
	destPath += ".hzip";

	// Create RAII handle for output file
	OutputFileRAII scopedOutputFile(destPath);
	std::ofstream& destHandle = scopedOutputFile.get();

	// Get Huffman Tree leaves
	std::vector<NodePtr_t> leaves = huffTree.getLeaves();

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
	// Maximum size possible of an encoded character
	const uint16_t maxSize = 256;

	// Get encoded version of the characters in the input file paired with its original form
	EncodedCharMap_t encodedCharMap = huffTree.getEncodedCharMap();

	EncodedChar_t destEncoded(0, 0), newEncoded(0, 0);
	uint8_t srcBuf[BUFFER_SIZE];

	// Get original file extension (without the '.')
	std::string srcExt = extension(srcPath);

	// A space will indicate the end of the original file extension
	srcExt.push_back(' ');

	size_t srcExtLen = srcExt.size();

	// Write original file extension at the beginning of the buffer to be compressed along 
	// the rest of the data
	for (int i = 0; i < srcExtLen; i++)
		srcBuf[i] = static_cast<uint8_t>(srcExt[i]);

	// Read a chunk of data from the input file into the buffer
	srcHandle.read(reinterpret_cast<char*>(srcBuf + srcExtLen), BUFFER_SIZE - srcExtLen);

	// Get the number of bytes read in this chunk
	std::streamsize bytesRead = srcHandle.gcount() + srcExtLen;

	while (bytesRead == BUFFER_SIZE) {
		// Compress chunk read
		for (int i = 0; i < BUFFER_SIZE; i++) {
			newEncoded = encodedCharMap[srcBuf[i]];
			int free = maxSize - destEncoded.second;

			if (free > newEncoded.second) {
				int shl = free - newEncoded.second;

				newEncoded.first <<= shl;
				destEncoded.first |= newEncoded.first;
				destEncoded.second += newEncoded.second;
			}
			else {
				EncodedChar_t auxBitset(newEncoded);
				int shr = newEncoded.second - free;

				auxBitset.first >>= shr;
				destEncoded.first |= auxBitset.first;

				// Write 'outBitset' in output file
				std::vector<uint8_t> destBitsetVec;
				uint256_t tempBigInt;

				for (int k = 0; k < maxSize / CHAR_BIT; k++) {
					tempBigInt = UCHAR_MAX;
					tempBigInt &= destEncoded.first;
					destBitsetVec.emplace_back(tempBigInt.convert_to<uint8_t>());
					destEncoded.first >>= CHAR_BIT;
				}
				while (!destBitsetVec.empty()) {
					uint8_t destByte = destBitsetVec.back();
					destHandle.put(destByte);
					destBitsetVec.pop_back();
				}

				newEncoded.first <<= (maxSize - shr);
				destEncoded.first = newEncoded.first;
				destEncoded.second = shr;
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
			newEncoded = encodedCharMap[srcBuf[i]];
			int free = maxSize - destEncoded.second;

			if (free > newEncoded.second) {
				int shl = free - newEncoded.second;

				newEncoded.first <<= shl;
				destEncoded.first |= newEncoded.first;
				destEncoded.second += newEncoded.second;
			}
			else {
				EncodedChar_t auxBitset(newEncoded);
				int shr = newEncoded.second - free;

				auxBitset.first >>= shr;
				destEncoded.first |= auxBitset.first;

				// Write 'outBitset' in output file
				std::vector<uint8_t> destEncodedVec;
				uint256_t tempBigInt;

				for (int k = 0; k < maxSize / CHAR_BIT; k++) {
					tempBigInt = UCHAR_MAX;
					tempBigInt &= destEncoded.first;
					destEncodedVec.emplace_back(tempBigInt.convert_to<uint8_t>());
					destEncoded.first >>= CHAR_BIT;
				}
				while (!destEncodedVec.empty()) {
					uint8_t destByte = destEncodedVec.back();
					destHandle.put(destByte);
					destEncodedVec.pop_back();
				}

				newEncoded.first <<= (maxSize - shr);
				destEncoded.first = newEncoded.first;
				destEncoded.second = shr;
			}
		}
	}

	if (destEncoded.second > 0) {
		// Some bits have not been written to the destination file yet
		int bitsLeft = maxSize - destEncoded.second;
		destEncoded.first >>= bitsLeft;

		std::vector<uint8_t> destEncodedVec;
		uint256_t tempBigInt;

		for (int j = 0; j < (destEncoded.second + (destEncoded.second % CHAR_BIT)) / CHAR_BIT; j++) {
			tempBigInt = UCHAR_MAX;
			tempBigInt &= destEncoded.first;
			destEncodedVec.emplace_back(tempBigInt.convert_to<uint8_t>());
			destEncoded.first >>= CHAR_BIT;
		}
		while (!destEncodedVec.empty()) {
			uint8_t destByte = destEncodedVec.back();
			destHandle.put(destByte);
			destEncodedVec.pop_back();
		}
	}

	// File compression ended with no errors
	std::cout << "File compressed successfully." << std::endl;
}

void unzip(const std::string& srcPath) {
	// Create RAII handle for input file
	InputFileRAII scopedInputFile(srcPath);
	std::ifstream& srcHandle = scopedInputFile.get();

	// Decompressed file will be created with the same name of the compressed file and 
	// with the original file extension (which is compressed along with the rest of the data)
	std::string destPath(stem(srcPath));
	destPath.push_back('.');

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

	// Get characters of the original file along with its frequencies
	std::vector<NodePtr_t> leaves;
	for (int i = 0; i < uniqueBytes; i++) {
		size_t index = i * (bytesForFreq + 1);
		uint8_t charBuf = serialInfoBuf[index];

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
	size_t encodedBytes = huffTree.getNumberOfBytes();

	// Buffer to read from input file
	uint8_t srcBuf[BUFFER_SIZE];

	// Read first chunk of data (which contains the original fle extension at the beginning)
	srcHandle.read(reinterpret_cast<char*>(srcBuf), BUFFER_SIZE);
	// Get the number of bytes read in this chunk
	std::streamsize bytesRead = srcHandle.gcount();

	size_t encodedBytesRead = 0;
	size_t i = 0;

	while (bytesRead > 0) {
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
				encodedBytesRead++;

				// Make 'ptrNode' point to the root again
				nodePtr = huffTree.getRoot();

				// If the decoded character is a space, the extension has been fully read
				// Start restoring the original file content from the compressed file
				if (destByte == 32) {
					// Create RAII handle for output file
					OutputFileRAII scopedOutputFile(destPath);
					std::ofstream& destHandle = scopedOutputFile.get();

					// Check if end of file was reached (original file was empty)
					if (encodedBytesRead == encodedBytes) {
						// Original file was completely restored
						std::cout << "File decompressed successfully." << std::endl;
						return;
					}
					// Continue reading from current byte
					mask >>= 1;

					// Decompress data
					do {
						while (mask != 0) {
							if ((srcByte & mask) == 0)
								nodePtr = nodePtr->getLeft();
							else
								nodePtr = nodePtr->getRight();

							if (nodePtr->isLeaf()) {
								// Get character from leaf node
								destByte = nodePtr->getCharacter();

								// Write character in the decompressed file
								destHandle.put(destByte);
								encodedBytesRead++;

								// Check if end of file was reached
								if (encodedBytesRead == encodedBytes) {
									// Original file was completely restored
									std::cout << "File decompressed successfully." << std::endl;
									return;
								}
								// Make 'ptrNode' point to the root again
								nodePtr = huffTree.getRoot();
							}
							mask >>= 1;
						}
						mask = 0x80;
						i++;
						if (i == bytesRead) {
							i = 0;
							// Read a chunk of data from the input file into the buffer
							srcHandle.read(reinterpret_cast<char*>(srcBuf), BUFFER_SIZE);
							// Get the number of bytes read in this chunk
							bytesRead = srcHandle.gcount();
							if (bytesRead == 0)
								break;
						}
						srcByte = srcBuf[i];
					} while (true);

					throw std::runtime_error("Error: File could not be decompressed correctly.");
				}
				// Otherwise, append the decoded character in the destiny path
				else {
					destPath += static_cast<char>(destByte);
				}
			}
			mask >>= 1;
		} while (mask != 0);
		i++;
	}

	throw std::runtime_error("Error: File could not be decompressed correctly.");
}