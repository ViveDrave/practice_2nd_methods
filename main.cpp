////////////////////////////////////////////////////////////////////////////////
// Filename: main.cpp
////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include "DX3D/Include/systemclass.h"


int main() {
	SystemClass* System;

	System = new SystemClass;

	bool result;
	result = System->Initialize();

	if (result) {
		System->Run();
	}

	System->Shutdown();
	delete System;
	System = 0;

	return 0;
}