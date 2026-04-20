#pragma once

void Patch(int address,std::initializer_list<unsigned char> bytes);
void Patch(int address,unsigned char value,int count);