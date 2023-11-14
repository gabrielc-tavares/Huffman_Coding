#include "hzip.h"
#include <cmath>

void zip(const std::string& srcPath) {
	// Build HuffmanTree from source file
	HuffmanTree huffTree(srcPath);

	// Create RAII handle for input file
	InputFileRAII scopedInputFile(srcPath);
	std::ifstream& srcHandle = scopedInputFile.get();

	// The compressed file will be created with the same name and in the 
	// same directory as the original file, but with a ".hzip" extension
	std::string destPath = stem(srcPath) + ".hzip";

	// Create RAII handle for output file
	OutputFileRAII scopedOutputFile(destPath);
	std::ofstream& destHandle = scopedOutputFile.get();

	// Get HuffmanTree leaves
	std::vector<NodePtr> leaves = huffTree.getLeaves();

	// Get number of different bytes of the original file
	uint8_t uniqueBytes = static_cast<uint8_t>(leaves.size());
	
	// Get number of bytes required to store frequencies
	uint8_t bytesForFreq = static_cast<uint8_t>(std::ceil(std::log2((double)huffTree.getHigherFrequency()) / 8));

	// To enable the decompression of the file, data about the content of the original file will be 
	// written at the beginning of the compressed file
	size_t metadataSize = (size_t)uniqueBytes * (bytesForFreq + 1) + 2;
	uint8_t* metadata = new uint8_t[metadataSize];

	metadata[0] = uniqueBytes;
	metadata[1] = bytesForFreq;

	// To decode the content of the compressed file, each character present in the original file 
	// will be written at the beginning of the compressed file along with its frequencies
	int idx = 2;
	for (auto iter = leaves.begin(); iter != leaves.end(); ++iter) {
		metadata[idx] = (*iter)->getOriginalByte();
		size_t freq = (*iter)->getFrequency();

		for (int i = bytesForFreq; i >= 1; i--) {
			metadata[idx + i] = static_cast<uint8_t>(freq & 0xFF);
			freq >>= 8;
		}
		idx += bytesForFreq + 1;
	}
	destHandle.write(reinterpret_cast<char*>(metadata), metadataSize);
	delete[] metadata;

	// Maximum size possible of an encoded byte (in bits)
	const int maxEncodedSize = 256;

	// Get encoded version of the characters in the input file along mapped with its original forms
	HuffmanEncodingMap encodingMap = huffTree.getEncodingMap();

	HuffmanEncoded destEncoded, newEncoded;
	uint8_t srcBuf[BUFFER_SIZE];

	// Get extension of the source file (it will be compressed along with the source 
	// file content by being placed before it and separated from it with a space).
	std::string srcExt = extension(srcPath) + ' ';
	size_t srcExtLen = srcExt.size();

	for (int i = 0; i < srcExtLen; i++)
		srcBuf[i] = static_cast<uint8_t>(srcExt[i]);

	// Read a chunk of data from the input file
	srcHandle.read(reinterpret_cast<char*>(srcBuf + srcExtLen), BUFFER_SIZE - srcExtLen);
	// Get the number of bytes read in this chunk
	std::streamsize bytesRead = srcHandle.gcount() + srcExtLen;

	if (bytesRead == 0) 
		throw std::runtime_error("Error: File could not be compressed correctly.");

	// Buffer to write encoded bytes in the destination file
	uint8_t* destBuf = new uint8_t[maxEncodedSize / 8];

	do {
		// Compress chunk
		for (int i = 0; i < bytesRead; i++) {
			newEncoded = encodingMap[srcBuf[i]];
			int free = maxEncodedSize - destEncoded.bitCount;

			if (free > newEncoded.bitCount) {
				int shl = free - newEncoded.bitCount;
				newEncoded.encodedByte <<= shl;
				destEncoded.encodedByte |= newEncoded.encodedByte;
				destEncoded.bitCount += newEncoded.bitCount;
			}
			else {
				HuffmanEncoded newEncodedAux = newEncoded;
				int shr = newEncoded.bitCount - free;
				newEncodedAux.encodedByte >>= shr;
				destEncoded.encodedByte |= newEncodedAux.encodedByte;

				for (int j = maxEncodedSize / 8 - 1; j >= 0; j--) {
					destBuf[j] = (destEncoded.encodedByte & 0xFF).convert_to<uint8_t>();
					destEncoded.encodedByte >>= 8;
				}
				destHandle.write(reinterpret_cast<char*>(destBuf), maxEncodedSize / 8);

				newEncoded.encodedByte <<= (maxEncodedSize - shr);
				destEncoded.encodedByte = newEncoded.encodedByte;
				destEncoded.bitCount = shr;
			}
		}
		// Read a chunk of data from the input file into the buffer
		srcHandle.read(reinterpret_cast<char*>(srcBuf), BUFFER_SIZE);
		// Get the number of bytes read in this chunk
		bytesRead = srcHandle.gcount();
	} while (bytesRead > 0);
	delete[] destBuf;

	if (destEncoded.bitCount > 0) {
		// Some bits have not been written to the destination file yet
		destEncoded.encodedByte >>= (maxEncodedSize - destEncoded.bitCount);

		size_t remainingBytes = (destEncoded.bitCount + 7) / 8;
		uint8_t* remainingBuffer = new uint8_t[remainingBytes];

		for (int i = remainingBytes - 1; i >= 0; i--) {
			// Extract a byte from destEncoded.encodedByte
			remainingBuffer[i] = (destEncoded.encodedByte & 0xFF).convert_to<uint8_t>();
			destEncoded.encodedByte >>= 8;
		}
		// Write the remaining bytes to the file
		destHandle.write(reinterpret_cast<char*>(remainingBuffer), remainingBytes);
		delete[] remainingBuffer;
	}
	// File compression ended with no errors
	std::cout << "File compressed successfully." << std::endl;
}

