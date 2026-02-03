#pragma once

template <typename Type>
class MultiThreadedQueue {
	std::queue<Type> queue;
	std::mutex mutex;

public:
	void push(const Type &value) {
		std::lock_guard<std::mutex> lock(mutex);

		queue.push(value);
	}
	void push(Type &&value) {
		std::lock_guard<std::mutex> lock(mutex);

		queue.push(std::move(value));
	}

	const Type pop() {
		std::lock_guard<std::mutex> lock(mutex);

		const Type value = std::move(queue.front());
		queue.pop();
		return value;
	}

	std::size_t size() {
		std::lock_guard<std::mutex> lock(mutex);

		return queue.size();
	}
};