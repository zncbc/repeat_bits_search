#pragma warning(disable:4996)

#include <iostream>
#include <algorithm>
#include <random>
#include <windows.h>
using namespace std;

#include "repeat_test.h"
//#define SORT1
int main()
{
	//create_test_data(1,23,2,"");
	const char *name0 = "0000.bin";
	char **name = new char *[10];
	for (int i = 0; i < 10; i++)
	{
		name[i] = new char[50];
		strcpy(name[i], name0);
		name[i][3] = i + '0';
	}
	repeat_search(1, name);
}