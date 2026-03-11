#include <array>
#include <exception>
#include <ntstatus.h>
#include "SHA256.h"
#include "Logger.h"

SHA256::SHA256() {
	auto result = BCryptOpenAlgorithmProvider(&algorithmHandle,BCRYPT_SHA256_ALGORITHM,0,0);
	if (result != STATUS_SUCCESS) {
		throw std::exception("BCrypt provider selection failed.");
	}
}

SHA256::~SHA256() {
	auto result = BCryptDestroyHash(hashHandle);
	if (result != STATUS_SUCCESS) {
		Logger::Log(std::format("Destroying a BCrypt hash failed.  Result: {}",result));
	}
	result = BCryptCloseAlgorithmProvider(algorithmHandle,0);
	if (result != STATUS_SUCCESS) {
		Logger::Log(std::format("Closing the BCrypt provider failed.  Result: {}",result));
	}
}

void SHA256::CalculateHash(const char *input,std::array<unsigned char,32> &output) {
	auto result = BCryptCreateHash(algorithmHandle,&hashHandle,0,0,0,0,0);
	if (result != STATUS_SUCCESS) {
		throw std::exception("BCrypt hash creation failed.");
	}

	result = BCryptHashData(hashHandle,(unsigned char *)input,strlen(input),0);
	if (result != STATUS_SUCCESS) {
		throw std::exception("BCrypt hash data generation failed.");
	}
	result = BCryptFinishHash(hashHandle,output.data(),32,0);
	if (result != STATUS_SUCCESS) {
		throw std::exception("BCrypt hash finalization failed.");
	}

	result = BCryptDestroyHash(hashHandle);
	if (result != STATUS_SUCCESS) {
		throw std::exception("BCrypt hash destruction failed.");
	}
}