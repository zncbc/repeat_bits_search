#pragma once
#include <iostream>
#include <fstream>
#include <algorithm>
#include <windows.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <random>
#include <omp.h>
#include "read_file.h"
#define normal 0
#define excessive_samedata 1
#define excessive_rep56 2
#define excessive_repstring 3
void create_test_data(int n, int seed, __int64 file_size, std::string path_name);

int repeat_search(int n, char** path_name);