#pragma once

//Minimal reimplementation of std::string from Visual Studio 6.  The modern string class has a different layout and cannot be used.
class VC6String {
	int allocator;
public:
	char *pointer;
	int length;
private:
	int reserved;

public:
	VC6String() {
		pointer = nullptr;
		length = 0;
		allocator = 0;
		reserved = 0;
	}
	VC6String(const VC6String &other) {
		pointer = new char[other.length + 2];
		memcpy(pointer,other.pointer - 1,length + 2);
		pointer++;
		length = other.length;
		allocator = other.allocator;
		reserved = other.reserved;
	}
	VC6String(VC6String &&other) noexcept {
		pointer = other.pointer;
		other.pointer = nullptr;
		length = other.length;
		other.length = 0;
		allocator = other.allocator;
		reserved = other.reserved;
	}

	VC6String &operator=(const VC6String &other) {
		if (pointer != nullptr) {
			delete [](pointer - 1);
		}
		pointer = new char[other.length + 2];
		memcpy(pointer,other.pointer - 1,length + 2);
		pointer++;
		length = other.length;
		allocator = other.allocator;
		reserved = other.reserved;

		return *this;
	}

	VC6String &operator=(VC6String &&other) noexcept {
		if (pointer != nullptr) {
			delete [](pointer - 1);
		}
		pointer = other.pointer;
		other.pointer = nullptr;
		length = other.length;
		other.length = 0;
		allocator = other.allocator;
		reserved = other.reserved;

		return *this;
	}

	VC6String &operator=(const char *input) {
		length = strlen(input);
		pointer = new char[length + 2];
		*pointer = 1; //Internal reference count.
		strcpy_s(pointer + 1,length + 1,input);
		pointer = pointer + 1; //The base is set after the reference count.

		return *this;
	}
	~VC6String() {
		if (pointer != nullptr) {
			delete [](pointer - 1);
		}
	}

	const char *c_str() const {
		return pointer;
	}

	bool operator==(const VC6String &other) const {
		return length == other.length && !strcmp(pointer,other.pointer);
	}

	bool operator<(const VC6String &other) const {
		if (length != other.length) {
			return length < other.length;
		}
		return strcmp(pointer,other.pointer) < 0;
	}
};