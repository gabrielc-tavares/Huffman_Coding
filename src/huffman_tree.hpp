#pragma once

#include <vector>
#include <array>
#include <unordered_map>
#include <optional>
#include <memory>
#include "raii_handle.hpp"
#include "pathmgr.h"

constexpr int ALPHABET_SIZE = 256; ///< Number of values that can be represented using 8 bits

/**
 * @typedef Codeword
 * @brief Represents the sequence of bits corresponding to the encoded version of some character.
 */
using Codeword = std::vector<uint8_t>;

/**
 * @class EncodedByte
 * @brief Represents the encoded version of a byte using Huffman coding.
 *
 * This class holds the information about the encoded representation of a byte,
 * including the codeword and the number of bits used in the codeword.
 *
 * @note The default constructor initializes the EncodedByte with a zero codeword
 * and zero numberOfBits.
 */
class EncodedByte {
public:
	/**
	 * @brief Default constructor for EncodedByte.
	 *
	 * Initializes EncodedByte with an empty codeword containing a single zero,
	 * and sets numberOfBits to zero.
	 */
	EncodedByte()
		: numberOfBits(0) {
		codeword.emplace_back(0);
	}

	/**
	 * @brief Parameterized constructor for EncodedByte.
	 *
	 * Initializes EncodedByte with the provided codeword and numberOfBits.
	 *
	 * @param _codeword The codeword representing the encoded byte.
	 * @param _numberOfBits The number of bits used in the codeword.
	 */
	EncodedByte(const Codeword& _codeword, int _numberOfBits)
		: codeword(_codeword), numberOfBits(_numberOfBits) {}

	/**
	 * @brief Assignment operator for EncodedByte.
	 *
	 * Copies the codeword and numberOfBits from the provided EncodedByte object.
	 *
	 * @param obj The EncodedByte object to copy from.
	 */
	void operator=(const EncodedByte& obj) {
		copyCodeword(obj.codeword);
		numberOfBits = obj.numberOfBits;
	}

	Codeword codeword; ///< The codeword representing the encoded byte.
	int numberOfBits; ///< The number of bits used in the codeword.

private:
	/**
	 * @brief Copies the codeword from the provided vector to the current codeword.
	 *
	 * @param _codeword The codeword to copy.
	 */
	void copyCodeword(Codeword _codeword) {
		codeword.swap(_codeword);
	}
};

// Previous declaration of HuffmanTreeNode for the HuffmanTreeNodePtr type alias
class HuffmanTreeNode;

/**
 * @typedef HuffmanTreeNodePtr
 * @brief Shared pointer to a HuffmanTreeNode.
 */
using HuffmanTreeNodePtr = std::shared_ptr<HuffmanTreeNode>;

/**
 * @class HuffmanTreeNode
 * @brief Represents a node in a Huffman Tree.
 *
 * This class models nodes in a Huffman Tree, which is used for Huffman coding.
 * Each node can be an internal node with left and right subtrees or a leaf node
 * containing a certain byte along with its frequency.
 */
class HuffmanTreeNode {
public:
	/**
	 * @brief Default constructor for internal nodes.
	 *
	 * @param _frequency The frequency of the node.
	 */
	HuffmanTreeNode(size_t _frequency = 0)
		: frequency(_frequency) {}

	/**
	 * @brief Constructor for leaf nodes.
	 *
	 * @param _originalByte The original byte in the leaf node.
	 * @param _frequency The frequency of the node.
	 */
	HuffmanTreeNode(uint8_t _originalByte, size_t _frequency = 0)
		: originalByte(_originalByte), frequency(_frequency) {}

	/**
	 * @brief Constructor for internal nodes.
	 *
	 * @param _left The left subtree.
	 * @param _right The right subtree.
	 * @throws std::invalid_argument if _left or _right is nullptr.
	 */
	HuffmanTreeNode(HuffmanTreeNodePtr _left, HuffmanTreeNodePtr _right)
		: left(_left), right(_right) {
		if (!left || !right) 
			throw std::invalid_argument("Trying to assign nullptr to a child node of an internal Huffman Tree node");
		
		frequency = left->frequency + right->frequency;
	}

	/**
	 * @brief Get the frequency of the node.
	 *
	 * @return The frequency of the node.
	 */
	size_t getFrequency() const { 
		return frequency; 
	}

	/**
	 * @brief Set the frequency of the node.
	 *
	 * @param _frequency The new frequency to be set.
	 */
	size_t setFrequency(size_t _frequency) { 
		frequency = _frequency; 
	}

