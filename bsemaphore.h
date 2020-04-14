#pragma once
#include "semaphore.h"

class BSemaphore : public Semaphore {
public:
	BSemaphore(bool b = 0);

	void up() override;
	void down() override;
	operator bool();
};