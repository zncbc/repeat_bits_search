#pragma once
#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <mutex>
#include <condition_variable>

#define SLEEP(x) std::this_thread::sleep_for(std::chrono::milliseconds(1*x))
#define MAX_ROUND 8
#define READ_BLOCK_SIZE (1<<25)
#define ERR_REP_STR 1

class read_file
{
public:
	read_file(int n, char** path);
	int start();
	char *read();
	void free();
	int reset();
	int end();
	bool read_if();
	__int64 get_total_size();
	__int32 get_block_num();
	void set_block(__int32 block_size_);
private:
	int read_thread();
	char *buf;
	int file_i;
	std::ifstream fin;
	int file_n;
	bool is_start;
	int data_block_i;
	int data_block_e;
	std::mutex data_flag[MAX_ROUND];
	std::condition_variable data_rd,data_et;
	
	char** path_name;
	__int64 file_size;
	__int64 total_size;
	__int32 block_size;
};