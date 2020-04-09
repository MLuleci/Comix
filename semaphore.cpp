#include "semaphore.h"

using namespace std;

Semaphore::Semaphore(int i)
	: _count(i)
{}

void Semaphore::up()
{
	unique_lock<mutex> _lock(_mut);
	if (++_count <= 0) {
		_cond.notify_one();
	}
}

void Semaphore::down()
{
	unique_lock<mutex> _lock(_mut);
	if (--_count < 0) {
		_cond.wait(_lock);
	}
}

Semaphore::operator int()
{
	scoped_lock<mutex> lk(_mut);
	return _count;
}