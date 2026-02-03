#pragma once

#include <exception>

//Minimal reimplementation of std::list from Visual Studio 6.  The modern list class has a different layout and cannot be used.
template <typename Type> class VC6List {
	class Node {
	public:
		Node *next;
		Node *previous;
		Type value;

		Node() {
			next = this;
			previous = this;
		}
		Node(const Type &input) : value(input) {
		}
		Node(Type &&input) : value(std::move(input)) {
		}
	};

	class Iterator {
		Node *head;
		Node *node;
	public:
		Iterator(Node *head,Node *node) : head(head),node(node) {
		}

		Iterator operator++() {
			if (node == head) {
				throw new std::exception("Unable to increment iterator beyond the end of the list.");
			}
			node = node->next;
			return *this;
		}
		Iterator operator--() {
			if (node->previous == head) {
				throw new std::exception("Unable to decrement iterator beyond the beginning of the list.");
			}
			node = node->previous;
			return *this;
		}

		bool operator==(const Iterator &other) {
			return node == other.node;
		}
		bool operator!=(const Iterator &other) {
			return node != other.node;
		}

		Type &operator*() {
			return node->value;
		}
		Type *operator->() {
			return &node->value;
		}

		Node *Node() const {
			return node;
		}
	};

	int allocator;
	Node *head;
	int size_;

	void Add(Node *node) {
		node->previous = head->previous;
		node->next = head;
		head->previous->next = node;
		head->previous = node;
		++size_;
	}

	void DeleteAll() {
		Node *current = head->next;
		while (current != head) {
			auto next = current->next;

			delete current;

			current = next;
		}
		delete head;
	}

public:
	VC6List() {
		allocator = 0;
		head = new Node();
		size_ = 0;
	}
	VC6List(VC6List<Type> &other) {
		head = new Node();
		Node *current = head;
		for (auto iterator = other.begin();iterator != other.end();++iterator) {
			current->next = new Node(*iterator);
		}
		current->next = head;
		size_ = other.size_;
	}
	VC6List(VC6List<Type> &&other) {
		head = other.head;
		other.head = nullptr;

		size_ = other.size_;
	}
	~VC6List() {
		DeleteAll();
	}

	void push_back(const Type &value) {
		auto node = new Node(value);
		Add(node);
	}

	void push_back(Type &&value) {
		auto node = new Node(std::move(value));
		Add(node);
	}

	const int size() const {
		return size_;
	}

	Iterator begin() {
		return Iterator(head,head->next);
	}
	Iterator end() {
		return Iterator(head,head);
	}

	void clear() {
		DeleteAll();
		head = new Node();
		size_ = 0;
	}

	void erase(Iterator position) {
		auto node = position.Node();
		node->next->previous = node->previous;
		node->previous->next = node->next;
		delete node;
		size_--;
	}
};