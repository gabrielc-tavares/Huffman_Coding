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

constexpr int ALPHABET_SIZE = 256;

using boost::multiprecision::uint256_t;

// Represents the encoding of a byte value
class HuffmanEncoded {
public:
	HuffmanEncoded(const uint256_t& _encodedByte = 0, uint16_t _bitCount = 0)
		: encodedByte(_encodedByte), bitCount(_bitCount) {}

	void operator=(const HuffmanEncoded& obj) {
		encodedByte = obj.encodedByte;
		bitCount = obj.bitCount;
	}

	uint256_t encodedByte;
	uint16_t bitCount;
};

class Node {
public:
	Node(size_t _frequency = 0) 
		: frequency(_frequency) {}

	Node(uint8_t _originalByte, size_t _frequency = 0) 
		: originalByte(_originalByte), frequency(_frequency) {}

	Node(std::shared_ptr<Node> _left, std::shared_ptr<Node> _right) 
		: left(_left), right(_right) {
		if (!left || !right) 
			throw std::runtime_error("Internal node must have exactly 2 child nodes.");
		frequency = left->frequency + right->frequency;
	}

	size_t getFrequency() const {
		return frequency;
	}

	void setFrequency(size_t _frequency) {
		frequency = _frequency;
	}

	uint8_t getOriginalByte() const {
		if (!originalByte.has_value()) 
			throw std::runtime_error("Trying to use 'getOriginalByte' method in a node with unsettled 'originalByte' atribute.");
		return originalByte.value();
	}

	void setOriginalByte(uint8_t _originalByte) {
		if (left || right)
			throw std::runtime_error("Trying to use 'setOriginalByte' method in an internal node.");
		originalByte = _originalByte;
	}

	// Get left child node
	std::shared_ptr<Node> getLeft() const {
		return left;
	}

	// Get right child node
	std::shared_ptr<Node> getRight() const {
		return right;
	}

	void setSubtrees(std::shared_ptr<Node> _left, std::shared_ptr<Node> _right) {
		if (originalByte.has_value())
			throw std::runtime_error("Trying to use 'setSubtrees' method in a leaf node.");
		if (!_left || !_right) 
			throw std::runtime_error("Internal node must have exactly 2 child nodes.");
		left = _left;
		right = _right;
		frequency = left->frequency + right->frequency;
	}

	bool isLeaf() const {
		return !left && !right && originalByte.has_value();
	}

private:
	std::optional<uint8_t> originalByte;
	size_t frequency;
	std::shared_ptr<Node> left, right;
};

using NodePtr = std::shared_ptr<Node>;
using HuffmanEncodingMap = std::unordered_map<uint8_t, HuffmanEncoded>;

class HuffmanTree {
public:
	HuffmanTree(const std::string& srcPath) {
		// Create RAII file handle
		InputFileRAII scopedFile(srcPath);
		std::ifstream& handle = scopedFile.get();

		// Temporary array to store frequencies
		std::array<size_t, ALPHABET_SIZE> freqArr{ 0 };

		// Get extension of the source file (it will be compressed along with the source 
		// file content by being placed before it and separated from it with a space).
		std::string ext = extension(srcPath) + ' ';

		// Count characters in the source file extension
		for (char c : ext) 
			freqArr[c]++;
		
		uint8_t buf[BUFFER_SIZE]; // Buffer to read from the source file
		do {
			// Read a chunk of data from the source file
			handle.read(reinterpret_cast<char*>(buf), BUFFER_SIZE);
			// Get the number of bytes read in this chunk
			std::streamsize bytesRead = handle.gcount();

			// If no bytes were read, break loop
			if (bytesRead == 0) break;

			// Count the frequencies of the bytes in this chunk
			for (int i = 0; i < bytesRead; i++) {
				freqArr[buf[i]]++;
			}
		} while (true);

		// Build tree leaves of the HuffmanTree with all bytes whose frequencies are > 0
		for (int i = 0; i < ALPHABET_SIZE; i++) {
			if (freqArr[i] == 0)
				continue;

			// Map new encoding with character 'i'
			encodingMap.insert({ static_cast<uint8_t>(i), HuffmanEncoded(0, 0) });

			// Insert new node ordered by frequency
			std::vector<std::shared_ptr<Node>>::iterator iter = leaves.begin();
			while (iter < leaves.end() && (*iter)->getFrequency() > freqArr[i]) {
				iter++;
			}
			leaves.insert(iter, std::make_shared<Node>(Node(static_cast<uint8_t>(i), freqArr[i])));
		}
		buildMinHeap();
	}

	HuffmanTree(std::vector<NodePtr>& _leaves) {
		leaves = _leaves;

		// Initialize encoding
		for (NodePtr leaf : leaves) 
			encodingMap.insert({ leaf->getOriginalByte(), HuffmanEncoded(0, 0) });

		buildMinHeap();
	}

	NodePtr getRoot() const {
		return root;
	}

	// Get map with each encoded characters indexed by its original form
	HuffmanEncodingMap getEncodingMap() const {
		return encodingMap;
	}

	// Get total number of bytes in the original file (+ its extension and a ' ')
	size_t getNumberOfBytes() const {
		return root->getFrequency();
	}

	// Get frequency of the most frequent character in the original file
	size_t getHigherFrequency() const {
		return leaves.front()->getFrequency();
	}

	// Get vector with all tree leaves
	std::vector<NodePtr> getLeaves() const {
		return leaves;
	}

private:
	NodePtr root;
	std::vector<NodePtr> leaves;
	HuffmanEncodingMap encodingMap;

	void traverseToEncode(HuffmanEncoded tempEncoding, NodePtr currentNode) {
		if (currentNode->isLeaf()) {
			// Leaf node: Assign encoding mapping
			encodingMap[currentNode->getOriginalByte()] = tempEncoding;
		}
		else {
			// Internal node: Traverse left and right subtrees
			tempEncoding.bitCount++;
			tempEncoding.encodedByte <<= 1;

			traverseToEncode(tempEncoding, currentNode->getLeft());
			tempEncoding.encodedByte |= 1;
			traverseToEncode(tempEncoding, currentNode->getRight());
		}
	}

	void setEncoding() {
		// Make all encodings prefixed by '0'
		traverseToEncode(HuffmanEncoded(0, 1), root->getLeft());
		// Make all encodings prefixed by '1'
		traverseToEncode(HuffmanEncoded(1, 1), root->getRight());
	}

	void buildMinHeap() {
		// Temporary node vector to build min-heap
		std::vector<NodePtr> temp(leaves);

		while (temp.size() > 1) {
			NodePtr left = temp.back();
			temp.pop_back();
			NodePtr right = temp.back();
			temp.pop_back();

			NodePtr newInternal = std::make_shared<Node>(Node(left, right));

			// Insert new internal node ordered in the vector
			std::vector<NodePtr>::iterator iter = temp.begin();
			while (iter < temp.end() && (*iter)->getFrequency() > newInternal->getFrequency()) {
				iter++;
			}
			temp.insert(iter, newInternal);
		}
		root = temp.front();

		// Map chars to its encoded forms
		setEncoding();
	}
};