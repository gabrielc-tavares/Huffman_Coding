# Huffman_Coding
Simple file compression tool using Huffman Coding.

# How to use
To use this tool, call it from the terminal passing the file to compress/decompress as argument.
If executed with no arguments, it will prompt the user to enter the path to the file they want to compress/decompress.
The compressed/decompressed file will be created in the same directory and with the same name as the input file.
The compressed files created by this tool will have a ".hzip" extension and the program uses the file extension to 
determine whether it should be compressed or decompressed, compressing it if it has a ".hzip" extension or decompressing 
it otherwise. Therefore, the input file extension must always be present.
Do not modify the content or extension of the ".hzip" file created by this program, as this may lead to errors in the 
decompression process.
