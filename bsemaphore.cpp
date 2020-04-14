#include "bsemaphore.h"

using namespace std;

BSemaphore::BSemaphore(bool b)
	: Semaphore(b)
{}

void BSemaphore::up()
{
	unique_lock<mutex> _lock(_mut);
	if (!_count) {
		_count = 1;
		_cond.notify_one();
	}
}

void BSemaphore::down()
{
	unique_lock<mutex> _lock(_mut);
	if (!_count) {
		_cond.wait(_lock);
	}
	_count = 0;
}

BSemaphore::operator bool()
{
	scoped_lock<mutex> lk(_mut);
	return _count;
}