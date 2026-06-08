#pragma once
#ifndef ensure
	#define ensure(Expr) (!!(Expr) || ([]() { ( __nop(), __debugbreak()); return false; } ()))
#endif