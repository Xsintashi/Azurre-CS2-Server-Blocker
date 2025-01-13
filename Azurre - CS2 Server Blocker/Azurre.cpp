#include <iostream>
#include "Mutex.h"
#include "GUI.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR arguments, int commandShow) {
#if !defined (_DEBUG)
	if (initMutex())
		return 0;
#endif
	GUI::init();
	return 0;
}