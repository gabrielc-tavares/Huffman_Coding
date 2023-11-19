#include "hzip.h"
#include <cmath>

// Returns the minimum number of bytes required to represent an unsigned integer n
uint8_t byteSize(size_t n) {
	return static_cast<uint8_t>(std::ceil(std::log2((double)n) / 8));
}

// Right shift "bitsToShift" bits of the "encodedByte" codeword
void shiftRight(EncodedByte& encodedByte, int bitsToShift) {
	int begin = 0;

	encodedByte.numberOfBits += bitsToShift;

	if (bitsToShift >= 8) {
		begin = bitsToShift / 8;
		for (int i = 0; i < begin; i++)
			encodedByte.codeword.insert(encodedByte.codeword.begin(), 0);
		bitsToShift %= 8;
	}
	if (bitsToShift > 0) {
		if (encodedByte.numberOfBits % 8 != 0)
			encodedByte.codeword.emplace_back(0);

		uint8_t shiftedBits = 0;
		for (int i = begin; i < encodedByte.codeword.size(); i++) {
			uint8_t nextShiftedBits = (encodedByte.codeword[i] << (8 - bitsToShift));
			encodedByte.codeword[i] = (encodedByte.codeword[i] >> bitsToShift) | shiftedBits;
			shiftedBits = nextShiftedBits;
		}
	}
}

void zip(const std::string& srcFilePath) {
	HuffmanTree huffmanTree;
	try {
		// Build HuffmanTree from source file
		huffmanTree = HuffmanTree(srcFilePath);
	} catch (std::exception& e) {
		// Rethrow exception raised while building Huffman Tree to be handled by the main
		throw;
	}
	// Create RAII handle for input file
	InputFileRAII scopedInputFile(srcFilePath);
	std::ifstream& srcFile = scopedInputFile.get();

	// Create RAII handle for output file
	std::string destFilePath = removeExtension(srcFilePath) + ".hzip";
	OutputFileRAII scopedOutputFile(destFilePath);
	std::ofstream& destFile = scopedOutputFile.get();

	std::vector<HuffmanTreeNodePtr> leaves = huffmanTree.getLeaves();
	PrefixFreeBinCode encodingDict = huffmanTree.getEncodingDict();

	// Number of codewords (i.e distinct bytes in the original file and its extension)
	uint8_t uniqueBytes = static_cast<uint8_t>(leaves.size());
	// Number of bytes required to store frequencies
	uint8_t bytesForFreq = byteSize(huffmanTree.getHigherFrequency());

	// Required data to enable the decompression of the compressed file
	size_t huffmanMetadataSize = (size_t)uniqueBytes * (bytesForFreq + 1) + 2;
	uint8_t* huffmanMetadata = new uint8_t[huffmanMetadataSize];
	huffmanMetadata[0] = uniqueBytes;
	huffmanMetadata[1] = bytesForFreq;

	// Each distinct byte in the original file will be written at the beginning of the 
	// compressed file along with its frequency
	int metadataIndex = 2;
	for (HuffmanTreeNodePtr node : leaves) {
		huffmanMetadata[metadataIndex] = node->getOriginalByte();
		size_t frequency = node->getFrequency();

		for (int i = bytesForFreq; i >= 1; i--) {
			huffmanMetadata[metadataIndex + i] = static_cast<uint8_t>(frequency & 0xFF);
			frequency >>= 8;
		}
		metadataIndex += bytesForFreq + 1;
	}
	destFile.write(reinterpret_cast<char*>(huffmanMetadata), huffmanMetadataSize);
	delete[] huffmanMetadata;

	uint8_t srcBuffer[BUFFER_SIZE];
	uint8_t destBuffer[BUFFER_SIZE] = { 0 };
	const int destBufferNumBits = BUFFER_SIZE * 8;
	int destBufferIndex = 0;
	int destBufferFreeBits = destBufferNumBits;
	std::streamsize bytesRead;

	// The extension of the source file will be compressed along with its content
	std::string srcFileExt = extension(srcFilePath) + ' ';
	size_t srcFileExtLen = srcFileExt.size();

	for (int i = 0; i < srcFileExtLen; i++)
		srcBuffer[i] = static_cast<uint8_t>(srcFileExt[i]);

	// Read a chunk of data from the input file and get the number of bytes read
	srcFile.read(reinterpret_cast<char*>(srcBuffer + srcFileExtLen), BUFFER_SIZE - srcFileExtLen);
	if ((bytesRead = srcFile.gcount() + srcFileExtLen) == 0) 
		throw std::runtime_error("Failed to read source file");
	
	do {
		for (int i = 0; i < bytesRead; i++) {
			EncodedByte encodedByte = encodingDict[srcBuffer[i]];
			int bitsWritten = destBufferNumBits - destBufferFreeBits;
			int destBufferIndex = bitsWritten / 8;
			int shr = bitsWritten % 8;

			destBufferFreeBits -= encodedByte.numberOfBits;
			shiftRight(encodedByte, shr);

			if (destBufferFreeBits <= 0) {
				destBuffer[destBufferIndex] |= encodedByte.codeword[0];
				
				destFile.write(reinterpret_cast<char*>(destBuffer), destBufferIndex + 1);

				memset(destBuffer, 0, BUFFER_SIZE * sizeof(*destBuffer));
				destBufferIndex = 0;
				destBufferFreeBits = destBufferNumBits;
				
				if (encodedByte.codeword.size() == 1) continue; 
				
				encodedByte.codeword.erase(encodedByte.codeword.begin());
				encodedByte.numberOfBits -= 8;

				destBufferFreeBits -= encodedByte.numberOfBits;
			}
			for (auto iter = encodedByte.codeword.begin(); iter != encodedByte.codeword.end(); ++iter) {
				destBuffer[destBufferIndex] |= *iter;
				destBufferIndex++;
			}
		}
		srcFile.read(reinterpret_cast<char*>(srcBuffer), BUFFER_SIZE);
		bytesRead = srcFile.gcount();
	} while (bytesRead > 0);

	if (destBufferFreeBits < destBufferNumBits) {
		// Some bits have not been written to the destination file yet
		int remainingBits = destBufferNumBits - destBufferFreeBits;
		int remainingBytes = (remainingBits + 7) / 8;
		destFile.write(reinterpret_cast<char*>(destBuffer), remainingBytes);
	}
	std::cout << "File compressed successfully" << std::endl;
}

