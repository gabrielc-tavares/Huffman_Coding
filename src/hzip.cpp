#include "hzip.h"

/**
 * @brief Calculate the minimum number of bytes required to represent an unsigned integer.
 *
 * Given an unsigned integer value, this function calculates the minimum number of bytes
 * required to represent that value. The result is based on the ceiling of the logarithm
 * base 2 of the input, divided by 8.
 *
 * @param n The unsigned integer value for which to calculate the byte size.
 *
 * @return The minimum number of bytes required to represent the input value.
 *
 * @note The result is cast to uint8_t for compactness.
 */
uint8_t byteSize(size_t n) {
	return static_cast<uint8_t>(std::ceil(std::log2((double)n) / 8));
}

/**
 * @brief Shifts the bits in the given EncodedByte's codeword to the right.
 *
 * This function shifts the bits in the codeword of the provided EncodedByte
 * to the right by the specified number of bits. It updates the numberOfBits
 * field accordingly.
 *
 * @param encodedByte The EncodedByte to be modified.
 * @param bitsToShift The number of bits to shift to the right.
 *
 * @details
 * If bitsToShift is greater than or equal to 8, additional zero bytes are inserted
 * at the beginning of the codeword to accommodate the shift.
 * If the total number of bits in the codeword plus the number of bits to shift is
 * not a multiple of 8, a new byte is added to accommodate the shifted bits.
 * All bits of the given EncodedByte's codeword are preserved, thus the number of
 * bits in it is increased by the number of bits shifted.
 *
 * @note This function assumes that the provided EncodedByte is properly initialized.
 */
void shiftRight(EncodedByte& encodedByte, int bitsToShift) {
	int bytesToInsert = 0;

	// Update the total number of bits in the codeword
	encodedByte.numberOfBits += bitsToShift;

	// Handle cases where bitsToShift is greater than or equal to 8
	if (bitsToShift >= 8) {
		// Calculate the number of bytes to insert at the beginning
		bytesToInsert = bitsToShift / 8;

		// Insert zero bytes at the beginning
		encodedByte.codeword.insert(encodedByte.codeword.begin(), bytesToInsert, 0);

		// Update bitsToShift to the remaining bits after inserting zero bytes
		bitsToShift %= 8;
	}
	if (bitsToShift > 0) {
		// Ensure byte alignment if the total number of bits is not a multiple of 8
		if (encodedByte.numberOfBits % 8 != 0)
			encodedByte.codeword.emplace_back(0);

		uint8_t shiftedBits = 0;

		// Perform the right shift on each byte in the codeword
		for (int i = bytesToInsert; i < encodedByte.codeword.size(); i++) {
			uint8_t nextShiftedBits = (encodedByte.codeword[i] << (8 - bitsToShift));
			encodedByte.codeword[i] = (encodedByte.codeword[i] >> bitsToShift) | shiftedBits;
			shiftedBits = nextShiftedBits;
		}
	}
}

/**
 * @brief Write Huffman coding metadata at the beginning of the compressed file.
 *
 * This function writes information about the original file content at the beginning
 * of the compressed file to allow its original content to be decoded later. The metadata
 * includes the number of distinct bytes, the number of bytes required to store frequencies,
 * and the distinct bytes along with their frequencies.
 *
 * @param destFile An output file stream representing the compressed file.
 * @param huffmanTree The Huffman tree used for compression.
 *
 * @details
 * The following data will be written at the beginning of the compressed file:
 * - byte 0: An integer n representing the number of codewords (distinct bytes).
 * - byte 1: An integer f representing the number of bytes required to store frequencies.
 * - bytes 2 to n*(f+1)+1: Distinct bytes present in the original file and their respective frequencies.
 *
 * @note This function assumes that the provided HuffmanTree is properly constructed.
 */
