#pragma once

struct option {
	const char* name;
	int has_arg;
	int* flag;
	int val;
};