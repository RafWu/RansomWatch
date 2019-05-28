#include "HashUtility.h"

std::wstring HashFileSHA1(const std::wstring & filename)
{
	// Create the algorithm provider for SHA-1 hashing 
	CryptAlgorithmProvider sha1;

	// Create the hash object for the particular hashing 
	CryptHashObject hasher(sha1);

	// Object to read data from file 
	FileReader file(filename);

	// Read buffer 
	std::vector<BYTE> buffer(4 * 1024);   // 4 KB buffer 

	// Reading loop 
	while (!file.EoF())
	{
		// Read a chunk of data from file to memory buffer 
		int readBytes = file.Read(buffer.data(), buffer.size());

		// Hash this chunk of data 
		hasher.HashData(buffer.data(), readBytes);
	}

	// Finalize hashing 
	hasher.FinishHash();

	// Return hash digest 
	return hasher.HashDigest();
}