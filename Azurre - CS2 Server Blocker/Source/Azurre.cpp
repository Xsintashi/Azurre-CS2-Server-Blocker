#include <iostream>
#include "Mutex.h"
#include "GUI.h"
#include "Firewall.h"
#include "Core.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR arguments, int commandShow) {
#if !defined (_DEBUG)
	if (initMutex())
		return 0;
#endif
	hInstance = instance;
	Core::init();
	GUI::init();
	return 0;
}