void unzip(const std::string& srcPath) {
	// Create RAII handle for input file
	InputFileRAII scopedInputFile(srcPath);
	std::ifstream& srcHandle = scopedInputFile.get();

	// The decompressed file will be created with the same name and in the 
	// same directory as the compressed file, but with the original file extension.
	std::string destPath = stem(srcPath) + '.';

	// Get number of different characters of the original file
	uint8_t uniqueBytes;
	srcHandle.read(reinterpret_cast<char*>(&uniqueBytes), 1);

	// Get number of bytes required to represent the characters frequencies
	uint8_t bytesForFreq;
	srcHandle.read(reinterpret_cast<char*>(&bytesForFreq), 1);

	size_t metadataSize = (size_t)uniqueBytes * ((size_t)bytesForFreq + 1);
	uint8_t* metadataBuf = new uint8_t[metadataSize];

	// Read information required to restore the original file data
	srcHandle.read(reinterpret_cast<char*>(metadataBuf), metadataSize);

	// Get characters of the original file along with its frequencies
	std::vector<NodePtr> leaves;
	for (int i = 0; i < uniqueBytes; i++) {
		size_t idx = i * (bytesForFreq + 1);
		uint8_t originalByte = metadataBuf[idx];
		size_t frequency = static_cast<size_t>(metadataBuf[idx + 1]);

		for (int j = 1; j < bytesForFreq; j++) {
			frequency <<= 8;
			frequency |= static_cast<size_t>(metadataBuf[idx + j + 1]);
		}
		leaves.emplace_back(std::make_shared<Node>(Node(originalByte, frequency)));
	}
	delete[] metadataBuf;

	HuffmanTree huffTree(leaves); // Build Huffman Tree from leaf nodes
	NodePtr nodePtr = huffTree.getRoot(); // Get Huffman Tree root
	size_t encodedBytes = huffTree.getNumberOfBytes(); // Number of encoded bytes in the zipped file
	size_t encodedBytesRead = 0, i = 0;
	uint8_t srcBuf[BUFFER_SIZE]; // Buffer to read from input file

	// Read first chunk of data (which contains the original fle extension at the beginning)
	srcHandle.read(reinterpret_cast<char*>(srcBuf), BUFFER_SIZE);
	// Get the number of bytes read in this chunk
	std::streamsize bytesRead = srcHandle.gcount();

	while (bytesRead > 0) {
		uint8_t srcByte = srcBuf[i];
		uint8_t mask = 0x80;

		do {
			if ((srcByte & mask) == 0) {
				nodePtr = nodePtr->getLeft();
			} else {
				nodePtr = nodePtr->getRight();
			}

			if (nodePtr->isLeaf()) {
				// Get character from leaf node
				uint8_t destByte = nodePtr->getOriginalByte();
				encodedBytesRead++;

				// Make node pointer points back to the tree root
				nodePtr = huffTree.getRoot();

				// If the decoded character is a space, the extension has been fully read
				// Start restoring the original file content from the compressed file
				if (destByte == 32) {
					// Create RAII handle for output file
					OutputFileRAII scopedOutputFile(destPath);
					std::ofstream& destHandle = scopedOutputFile.get();

					// Check if end of file was reached (original file was empty)
					if (encodedBytesRead == encodedBytes) {
						std::cout << "File decompressed successfully." << std::endl;
						return;
					}
					mask >>= 1; // Continue reading from current byte

					do {
						while (mask != 0) {
							if ((srcByte & mask) == 0) {
								nodePtr = nodePtr->getLeft();
							} else {
								nodePtr = nodePtr->getRight();
							}

							if (nodePtr->isLeaf()) {
								// Get character from leaf node
								destByte = nodePtr->getOriginalByte();

								// Write character in the decompressed file
								destHandle.put(destByte);
								encodedBytesRead++;

								// Check if end of file was reached
								if (encodedBytesRead == encodedBytes) {
									std::cout << "File decompressed successfully." << std::endl;
									return;
								}
								// Make node pointer points back to the tree root
								nodePtr = huffTree.getRoot();
							}
							mask >>= 1;
						}
						mask = 0x80;
						i++;

						if (i == bytesRead) {
							i = 0;
							// Read a chunk of data from the input file
							srcHandle.read(reinterpret_cast<char*>(srcBuf), BUFFER_SIZE);
							// Get the number of bytes read in this chunk
							bytesRead = srcHandle.gcount();
							// Stop decompression process if no bytes were read
							if (bytesRead == 0) break;
						}
						srcByte = srcBuf[i];
					} while (true);

					throw std::runtime_error("Error: File could not be decompressed correctly.");
				} else {
					// Otherwise, append the decoded character in the destiny path
					destPath += static_cast<char>(destByte);
				}
			}
			mask >>= 1;
		} while (mask != 0);
		i++;
	}
	throw std::runtime_error("Error: File could not be decompressed correctly.");
}