#include "Mutex.h"
#include "xor.h"
#include <mutex>
HANDLE mutex = 0;
INT size = 0;

 

int initMutex() {

	constexpr char mutexName[15] = "azurreRoulette";

	mutex = OpenMutex(MUTEX_ALL_ACCESS, 0, mutexName);
	
	if (!mutex) mutex = CreateMutex(0, 0, mutexName);
	else {
		MessageBoxA(nullptr, xin("Only one instance of the software can be running at one time."), xin("Azurre 2"), MB_OK | MB_ICONERROR);
		return 1;
	}
	return 0;
}

int uninitMutex() {
	ReleaseMutex(mutex);
	return 0;
}