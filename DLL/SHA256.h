#pragma once

class SHA256 {
	BCRYPT_ALG_HANDLE algorithmHandle;
	BCRYPT_HASH_HANDLE hashHandle;

public:
	SHA256();
	~SHA256() {
		BCryptDestroyHash(hashHandle);
		BCryptCloseAlgorithmProvider(algorithmHandle,0);
	}

	void CalculateHash(const char *input,std::array<unsigned char,32> &output);
};