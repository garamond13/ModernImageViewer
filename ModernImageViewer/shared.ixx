module;
#include "framework.h"

export module shared;
import config;

//globaly used data
export namespace shared {
	HINSTANCE hinstance;
	HWND hwnd;
	Config config;
}