void writeHuffmanMetadata(std::ofstream& destFile, const HuffmanTree& huffmanTree) {
	// Get the leaves of the Huffman tree
	std::vector<HuffmanTreeNodePtr> leaves = huffmanTree.getLeaves();

	// Number of codewords (distinct bytes)
	uint8_t uniqueBytes = static_cast<uint8_t>(leaves.size());
	// Number of bytes required to store frequencies
	uint8_t bytesForFreq = byteSize(huffmanTree.getHigherFrequency());

	// Required data to enable the decompression of the compressed file
	size_t huffmanMetadataSize = (size_t)uniqueBytes * (bytesForFreq + 1) + 2;
	uint8_t* huffmanMetadata = new uint8_t[huffmanMetadataSize];
	huffmanMetadata[0] = uniqueBytes;
	huffmanMetadata[1] = bytesForFreq;

	// Write each distinct byte along with its frequency
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

	// Write the Huffman metadata to the compressed file
	destFile.write(reinterpret_cast<char*>(huffmanMetadata), huffmanMetadataSize);
	delete[] huffmanMetadata;
}

/**
 * @brief Compresses the content of the given file using Huffman coding.
 *
 * This function compresses the content of the source file located at `srcFilePath`
 * using Huffman coding. The compressed file is written to a destination file, that 
 * will be created in the same directory and with the same name of the source file,
 * but with a '.hzip' extension.
 *
 * @param srcFilePath The path to the source file to be compressed.
 *
 * @throws std::exception if an error occurs during the compression process.
 *
 * @details
 * The compression process involves the following steps:
 * - Building a Huffman Tree based on the frequency of each byte in the source file.
 * - Writing Huffman coding metadata at the beginning of the compressed file.
 * - Encoding the content of the source file using Huffman coding and writing it to the compressed file.
 *
 * @note This function assumes that the source file is accessible and properly formatted.
 * @note The function does not handle file stream errors or exceptions.
 */
void zip(const std::string& srcFilePath) {
	// Build HuffmanTree from source file
	HuffmanTree huffmanTree;
	try {
		huffmanTree = HuffmanTree(srcFilePath);
	} catch (std::exception& e) {
		// Rethrow exception raised while building Huffman Tree to be handled by the main
		throw;
	}

	// Create RAII handle for input file
	InputFileRAII scopedInputFile(srcFilePath);
	std::ifstream& srcFile = scopedInputFile.get();

	// Set the destination file path for the compressed file
	std::string destFilePath = setCompressedFilePath(srcFilePath);

	// Create RAII handle for output file
	OutputFileRAII scopedOutputFile(destFilePath);
	std::ofstream& destFile = scopedOutputFile.get();

	// Write Huffman metadata at the beginning of the compressed file
	writeHuffmanMetadata(destFile, huffmanTree);

	// Get the Huffman encoding dictionary
	PrefixFreeBinCode encodingDict = huffmanTree.getEncodingDict();

	// Buffers for reading from the source file and writing to the destination file
	uint8_t srcBuffer[BUFFER_SIZE], destBuffer[BUFFER_SIZE] = { 0 };

	// The number of bits in the destination buffer
	const int destBufferNumBits = BUFFER_SIZE * 8;
	int destBufferFreeBits = destBufferNumBits;

	// Get source file extension, that will be compressed along with its content
	std::string srcFileExt;
	try {
		srcFileExt = getExtension(srcFilePath) + ' ';
	} catch (std::exception& e) {
		throw;
	}

	// Put source file extension at the beginning of the source buffer to be compressed
	size_t srcFileExtLen = srcFileExt.size();
	for (int i = 0; i < srcFileExtLen; i++)
		srcBuffer[i] = static_cast<uint8_t>(srcFileExt[i]);

	// Read a chunk of data from the input file
	srcFile.read(reinterpret_cast<char*>(srcBuffer + srcFileExtLen), BUFFER_SIZE - srcFileExtLen);
	std::streamsize bytesRead = bytesRead = srcFile.gcount() + srcFileExtLen;

	do {
		// Iterate through each byte in the source buffer
		for (int i = 0; i < bytesRead; i++) {
			// Get the Huffman-encoded representation of the byte
			EncodedByte encodedByte = encodingDict[srcBuffer[i]];

			int bitsWritten = destBufferNumBits - destBufferFreeBits;
			int destBufferIndex = bitsWritten / 8;
			int shr = bitsWritten % 8;

			// Update the number of free bits in the destination buffer
			destBufferFreeBits -= encodedByte.numberOfBits;
			// Shift the bits of the encoded byte to the right
			shiftRight(encodedByte, shr);

			// If the destination buffer is full, write it to the compressed file
			if (destBufferFreeBits <= 0) {
				destBuffer[destBufferIndex] |= encodedByte.codeword[0];
				destFile.write(reinterpret_cast<char*>(destBuffer), destBufferIndex + 1);

				// Reset the destination buffer
				memset(destBuffer, 0, BUFFER_SIZE * sizeof(*destBuffer));
				destBufferIndex = 0;
				destBufferFreeBits = destBufferNumBits;

				// If there are no more bits in the encoded byte, go to the next iteration
				if (encodedByte.codeword.size() == 1) 
					continue;

				// Otherwise, remove the first byte from the encoded byte (already written) and 
				// process its remaining bits
				encodedByte.codeword.erase(encodedByte.codeword.begin());
				encodedByte.numberOfBits -= 8;
				destBufferFreeBits -= encodedByte.numberOfBits;
			}
			// Write each bit of the encoded byte to the destination buffer
			for (auto iter = encodedByte.codeword.begin(); iter != encodedByte.codeword.end(); ++iter) {
				destBuffer[destBufferIndex] |= *iter;
				destBufferIndex++;
			}
		}
		// Read another chunk of data from the input file
		srcFile.read(reinterpret_cast<char*>(srcBuffer), BUFFER_SIZE);
		bytesRead = srcFile.gcount();
	} while (bytesRead > 0);

	// If there are remaining bits in the destination buffer, write them to the compressed file
	if (destBufferFreeBits < destBufferNumBits) {
		int remainingBits = destBufferNumBits - destBufferFreeBits;
		int remainingBytes = (remainingBits + 7) / 8;
		destFile.write(reinterpret_cast<char*>(destBuffer), remainingBytes);
	}
	std::cout << "File compressed successfully" << std::endl;
}