	/**
	 * @brief Get the original byte of a leaf node.
	 *
	 * @return The original byte.
	 * @throws std::logic_error if called on a non-leaf node.
	 */
	uint8_t getOriginalByte() const {
		if (!originalByte.has_value()) 
			throw std::logic_error("Trying to get unsettled originalByte from a Huffman Tree node");
		
		return originalByte.value();
	}

	/**
	 * @brief Set the original byte for a leaf node.
	 *
	 * @param _originalByte The original byte to be set.
	 * @throws std::logic_error if called on a non-leaf node.
	 */
	void setOriginalByte(uint8_t _originalByte) {
		if (left || right) 
			throw std::logic_error("Trying to set originalByte attribute for an internal Huffman Tree node");
		
		originalByte = _originalByte;
	}

	/**
	 * @brief Get the left subtree.
	 *
	 * @return A shared pointer to the left subtree.
	 */
	HuffmanTreeNodePtr getLeftSubtree() const {
		return left; 
	}

	/**
	 * @brief Get the right subtree.
	 *
	 * @return A shared pointer to the right subtree.
	 */
	HuffmanTreeNodePtr getRightSubtree() const { 
		return right; 
	}

	/**
	 * @brief Set the left and right subtrees for an internal node.
	 *
	 * @param _left The new left subtree.
	 * @param _right The new right subtree.
	 * @throws std::logic_error if called on a leaf node.
	 * @throws std::invalid_argument if _left or _right is nullptr.
	 */
	void setSubtrees(HuffmanTreeNodePtr _left, HuffmanTreeNodePtr _right) {
		if (originalByte.has_value()) 
			throw std::logic_error("Trying to set child nodes for a leaf Huffman Tree node");
		
		if (!left || !right) 
			throw std::invalid_argument("Trying to assign nullptr to a child node of an internal Huffman Tree node");
		
		left = _left;
		right = _right;
		frequency = left->frequency + right->frequency;
	}

	/**
	 * @brief Check if the node is a leaf node.
	 *
	 * @return true if the node is a leaf, false otherwise.
	 */
	bool isLeaf() const { return originalByte.has_value(); }

private:
	std::optional<uint8_t> originalByte; ///< The original byte for leaf nodes.
	size_t frequency; ///< The frequency of the node.
	HuffmanTreeNodePtr left, right; ///< Pointers to left and right subtrees.
};

/**
 * @typedef PrefixFreeBinCode
 * @brief Represents a mapping between the original bytes and the Huffman Coding codewords.
 */
using PrefixFreeBinCode = std::unordered_map<uint8_t, EncodedByte>;

/**
 * @class HuffmanTree
 * @brief Represents a Huffman Tree used for encoding and decoding data.
 *
 * This class provides functionality to build a Huffman Tree from a source file,
 * either directly or from a set of pre-existing leaves. It also exposes methods
 * to retrieve the root of the tree, the encoding dictionary, the total number of
 * bytes encoded, the frequency of the most frequent character, and a vector of
 * leaf nodes in the tree.
 */
class HuffmanTree {
public:
	/**
	 * @brief Default constructor for HuffmanTree.
	 */
	HuffmanTree() {}
	
	/**
	 * @brief Build Huffman Tree from a source file.
	 * @param srcFilePath The path to the source file.
	 */
	HuffmanTree(const std::string& srcFilePath) {
		// Create RAII file handle
		InputFileRAII scopedSrcFile(srcFilePath);
		std::ifstream& srcFile = scopedSrcFile.get();

		// The extension of the source file will be compressed along with its content
		char separator = ' ';
		std::string srcFileExt = getExtension(srcFilePath) + separator;

		// Temporary array to store frequencies
		std::array<size_t, ALPHABET_SIZE> freqArr{ 0 };

		// Count frequencies of the characters in the file extension
		for (char c : srcFileExt)
			freqArr[c]++;

		uint8_t srcBuffer[BUFFER_SIZE]; // Buffer to read chunks of data from the source file
		std::streamsize bytesRead; // Number of bytes read in a chunk

		srcFile.read(reinterpret_cast<char*>(srcBuffer), BUFFER_SIZE);

		while ((bytesRead = srcFile.gcount()) != 0) {
			// Count frequencies of the bytes in the chunk
			for (int i = 0; i < bytesRead; i++)
				freqArr[srcBuffer[i]]++;

			srcFile.read(reinterpret_cast<char*>(srcBuffer), BUFFER_SIZE);
		}

		// Build Huffman Tree leaves, inserting them in a vector ordered by frequency
		for (int i = 0; i < ALPHABET_SIZE; i++) {
			if (freqArr[i] == 0) 
				continue;

			std::vector<HuffmanTreeNodePtr>::iterator iter = leaves.begin();

			while (iter < leaves.end() && (*iter)->getFrequency() > freqArr[i]) 
				iter++;

			leaves.insert(iter, std::make_shared<HuffmanTreeNode>(HuffmanTreeNode(static_cast<uint8_t>(i), freqArr[i])));
		}
		buildMinHeapFromLeaves(); // Build Huffman Tree
		buildEncodingDict(); // Set encoding and map byte values to its compressed codewords
	}

