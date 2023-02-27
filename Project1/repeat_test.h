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
#include "rep56.h"
void create_test_data(int n, int seed, __int64 file_size, std::string path_name);

int repeat_search(int n, char** path_name);
int repeat_search(int file_num, char** path_name, char *result_file, int thread_num,bool avx);