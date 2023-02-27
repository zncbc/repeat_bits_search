#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <immintrin.h>
#include <intrin.h>
#include <omp.h>

#include "read_file.h"

class rep56
{
public:
	void find(read_file &F, std::vector <unsigned __int64> &rep56_list, int thread_num, bool avx);
private:
	void work_thread(unsigned __int32 *sort_array, unsigned __int16 *sort_tail, unsigned __int8 const *buf, int time, int thread_i);
	void work_thread_avx(unsigned __int32 *sort_array, unsigned __int16 *sort_tail, unsigned __int8 const *buf, int time, int thread_i);
	void calc_list(unsigned __int8 cmp_head[256],unsigned __int8 cmp_tail[256], int i);
	void sort_in(int u, unsigned __int32 *sort_array, unsigned __int16 *sort_tail, std::vector<unsigned __int64> &save);

	int bucket_size, bucket_num;
	bool is_avx;
};

