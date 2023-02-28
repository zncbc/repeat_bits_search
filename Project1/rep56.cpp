#include "rep56.h"
#pragma warning(disable:4996)

#define BUCKET_NUM0 (1<<16)
#define BUCKET_SIZE0 (1<<13)

#define SEARCH8 256
std::mutex data_flag;
std::condition_variable data_rd, data_ep;
int c_thread_num;
int data_use = 0;


extern void print_binary(unsigned __int64 x, int digits);

void rep56::sort_in(int u, unsigned __int32 *sort_array, unsigned __int16 *sort_tail, std::vector<unsigned __int64> &save)
{
	omp_set_num_threads(c_thread_num);
	const int store_size = 1<<16;
	const int thread_ar_block = bucket_num * bucket_size / c_thread_num;
	const int thread_ta_block = bucket_num / c_thread_num;
	const int thread_store_block = store_size / c_thread_num;
	unsigned __int32 *sort_buf = new unsigned __int32[bucket_size * c_thread_num * c_thread_num];
	unsigned __int64 *rep_store = new unsigned __int64[store_size];

	memset(rep_store, 0, store_size * sizeof(unsigned __int64));
	int d = 0;
	for (int i = 0; i < 1 << 19; i++)
	{
		if (sort_tail[i] > d)
			d = sort_tail[i];
	}
	printf("max %d\n", d);
	#pragma omp parallel for
	for (int i = 0; i < 1 << 16; i++)
	{
		int threadi = omp_get_thread_num();
		unsigned __int32 *dst, *src, *tmp;
		unsigned __int16 src_len;
		tmp = dst = sort_buf + threadi * bucket_size * c_thread_num;
		for (int j = 0; j < c_thread_num; j++)
		{
			src = sort_array + i * bucket_size + j * thread_ar_block;
			src_len = sort_tail[i + j * thread_ta_block];
			memcpy(tmp, src, src_len * sizeof(unsigned __int32));
			tmp += src_len;
		}
		//printf("%d,%d,%d\n", i, threadi, rep_store[thread_store_block * threadi]);
		std::sort(dst, tmp);
		//printf("%d,%d\n", tmp - dst, sort_tail[i]);
	
		for (int j = 1; j < tmp - dst; j++)
		{
			if (dst[j] == dst[j - 1])
			{
				++rep_store[thread_store_block * threadi];
				rep_store[thread_store_block * threadi + rep_store[thread_store_block * threadi]] 
					= (((unsigned __int64)i) << 32) + dst[j];
			}
			//printf("%d,",j); print_binary(dst[j], 8); putchar('\n');
		}
		//printf("%d,%d\n", i, rep_store[0]);

	}
	for (int i = 0; i < c_thread_num; i++)
	{
		for (int j = 1; j <= rep_store[i * thread_store_block]; j++)
		{
			save.push_back((rep_store[j + thread_store_block * i] << 8) + u);
		}
	}
}

bool data_abnormal( unsigned __int16 *sort_tail)
{
	for (int i = 0; i < BUCKET_NUM0 * c_thread_num; i++)
	{
		if (sort_tail[i] > BUCKET_SIZE0 / c_thread_num)
			return true;
	}
	return false;
}

