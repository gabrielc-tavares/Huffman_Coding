#pragma once

#include <vector>
#include <array>
#include <unordered_map>
#include <optional>
#include <memory>
#include "raii_handle.hpp"
#include "pathmgr.h"

constexpr int ALPHABET_SIZE = 256;

using Codeword = std::vector<uint8_t>;

// Represents the encoded version of some byte
class EncodedByte {
public:
	EncodedByte()
		: numberOfBits(0) {
		codeword.emplace_back(0);
	}

	EncodedByte(const Codeword& _codeword, int _numberOfBits)
		: codeword(_codeword), numberOfBits(_numberOfBits) {}

	void operator=(const EncodedByte& obj) {
		copyCodeword(obj.codeword);
		numberOfBits = obj.numberOfBits;
	}

	Codeword codeword;
	int numberOfBits;

private:
	void copyCodeword(Codeword _codeword) {
		codeword.swap(_codeword);
	}
};

class HuffmanTreeNode;
using HuffmanTreeNodePtr = std::shared_ptr<HuffmanTreeNode>;

class HuffmanTreeNode {
public:
	HuffmanTreeNode(size_t _frequency = 0)
		: frequency(_frequency) {}

	// Leaf node constructor
	HuffmanTreeNode(uint8_t _originalByte, size_t _frequency = 0)
		: originalByte(_originalByte), frequency(_frequency) {}

	// Internal node constructor
	HuffmanTreeNode(HuffmanTreeNodePtr _left, HuffmanTreeNodePtr _right)
		: left(_left), right(_right) {
		if (!left || !right) {
			throw std::invalid_argument("Trying to assign nullptr to a child node of an internal Huffman Tree node");
		}
		frequency = left->frequency + right->frequency;
	}

	size_t getFrequency() const { return frequency; }
	size_t setFrequency(size_t _frequency) { frequency = _frequency; }

	uint8_t getOriginalByte() const {
		if (!originalByte.has_value()) {
			throw std::logic_error("Trying to get unsettled originalByte from a Huffman Tree node");
		}
		return originalByte.value();
	}
	void setOriginalByte(uint8_t _originalByte) {
		if (left || right) {
			throw std::logic_error("Trying to set originalByte attribute for an internal Huffman Tree node");
		}
		originalByte = _originalByte;
	}

	HuffmanTreeNodePtr getLeft() const { return left; }
	HuffmanTreeNodePtr getRight() const { return right; }
	void setSubtrees(HuffmanTreeNodePtr _left, HuffmanTreeNodePtr _right) {
		if (originalByte.has_value()) {
			throw std::logic_error("Trying to set child nodes for a leaf Huffman Tree node");
		}
		if (!left || !right) {
			throw std::invalid_argument("Trying to assign nullptr to a child node of an internal Huffman Tree node");
		}
		left = _left;
		right = _right;
		frequency = left->frequency + right->frequency;
	}

	bool isLeaf() const { return originalByte.has_value(); }

private:
	std::optional<uint8_t> originalByte;
	size_t frequency;
	HuffmanTreeNodePtr left, right;
};

// A mapping between the original bytes and the Huffman Coding codewords
using PrefixFreeBinCode = std::unordered_map<uint8_t, EncodedByte>;

class HuffmanTree {
public:
	HuffmanTree() {}
	