/**
 * @brief Read Huffman coding metadata from the beginning of a compressed file.
 *
 * This function reads Huffman coding metadata from the beginning of the provided
 * compressed file stream. The metadata includes the number of distinct bytes,
 * the number of bytes required to store frequencies, and the distinct bytes along
 * with their frequencies.
 *
 * @param compressedFile An input file stream representing the compressed file.
 *
 * @return A dynamically allocated array containing the Huffman coding metadata.
 *
 * @details
 * The function reads the first two bytes from the compressed file, which represent:
 * - byte 0: An integer n representing the number of codewords (distinct bytes).
 * - byte 1: An integer f representing the number of bytes required to store frequencies.
 *
 * The size of the dynamically allocated array is determined based on the values
 * read from the file. The remaining bytes (n*(f+1)) are then read and stored in
 * the array, and it represents each distinct byte from the original file along 
 * with its frequency.
 *
 * @note The returned array is dynamically allocated and should be freed by the caller.
 * @note This function assumes that the compressed file contains valid Huffman coding metadata.
 * @note The function does not handle file stream errors or exceptions.
 */
uint8_t* readHuffmanMetadata(std::ifstream& compressedFile) {
	// Read the first two bytes from the compressed file
	uint8_t auxBuffer[2];
	compressedFile.read(reinterpret_cast<char*>(auxBuffer), 2);

	// Number of codewords (distinct bytes)
	uint8_t uniqueBytes = auxBuffer[0];
	// Number of bytes required to store frequencies
	uint8_t bytesForFreq = auxBuffer[1];
	
	// Calculate the size of the Huffman metadata array
	size_t huffmanMetadataSize = (size_t)uniqueBytes * ((size_t)bytesForFreq + 1) + 2;

	// Dynamically allocate memory for the Huffman metadata array
	uint8_t* huffmanMetadata = new uint8_t[huffmanMetadataSize];
	huffmanMetadata[0] = uniqueBytes;
	huffmanMetadata[1] = bytesForFreq;

	// Read the remaining bytes (distinct bytes and their frequencies) from the compressed file
	compressedFile.read(reinterpret_cast<char*>(huffmanMetadata + 2), huffmanMetadataSize - 2);

	// Return the dynamically allocated array containing Huffman coding metadata
	return huffmanMetadata;
}