void rep56::find(read_file &F , std::vector <unsigned __int64> &rep56_list,int thread_num, bool avx)
{
	if (thread_num >= 8)
		c_thread_num = 8;
	else if (thread_num >= 4)
		c_thread_num = 4;
	else if (thread_num >= 2)
		c_thread_num = 2;
	else
		c_thread_num = 1;
	bucket_num = BUCKET_NUM0 * c_thread_num;
	bucket_size = BUCKET_SIZE0 / c_thread_num;
	is_avx = avx;
	unsigned __int32 *sort_array = new unsigned __int32[bucket_num * bucket_size + READ_BLOCK_SIZE];
	unsigned __int16 *sort_tail = new unsigned __int16[bucket_num];
	unsigned __int8 *buf = new unsigned __int8[READ_BLOCK_SIZE + 8];
	int read_time = F.get_block_num();
	std::thread T1(&rep56::work_thread, this, sort_array, sort_tail, buf, read_time, 0);
	std::thread T2(&rep56::work_thread, this, sort_array, sort_tail, buf, read_time, 1);
	std::thread T3(&rep56::work_thread, this, sort_array, sort_tail, buf, read_time, 2);
	std::thread T4(&rep56::work_thread, this, sort_array, sort_tail, buf, read_time, 3);
	std::thread T5(&rep56::work_thread, this, sort_array, sort_tail, buf, read_time, 4);
	std::thread T6(&rep56::work_thread, this, sort_array, sort_tail, buf, read_time, 5);
	std::thread T7(&rep56::work_thread, this, sort_array, sort_tail, buf, read_time, 6);
	std::thread T8(&rep56::work_thread, this, sort_array, sort_tail, buf, read_time, 7);
	std::thread T9(&rep56::work_thread_avx, this, sort_array, sort_tail, buf, read_time, 0);
	std::thread T10(&rep56::work_thread_avx, this, sort_array, sort_tail, buf, read_time, 1);
	std::thread T11(&rep56::work_thread_avx, this, sort_array, sort_tail, buf, read_time, 2);
	std::thread T12(&rep56::work_thread_avx, this, sort_array, sort_tail, buf, read_time, 3);
	std::thread T13(&rep56::work_thread_avx, this, sort_array, sort_tail, buf, read_time, 4);
	std::thread T14(&rep56::work_thread_avx, this, sort_array, sort_tail, buf, read_time, 5);
	std::thread T15(&rep56::work_thread_avx, this, sort_array, sort_tail, buf, read_time, 6);
	std::thread T16(&rep56::work_thread_avx, this, sort_array, sort_tail, buf, read_time, 7);
	for (int i = 0; i < SEARCH8; i++)
	{
		F.start();
		memset(buf, 0, 8);
		memset(sort_tail, 0, bucket_num * sizeof(unsigned __int16));
		for (int j = 0; j < read_time; j++)
		{
			{
				std::unique_lock<std::mutex> lk(data_flag);
				memcpy(buf, buf + READ_BLOCK_SIZE, 8);
				memcpy(buf + 8, F.read(), READ_BLOCK_SIZE);
				F.free();

				data_use = (1 << thread_num) - 1;
				//printf("r%d\n" , j);
				data_rd.notify_all();
			}
			{
				std::unique_lock<std::mutex> lk(data_flag);
				data_ep.wait(lk, [this] {return data_use == 0; });
				if (((j + 1) % 32 == 0) && (rep56_list.size() > 65536 || data_abnormal(sort_tail)))
				{
					data_use = 0xffffffff;
					printf("error");
					break;
				}
					//printf(" ed\n");
			}
		}
		sort_in(i, sort_array, sort_tail, rep56_list);
		F.end();
		printf("(%3d/256)\n",i);
	}
	T1.join();
	T2.join();
	T3.join();
	T4.join();
	T5.join();
	T6.join();
	T7.join();
	T8.join();
	T9.join();
	T10.join();
	T11.join();
	T12.join();
	T13.join();
	T14.join();
	T15.join();
	T16.join();
	delete sort_array, sort_tail;
	delete buf;
	for (int i = 0; i < rep56_list.size(); i++)
	{
		print_binary(rep56_list[i], 16); putchar('\n');
	}
	printf("%d,",rep56_list.size());
}

void rep56::calc_list(unsigned __int8 cmp_head[256],unsigned __int8 cmp_tail[256], int i)
{
	for (int j = 0; j < 256; j++)
	{
		cmp_head[j] = 0;
		cmp_tail[j] = 0;
		unsigned __int32 x, y;
		for (x = j, y = i; x != y && cmp_head[j] < 8; cmp_head[j]++)
		{
			x = (x & ~(1 << (7 - cmp_head[j]))) & 0xff;
			y >>= 1;
			if (x == y)
				break;
		}
		for (x = i, y = j; (cmp_tail[j] == 0) || (x != y && cmp_tail[j] < 8); cmp_tail[j]++)
		{
			x = (x & ~(1 << (7 - cmp_tail[j]))) & 0xff;
			y >>= 1;
			if (x == y && (cmp_tail[j] != 0))
				break;
		}
		cmp_tail[j] = 8 - cmp_tail[j];
	}
}

