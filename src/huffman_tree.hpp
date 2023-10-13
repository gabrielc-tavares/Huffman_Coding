#pragma once

#include <vector>
#include <array>
#include <unordered_map>
#include <utility>
#include <cstdint>
#include <optional>
#include <memory>
#include <boost/multiprecision/cpp_int.hpp>
#include "raii_handle.hpp"
#include "pathmgr.h"

constexpr int ALPHABET_SIZE = UCHAR_MAX + 1;

using uint256_t = boost::multiprecision::uint256_t;

// Using (bitstream, numberOfBits) pair to store encoded characters
using EncodedChar_t = std::pair<uint256_t, uint16_t>;

// Map each encoded character to its original form
using EncodedCharMap_t = std::unordered_map<uint8_t, EncodedChar_t>;

class HuffmanTreeNode {
public:
	using NodePtr_t = std::shared_ptr<HuffmanTreeNode>;

	HuffmanTreeNode(size_t _frequency = 0) : frequency(_frequency) {}

	HuffmanTreeNode(uint8_t _originalChar, size_t _frequency = 0) : character(_originalChar), frequency(_frequency) {}

	HuffmanTreeNode(NodePtr_t _left, NodePtr_t _right) : left(_left), right(_right) {
		if (!left || !right) {
			throw std::runtime_error("Huffman Tree internal node must have exactly 2 child nodes");
		}
		frequency = left->frequency + right->frequency;
	}

	// Get node character frequency
	size_t getFrequency() const {
		return frequency;
	}

	// Set node character frequency
	void setFrequency(size_t _frequency) {
		frequency = _frequency;
	}

	// Get node character
	uint8_t getCharacter() const {
		if (!character.has_value()) {
			throw std::runtime_error("'originalByte' was not initialized");
		}
		return character.value();
	}

	// Set node character
	void setOriginalChar(uint8_t _character) {
		character = _character;
	}

	// Get left subtree (or nullptr if node is leaf)
	NodePtr_t getLeft() const {
		return left;
	}

	// Get right subtree (or nullptr if node is leaf)
	NodePtr_t getRight() const {
		return right;
	}

	// Set node subtrees (if internal)
	void setSubTrees(NodePtr_t _left, NodePtr_t _right) {
		if (!_left || !_right) {
			throw std::runtime_error("Huffman Tree internal node must have exactly 2 child nodes");
		}
		left = _left;
		right = _right;
		frequency = left->frequency + right->frequency;
	}

	// Check if node is leaf
	bool isLeaf() const {
		return !left && !right;
	}

private:
	std::optional<uint8_t> character;
	size_t frequency;
	NodePtr_t left, right;
};

class HuffmanTree {
public:
	using NodePtr_t = std::shared_ptr<HuffmanTreeNode>;

	HuffmanTree(const std::string& fileName) {
		// Create RAII file handle
		InputFileRAII scopedFile(fileName);
		std::ifstream& handle = scopedFile.get();

		// Check if the file is empty
		if (handle.peek() == std::ifstream::traits_type::eof()) {
			throw std::runtime_error("Empty input file");
		}
		// Temporary array to store frequencies
		std::array<size_t, ALPHABET_SIZE> freqArr{ 0 };

		// Get original file extension
		std::string ext = extension(fileName);
		// Append a space at the end of the extension, which will serve as a 
		// separator between it and the actual file data in the compressed file
		ext.push_back(' ');

		// Count frequencies of the characters in the original file extension
		for (int i = 0; i < ext.size(); i++)
			freqArr[ext[i]]++;

		// Buffer to read from input file
		uint8_t buf[BUFFER_SIZE];

		// Read a chunk of data from the input file into the buffer
		handle.read(reinterpret_cast<char*>(buf), BUFFER_SIZE);
		// Get the number of bytes read in this chunk
		std::streamsize bytesRead = handle.gcount();

		while (bytesRead == BUFFER_SIZE) {
			// Count frequencies of the characters in this chunk
			for (int i = 0; i < BUFFER_SIZE; i++)
				freqArr[buf[i]]++;

			// Read a chunk of data from the input file into the buffer
			handle.read(reinterpret_cast<char*>(buf), BUFFER_SIZE);
			// Get the number of bytes read in this chunk
			bytesRead = handle.gcount();
		}
		
		// If the number of bytes in a chunk is < BUFFER_SIZE and > 0
		if (bytesRead > 0) {
			// Count frequencies of the characters in this chunk
			for (int i = 0; i < bytesRead; i++)
				freqArr[buf[i]]++;
		}

		// Build leaf nodes
		for (int i = 0; i < ALPHABET_SIZE; i++) {
			if (freqArr[i]) {
				// Map new empty encoded char with 'i'
				EncodedChar_t encoded(0, 0);
				encodedCharMap.insert({ i, encoded });

				// Insert leaf node ordered by frequency
				std::vector<NodePtr_t>::iterator iter = leaves.begin();

				while (iter < leaves.end() && (*iter)->getFrequency() > freqArr[i])
					iter++;

				NodePtr_t newNodePtr = std::make_shared<HuffmanTreeNode>(HuffmanTreeNode((uint8_t)i, freqArr[i]));
				leaves.emplace(iter, newNodePtr);
			}
		}

		// Build Huffman Tree
		buildMinHeap();
	}

