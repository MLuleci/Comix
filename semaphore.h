#pragma once
#include <condition_variable>
#include <mutex>

class Semaphore {
	std::mutex _mut;
	std::condition_variable _cond;
	int _count;
public:
	Semaphore(int i = 0);

	void up();
	void down();
	operator int();
};