void rep56::work_thread(unsigned __int32 *sort_array, unsigned __int16 *sort_tail, unsigned __int8 const *buf, int time,int thread_i)
{
	if (thread_i >= c_thread_num || is_avx)
	{
		return;
	} 
	const int buf_len = READ_BLOCK_SIZE / c_thread_num;

	buf += thread_i * buf_len;

	sort_array += bucket_num * bucket_size / c_thread_num * thread_i;
	sort_tail += bucket_num / c_thread_num * thread_i;
	unsigned __int8 cmp_head[256];
	unsigned __int8 cmp_tail[256];
	for (int i = 0; i < SEARCH8; i++) 
	{
		calc_list(cmp_head, cmp_tail, i);
		unsigned __int64 t = 0;
		for (int j = 0; j < time; j++)
		{
			{
				std::unique_lock<std::mutex> lk(data_flag);
				data_rd.wait(lk, [this, thread_i] {return (data_use&(1 << thread_i)) != 0; });
				if (data_use == 0xffffffff)
				{
					return;
				}
			}
			unsigned __int64 save_8bytes = 0;
			unsigned __int8 input_byte = 0;
			for (int k = 0; k < buf_len; k++)
			{
				input_byte = buf[k];
				if (k >= 8)
				{
					for (int l = cmp_head[save_8bytes & 0xff]; l <= cmp_tail[input_byte]; l++)
					{
						if ((0xff & (((save_8bytes & 0xff) << l)) + (input_byte >> (8 - l))) == i)
						{
							unsigned __int64 data = ((save_8bytes << l) >> 8) & 0xffffffffffffll;
							unsigned __int32 dat1 = data >> 32ll, dat2 = data & 0xffffffff;
							sort_array[dat1 * bucket_size + sort_tail[dat1]] = dat2;
							sort_tail[dat1]++;
						}
					}
				}
				save_8bytes = (save_8bytes << 8) + input_byte;
			}
			{
				std::unique_lock<std::mutex> lk(data_flag);
				data_use -= 1 << thread_i;
				data_ep.notify_all();
			}
		}
	}
}

void rep56::work_thread_avx(unsigned __int32 *sort_array, unsigned __int16 *sort_tail, unsigned __int8 const *buf, int time, int thread_i)
{
	if (thread_i >= c_thread_num || (!is_avx))
	{
		return;
	}
	const int buf_len = READ_BLOCK_SIZE / c_thread_num / 4;

	buf += thread_i * buf_len * 4;

	sort_array += bucket_num * bucket_size / c_thread_num * thread_i;
	sort_tail += bucket_num / c_thread_num * thread_i;
	for (int i = 0; i < SEARCH8; i++)
	{
		unsigned __int32 s1 = 0x5221AEEF;
		unsigned __int32 s2 = 0x5221AEEF;
		for (int j = 0; j < time; j++)
		{
			{
				std::unique_lock<std::mutex> lk(data_flag);
				//printf("w%d ", thread_i);
				data_rd.wait(lk, [this, thread_i] {return (data_use&(1 << thread_i)) != 0; });
				if (data_use == 0xffffffff)
				{
					printf("ffffff");
					return;
				}
				//printf("g%d ", thread_i);
			}
			unsigned __int32 *p = (unsigned __int32 *)buf;

			for (int k = 0; k < buf_len; k++)
			{
				unsigned __int64 s3 = p[k];
				s3 = (s3 >> 16) + (s3 << 16);
				s3 = ((s3 >> 8) & 0x00ff00ff) + ((s3 << 8) & 0xff00ff00);
				__m256i A, B;
				A = _mm256_set1_epi32(s3);
				A = _mm256_srlv_epi32(A, _mm256_set_epi32(0, 1, 2, 3, 4, 5, 6, 7));
				B = _mm256_set1_epi32(s2);
				B = _mm256_sllv_epi32(B, _mm256_set_epi32(32, 31, 30, 29, 28, 27, 26, 25));
				A = _mm256_add_epi32(A, B);
				A = _mm256_cmpeq_epi8(A, _mm256_set1_epi8(i));
				A = _mm256_and_si256(A, _mm256_set1_epi64x(0x1020408001020408ll));
				A = _mm256_sad_epu8(A, _mm256_set1_epi32(0));
				B = _mm256_srli_si256(A, 7);
				A = _mm256_add_epi64(A, B);
				unsigned __int32 s0;
				s0 = _mm256_extract_epi16(A, 0) + (_mm256_extract_epi16(A, 8) << 16);
				while (s0 != 0)
				{
					unsigned __int32 t = s0 - (s0 & (s0 - 1));
					s0 -= t;
					_BitScanForward((unsigned long*)&t, t);
					t = 31 - (t / 4) - (t % 4) * 8;

					unsigned __int32 da2 = ((s2 >> t) + (s1 << (32 - t)) * (t > 0));
					unsigned __int16 dat = da2 >> 8;
					sort_array[dat * bucket_size + sort_tail[dat]] = (((s3 >> t) + (s2 << (32 - t))*(t > 0)) >> 8) + (da2 << 24);
					++sort_tail[dat];
				}
				s1 = s2;
				s2 = s3;
			}
			{
				std::unique_lock<std::mutex> lk(data_flag);
				data_use -= 1 << thread_i;
				data_ep.notify_all();
					//printf("e%d ", thread_i);
			}
		}
	}
}