	HuffmanTree(std::vector<NodePtr_t>& _leaves) {
		leaves = _leaves;

		// Initialize bitsets
		for (NodePtr_t leaf : leaves) {
			EncodedChar_t bitset(0, 0);
			encodedCharMap.insert({ leaf->getCharacter(), bitset });
		}

		// Build tree
		buildMinHeap();
	}

	// Get Huffman Tree root
	NodePtr_t getRoot() const {
		return root;
	}

	// Get map with each encoded characters indexed by its original form
	EncodedCharMap_t getEncodedCharMap() const {
		return encodedCharMap;
	}

	// Get total number of bytes in the original file (+ its extension and a ' ')
	size_t getNumberOfBytes() const {
		return root->getFrequency();
	}

	// Get frequency of the most frequent character in the original file
	size_t getBiggerFrequency() const {
		return leaves.front()->getFrequency();
	}

	// Get leaf nodes of the Huffman Tree
	std::vector<NodePtr_t> getLeaves() const {
		return leaves;
	}

private:
	NodePtr_t root;
	std::vector<NodePtr_t> leaves;
	EncodedCharMap_t encodedCharMap;

	void buildBitsets(EncodedChar_t bitset, NodePtr_t nodePtr) {
		if (nodePtr->isLeaf()) {
			// Leaf node: Assign bitset mapping
			encodedCharMap[nodePtr->getCharacter()] = bitset;
		}
		else {
			// Internal node: Traverse left and right subtrees
			bitset.second++;

			bitset.first <<= 1;
			buildBitsets(bitset, nodePtr->getLeft());

			bitset.first |= 1;
			buildBitsets(bitset, nodePtr->getRight());
		}
	}

	void setEncoding() {
		// Make all bitsets prefixed by '0'
		EncodedChar_t prefix0(0, 1);
		buildBitsets(prefix0, root->getLeft());

		// Make all bitsets prefixed by '1'
		EncodedChar_t prefix1(1, 1);
		buildBitsets(prefix1, root->getRight());
	}

	void buildMinHeap() {
		// Temporary vector with leaves to build min-heap
		std::vector<NodePtr_t> leavesTemp(leaves);

		while (leavesTemp.size() > 1) {
			NodePtr_t right = leavesTemp.back();
			leavesTemp.pop_back();
			NodePtr_t left = leavesTemp.back();
			leavesTemp.pop_back();

			NodePtr_t newInternal = std::make_shared<HuffmanTreeNode>(HuffmanTreeNode(right, left));

			std::vector<NodePtr_t>::iterator iter = leavesTemp.begin();

			while (iter < leavesTemp.end() && (*iter)->getFrequency() > newInternal->getFrequency())
				iter++;

			leavesTemp.insert(iter, newInternal);
		}
		root = leavesTemp.front();

		// Map chars to its encoded bitsets
		setEncoding();
	}
};