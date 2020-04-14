#pragma once
#include <condition_variable>
#include <mutex>

class Semaphore {
protected:
	std::mutex _mut;
	std::condition_variable _cond;
	int _count;
public:
	Semaphore(int i = 0);

	virtual void up();
	virtual void down();
	operator int();
};