	/**
	 * @brief Build Huffman Tree from its leaves.
	 * @param _leaves The vector of leaf nodes.
	 */
	HuffmanTree(std::vector<HuffmanTreeNodePtr>& _leaves)
		: leaves(_leaves) {
		buildMinHeapFromLeaves();
		buildEncodingDict();
	}

	/**
	 * @brief Get the root of the Huffman Tree.
	 * @return A shared pointer to the root node.
	 */
	HuffmanTreeNodePtr getRoot() const { 
		return root; 
	}

	/**
	 * @brief Get the encoding dictionary.
	 * @return PrefixFreeBinCode representing the encoding mapping.
	 */
	PrefixFreeBinCode getEncodingDict() const { 
		return encodingDict; 
	}

	/**
	 * @brief Get the total number of bytes encoded.
	 * @return The total number of bytes encoded.
	 */
	size_t getNumberOfBytes() const { 
		return root->getFrequency(); 
	}

	/**
	 * @brief Get the frequency of the most frequent character.
	 * @return The frequency of the most frequent character.
	 */
	size_t getHigherFrequency() const { 
		return leaves.front()->getFrequency(); 
	}

	/**
	 * @brief Get all leaf nodes of the Huffman Tree.
	 * @return A vector of shared pointers to leaf nodes.
	 */
	std::vector<HuffmanTreeNodePtr> getLeaves() const { 
		return leaves; 
	}

private:
	HuffmanTreeNodePtr root; ///< The root of the Huffman Tree.
	std::vector<HuffmanTreeNodePtr> leaves; ///< Vector of leaf nodes.
	PrefixFreeBinCode encodingDict; ///< Encoding dictionary for Huffman Coding.

	/**
	 * @brief Set the Huffman Coding encodings.
	 * @param encodedByte The encoding information for a byte.
	 * @param currentNode The current node in the traversal.
	 * @param position The position in the codeword.
	 */
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
			setEncodings(encodedByte, currentNode->getLeftSubtree(), position / 2);
			encodedByte.codeword[encodedByte.codeword.size() - 1] |= position;
			setEncodings(encodedByte, currentNode->getRightSubtree(), position / 2);
		}
	}

	/**
	 * @brief Build the encoding dictionary.
	 */
	void buildEncodingDict() {
		uint8_t initialPosition = 0x40; // 01000000 (binary)

		// Make all encodings prefixed by '0'
		Codeword initialCodeword;
		initialCodeword.emplace_back(0);
		setEncodings(EncodedByte(initialCodeword, 1), root->getLeftSubtree(), initialPosition);

		// Make all encodings prefixed by '1'
		initialCodeword[0] |= 0x80;
		setEncodings(EncodedByte(initialCodeword, 1), root->getRightSubtree(), initialPosition);
	}

	/**
	 * @brief Build a min-heap from the leaves.
	 */
	void buildMinHeapFromLeaves() {
		std::vector<HuffmanTreeNodePtr> tempLeaves(leaves);

		while (tempLeaves.size() > 1) {
			HuffmanTreeNodePtr left = tempLeaves.back();
			tempLeaves.pop_back();
			HuffmanTreeNodePtr right = tempLeaves.back();
			tempLeaves.pop_back();

			HuffmanTreeNodePtr newInternalNode = std::make_shared<HuffmanTreeNode>(HuffmanTreeNode(left, right));

			std::vector<HuffmanTreeNodePtr>::iterator iter = tempLeaves.begin();

			while (iter < tempLeaves.end() && (*iter)->getFrequency() > newInternalNode->getFrequency()) 
				iter++;

			tempLeaves.insert(iter, newInternalNode);
		}
		root = tempLeaves.front();
	}
};