void unzip(const std::string& srcFilePath) {
	// Create RAII handle for input file
	InputFileRAII scopedInputFile(srcFilePath);
	std::ifstream& srcHandle = scopedInputFile.get();

	// The decompressed file will be created with the same name and in the 
	// same directory as the compressed file, but with the original file extension
	std::string destPath = removeExtension(srcFilePath) + '.';

	// Number of codewords (i.e distinct bytes in the original file and its extension)
	uint8_t uniqueBytes;
	srcHandle.read(reinterpret_cast<char*>(&uniqueBytes), 1);
	// Number of bytes required to store frequencies
	uint8_t bytesForFreq;
	srcHandle.read(reinterpret_cast<char*>(&bytesForFreq), 1);

	// Get metadata string about the original file characters and its frequencies to enable the decompression
	size_t huffmanMetadataSize = (size_t)uniqueBytes * ((size_t)bytesForFreq + 1);
	uint8_t* huffmanMetadata = new uint8_t[huffmanMetadataSize];
	srcHandle.read(reinterpret_cast<char*>(huffmanMetadata), huffmanMetadataSize);

	// Build Huffman Tree leaves to decode the encoded bytes
	std::vector<HuffmanTreeNodePtr> leaves;
	for (int i = 0; i < uniqueBytes; i++) {
		size_t idx = i * (bytesForFreq + 1);
		uint8_t originalByte = huffmanMetadata[idx];
		size_t frequency = static_cast<size_t>(huffmanMetadata[idx + 1]);

		for (int j = 1; j < bytesForFreq; j++) {
			frequency <<= 8;
			frequency |= static_cast<size_t>(huffmanMetadata[idx + j + 1]);
		}
		leaves.emplace_back(std::make_shared<HuffmanTreeNode>(HuffmanTreeNode(originalByte, frequency)));
	}
	delete[] huffmanMetadata;

	HuffmanTree huffmanTree;
	try {
		// Build Huffman Tree from its leaf nodes
		huffmanTree = HuffmanTree(leaves);
	} catch (std::exception& e) {
		// Rethrow exception raised while building Huffman Tree to be handled by the main
		throw;
	}

	uint8_t srcBuffer[BUFFER_SIZE]; // Buffer to read from input file
	HuffmanTreeNodePtr nodePtr = huffmanTree.getRoot(); // Get Huffman Tree root
	size_t bytesToDecode = huffmanTree.getNumberOfBytes(); // Get number of encoded bytes in the zipped file to be decoded
	size_t currentByte = 0;

	// Read first chunk of data (which contains the original fle extension at the beginning) 
	srcHandle.read(reinterpret_cast<char*>(srcBuffer), BUFFER_SIZE);
	// Get the number of bytes read in this chunk
	std::streamsize bytesRead = srcHandle.gcount();

	while (bytesRead > 0) {
		uint8_t srcByte = srcBuffer[currentByte];
		uint8_t mask = 0x80;

		do {
			if ((srcByte & mask) == 0) {
				nodePtr = nodePtr->getLeft();
			} else {
				nodePtr = nodePtr->getRight();
			}

			if (nodePtr->isLeaf()) {
				// Get character from leaf node
				uint8_t decodedByte = nodePtr->getOriginalByte();
				bytesToDecode--;

				// Make node pointer points back to the tree root
				nodePtr = huffmanTree.getRoot();

				if (decodedByte == ' ') {
					// If the decoded character is a space, the extension has been fully read
					// Create RAII handle for output file and start restoring the original file content
					OutputFileRAII scopedOutputFile(destPath);
					std::ofstream& destHandle = scopedOutputFile.get();

					// Check if end of file was reached (original file was empty)
					if (bytesToDecode == 0) {
						std::cout << "File decompressed successfully" << std::endl;
						return;
					}
					mask >>= 1; // Continue reading from current byte

					while(true) {
						while (mask != 0) {
							if ((srcByte & mask) == 0) {
								nodePtr = nodePtr->getLeft();
							} else {
								nodePtr = nodePtr->getRight();
							}

							if (nodePtr->isLeaf()) {
								// Get character from leaf node
								decodedByte = nodePtr->getOriginalByte();
								bytesToDecode--;

								// Write character in the decompressed file
								destHandle.put(decodedByte);

								// Check if end of file was reached
								if (bytesToDecode == 0) {
									std::cout << "File decompressed successfully" << std::endl;
									return;
								}
								// Make node pointer points back to the tree root
								nodePtr = huffmanTree.getRoot();
							}
							mask >>= 1;
						}
						mask = 0x80;
						currentByte++;

						if (currentByte == bytesRead) {
							// Chunk of data was totally read
							// "Clear" buffer, read another chunk and continue the decompression
							currentByte = 0;
							srcHandle.read(reinterpret_cast<char*>(srcBuffer), BUFFER_SIZE);
							bytesRead = srcHandle.gcount();

							if (bytesRead == 0) {
								// If no bytes were read here, some error must have occurred
								throw std::runtime_error("Error: File could not be decompressed correctly");
							}
						}
						srcByte = srcBuffer[currentByte];
					}
				} else {
					// Otherwise, append the decoded character in the destiny path
					destPath += static_cast<char>(decodedByte);
				}
			}
			mask >>= 1;
		} while (mask != 0);
		currentByte++;
	}
	throw std::runtime_error("Error: File could not be decompressed correctly");
}