/**
 * @brief Build Huffman tree leaves from Huffman coding metadata.
 *
 * This function constructs a vector of Huffman tree leaves based on the Huffman
 * coding metadata provided. The metadata includes the number of distinct bytes,
 * the number of bytes required to store frequencies, and the distinct bytes along
 * with their frequencies.
 *
 * @param huffmanMetadata A dynamically allocated array containing Huffman coding metadata.
 *
 * @return A vector of Huffman tree leaves constructed from the provided metadata.
 *
 * @details
 * The function interprets the Huffman coding metadata to extract information about
 * each distinct byte and its frequency. It then constructs Huffman tree leaves
 * using this information and returns them in a vector.
 *
 * @note The provided `huffmanMetadata` array is expected to be properly formatted,
 * and the function does not perform error checking on the input array.
 * @note The returned vector contains shared pointers to the allocated Huffman tree 
 * leaves and should be managed accordingly by the caller.
 */
std::vector<HuffmanTreeNodePtr> buildLeavesFromMetadata(uint8_t* huffmanMetadata) {
	// Extract information from Huffman coding metadata
	uint8_t uniqueBytes = huffmanMetadata[0];
	uint8_t bytesForFreq = huffmanMetadata[1];

	std::vector<HuffmanTreeNodePtr> leaves; // Vector to store Huffman tree leaves

	// Iterate through each distinct byte in the metadata
	for (int i = 0; i < uniqueBytes; i++) {
		// Calculate the index for the current distinct byte
		size_t idx = i * (bytesForFreq + 1) + 2;

		// Extract original byte and its frequency from metadata
		uint8_t originalByte = huffmanMetadata[idx];
		size_t frequency = static_cast<size_t>(huffmanMetadata[idx + 1]);

		// Combine bytes to form the frequency value
		for (int j = 1; j < bytesForFreq; j++) {
			frequency <<= 8;
			frequency |= static_cast<size_t>(huffmanMetadata[idx + j + 1]);
		}
		// Create a Huffman tree leaf and add it to the vector
		leaves.emplace_back(std::make_shared<HuffmanTreeNode>(HuffmanTreeNode(originalByte, frequency)));
	}
	// Return the vector of Huffman tree leaves
	return leaves;
}

/**
 * @brief Decompress the given file using Huffman coding.
 *
 * This function decompresses a file that was previously compressed using Huffman
 * coding. It reads the compressed file, extracts Huffman coding metadata, builds
 * Huffman tree, and then decodes the original file content.
 *
 * @param srcFilePath The path to the compressed file to be decompressed.
 *
 * @throws std::runtime_error if there is an issue during the decompression process.
 *
 * @details
 * The function starts by opening the compressed file and creating an RAII handle for input.
 * Huffman coding metadata is read from the compressed file, and Huffman tree leaves
 * are constructed. The Huffman tree is then built using these leaves.
 * The function proceeds to read the encoded bytes from the compressed file, traverse
 * the Huffman tree, and decode the original file content. The decompressed file is
 * created, and the content is written to it.
 * The decompressed file is created in the same directory and with the same name
 * as the compressed file, but with the original file extension (that was compressed along
 * with its data).
 *
 * @note This function assumes that the compressed file contains valid Huffman coding metadata.
 * @note The function does not handle file stream errors or exceptions.
 */