	// Build Huffman Tree from a source file
	HuffmanTree(const std::string& srcFilePath) {
		// Create RAII file handle
		InputFileRAII scopedSrcFile(srcFilePath);
		std::ifstream& srcFile = scopedSrcFile.get();

		// The extension of the source file will be compressed along with its content
		char separator = ' ';
		std::string srcFileExt = extension(srcFilePath) + separator;

		// Temporary array to store frequencies
		std::array<size_t, ALPHABET_SIZE> freqArr{ 0 };

		// Count frequencies of the characters in the file extension
		for (char c : srcFileExt)
			freqArr[c]++;

		uint8_t srcBuffer[BUFFER_SIZE]; // Buffer to read chunks of data from the source file
		std::streamsize bytesRead; // Number of bytes read in a chunk

		while (true) {
			srcFile.read(reinterpret_cast<char*>(srcBuffer), BUFFER_SIZE);
			// Break loop if there is no more data to read
			if ((bytesRead = srcFile.gcount()) == 0) break;

			// Count frequencies of the bytes in the chunk
			for (int i = 0; i < bytesRead; i++)
				freqArr[srcBuffer[i]]++;
		}

		// Build Huffman Tree leaves, inserting them in a vector ordered by frequency
		for (int i = 0; i < ALPHABET_SIZE; i++) {
			if (freqArr[i] == 0) continue;

			std::vector<HuffmanTreeNodePtr>::iterator iter = leaves.begin();
			while (iter < leaves.end() && (*iter)->getFrequency() > freqArr[i]) iter++;
			leaves.insert(iter, std::make_shared<HuffmanTreeNode>(HuffmanTreeNode(static_cast<uint8_t>(i), freqArr[i])));
		}
		buildMinHeapFromLeaves(); // Build Huffman Tree
		buildEncodingDict(); // Set encoding and map byte values to its compressed codewords
	}

	// Build Huffman Tree from its leaves
	HuffmanTree(std::vector<HuffmanTreeNodePtr>& _leaves)
		: leaves(_leaves) {
		buildMinHeapFromLeaves();
		buildEncodingDict();
	}

	HuffmanTreeNodePtr getRoot() const { return root; }

	// Get map indexing the Huffman Coding codewords with its original bytes
	PrefixFreeBinCode getEncodingDict() const { return encodingDict; }

	// Get total number of bytes that were encoded (source file content + source file extension + separator character)
	size_t getNumberOfBytes() const { return root->getFrequency(); }

	// Get frequency of the most frequent character
	size_t getHigherFrequency() const { return leaves.front()->getFrequency(); }

	// Get all leaf nodes of the Huffman Tree in a vector
	std::vector<HuffmanTreeNodePtr> getLeaves() const { return leaves; }

private:
	HuffmanTreeNodePtr root;
	std::vector<HuffmanTreeNodePtr> leaves;
	PrefixFreeBinCode encodingDict; // Mapping between the original bytes and the Huffman Coding codewords

	void setEncodings(EncodedByte encodedByte, HuffmanTreeNodePtr currentNode, uint8_t position) {
		if (currentNode->isLeaf()) {
			// Leaf node: Assign encoding mapping
			encodingDict.insert({ currentNode->getOriginalByte(), encodedByte });
		} else {
			// Internal node: Traverse left and right subtrees
			if (position == 0) {
				position = 0x80;
				encodedByte.codeword.emplace_back(0);
			}
			encodedByte.numberOfBits++;
			setEncodings(encodedByte, currentNode->getLeft(), position / 2);
			encodedByte.codeword[encodedByte.codeword.size() - 1] |= position;
			setEncodings(encodedByte, currentNode->getRight(), position / 2);
		}
	}

	void buildEncodingDict() {
		uint8_t initialPosition = 0x40; // 01000000 (base 2)

		// Make all encodings prefixed by '0'
		Codeword initialCodeword;
		initialCodeword.emplace_back(0);
		setEncodings(EncodedByte(initialCodeword, 1), root->getLeft(), initialPosition);

		// Make all encodings prefixed by '1'
		initialCodeword[0] |= 0x80;
		setEncodings(EncodedByte(initialCodeword, 1), root->getRight(), initialPosition);
	}

	void buildMinHeapFromLeaves() {
		std::vector<HuffmanTreeNodePtr> tempLeaves(leaves);

		while (tempLeaves.size() > 1) {
			HuffmanTreeNodePtr left = tempLeaves.back();
			tempLeaves.pop_back();
			HuffmanTreeNodePtr right = tempLeaves.back();
			tempLeaves.pop_back();

			HuffmanTreeNodePtr newInternalNode = std::make_shared<HuffmanTreeNode>(HuffmanTreeNode(left, right));

			std::vector<HuffmanTreeNodePtr>::iterator iter = tempLeaves.begin();
			while (iter < tempLeaves.end() && (*iter)->getFrequency() > newInternalNode->getFrequency()) iter++;
			tempLeaves.insert(iter, newInternalNode);
		}
		root = tempLeaves.front();
	}
};