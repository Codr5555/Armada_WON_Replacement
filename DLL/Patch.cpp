#include "Patch.h"

void Patch(int address,std::initializer_list<unsigned char> bytes) {
	unsigned long oldProtect;
	VirtualProtect(std::bit_cast<void*>(address),bytes.size(),PAGE_EXECUTE_READWRITE,&oldProtect);

	auto base = std::bit_cast<unsigned char *>(address);
	for (unsigned int index = 0;index < bytes.size();index++) {
		base[index] = bytes.begin()[index];
	}

	VirtualProtect(std::bit_cast<void*>(address),bytes.size(),oldProtect,&oldProtect);
}

void Patch(int address,unsigned char value,int count) {
	unsigned long oldProtect;
	VirtualProtect(std::bit_cast<void*>(address),count,PAGE_EXECUTE_READWRITE,&oldProtect);

	auto base = std::bit_cast<unsigned char *>(address);
	for (unsigned int index = 0;index < count;index++) {
		base[index] = value;
	}

	VirtualProtect(std::bit_cast<void*>(address),count,oldProtect,&oldProtect);
}