void unzip(const std::string& srcFilePath) {
	// Create RAII handle for input file
	InputFileRAII scopedInputFile(srcFilePath);
	std::ifstream& srcFile = scopedInputFile.get();

	// Set decompressed file path
	std::string destPath = removeExtension(srcFilePath) + '.';

	// Get Huffman coding metadata from the compressed file
	uint8_t* huffmanMetadata = readHuffmanMetadata(srcFile);
	
	// Make Huffman tree leaves from the Huffman metadata
	std::vector<HuffmanTreeNodePtr> leaves = buildLeavesFromMetadata(huffmanMetadata);

	delete[] huffmanMetadata; // Free allocated memory for Huffman metadata

	// Build Huffman Tree from its leaf nodes
	HuffmanTree huffmanTree;
	try {
		huffmanTree = HuffmanTree(leaves);
	} catch (std::exception& e) {
		// Rethrow exception raised while building Huffman Tree to be handled by the main
		throw;
	}

	// Get Huffman Tree root
	HuffmanTreeNodePtr nodePtr = huffmanTree.getRoot();
	// Get number of encoded bytes in the zipped file to be decoded
	size_t bytesToDecode = huffmanTree.getNumberOfBytes();

	uint8_t srcBuffer[BUFFER_SIZE]; // Buffer to read from input file
	size_t currentByte = 0; // Current byte of the source buffer being processed

	// Read first chunk of data (which contains the original fle extension at the beginning) 
	srcFile.read(reinterpret_cast<char*>(srcBuffer), BUFFER_SIZE);
	std::streamsize bytesRead = srcFile.gcount();

	// Process compressed file bytes
	while (bytesRead > 0) {
		uint8_t srcByte = srcBuffer[currentByte];
		uint8_t mask = 0x80;

		// Loop through each bit in the current byte
		do {
			// Traverse Huffman Tree based on the bit value
			if ((srcByte & mask) == 0) 
				nodePtr = nodePtr->getLeft();
			else 
				nodePtr = nodePtr->getRight();

			// Check if the current node is a leaf (contains an original byte)
			if (nodePtr->isLeaf()) {
				// Get original byte from leaf node
				uint8_t decodedByte = nodePtr->getOriginalByte();
				bytesToDecode--;

				// Make node pointer points back to the tree root
				nodePtr = huffmanTree.getRoot();

				// Handle space character signaling the end of the extension
				if (decodedByte == ' ') {
					// Create RAII handle for output file and start restoring the original file content
					OutputFileRAII scopedOutputFile(destPath);
					std::ofstream& destHandle = scopedOutputFile.get();

					// Check if end of file was reached (original file was empty)
					if (bytesToDecode == 0) {
						std::cout << "File decompressed successfully" << std::endl;
						return;
					}
					mask >>= 1; // Continue reading from the current byte

					// Continue reading and decoding the remaining bits
					while (true) {
						// Loop through each bit in the current byte
						while (mask != 0) {
							// Traverse Huffman Tree based on the bit value
							if ((srcByte & mask) == 0) 
								nodePtr = nodePtr->getLeft();
							else 
								nodePtr = nodePtr->getRight();
							
							// Check if the current node is a leaf (contains an original byte)
							if (nodePtr->isLeaf()) {
								// Get original byte from leaf node
								decodedByte = nodePtr->getOriginalByte();
								bytesToDecode--;

								// Write byte in the decompressed file
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
						// Check if current chunk of data was totally read
						if (currentByte == bytesRead) {
							// Read another chunk and continue decompression
							currentByte = 0;
							srcFile.read(reinterpret_cast<char*>(srcBuffer), BUFFER_SIZE);
							bytesRead = srcFile.gcount();

							// If no bytes were read here, some error must have occurred
							if (bytesRead == 0) 
								throw std::runtime_error("Error: File could not be decompressed correctly");
						}
						srcByte = srcBuffer[currentByte];
					}
				}
				else {
					// Append the decoded character to the destination path
					destPath += static_cast<char>(decodedByte);
				}
			}
			mask >>= 1;
		} while (mask != 0);
		currentByte++;
	}
	throw std::runtime_error("Error: File could not be decompressed correctly");
}