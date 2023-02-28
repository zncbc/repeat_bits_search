#include "repeat_test.h"
#pragma warning(disable:4996)

#define MAX_SORT_BUF_SIZE (3<<27)
#define MAX_SORT_BLOCK_NUM (1<<16)
#define MAX_SORT_BLOCK_SIZE (3<<11)
#define SORT_BLOCK_HEAD(i) (((i)<<11)*3)
#define MAX_RB_LIST_NUM 100000

struct repeat_bits
{
	unsigned __int64 s1, r1, offset;
	unsigned __int16 s2, r2;
	unsigned __int8 l;
};
int G_flag = 0;

//以梅森旋转算法生成随机序列
void create_test_data(int file_num, int seed, __int64 file_size, std::string path_name)
{

	LARGE_INTEGER t1, t2, tc;//计时工具
	QueryPerformanceFrequency(&tc);
	QueryPerformanceCounter(&t1);//计时开始

	std::string file_name = path_name + "0000.bin";
	const int buf_size = 1 << 18;//4*2^18=2^20B=1MB
	unsigned int *buf = new unsigned int[buf_size];
	std::mt19937 mt_rd(seed);
	std::ofstream fout;
	int file_name_len = file_name.size();
	for (int i = 0; i < file_num; i++)
	{
		file_name[file_name_len - 5] = i % 10 + '0';
		file_name[file_name_len - 6] = i / 10 % 10 + '0';
		file_name[file_name_len - 7] = i / 10 % 10 + '0';
		file_name[file_name_len - 8] = i / 10 % 10 + '0';
		//printf("generating %d\n",file_name);
		fout.open(file_name, std::ios::out | std::ios::binary);
		for (int j = 0; j < file_size; j++)
		{
			for (int k = 0; k < buf_size; k++)
			{
				buf[k] = mt_rd();
			}
			fout.write((char*)buf, buf_size * 4);//int类型占4个字节
		}
		fout.close();
	}
	free(buf);
	QueryPerformanceCounter(&t2);//计时结束
	double time = (double)(t2.QuadPart - t1.QuadPart) / (double)tc.QuadPart;
	printf("create finish ,time = %lf\n",time);  //输出时间（单位：ｓ）
}

//输出固定位数的十六进制
void print_binary(unsigned __int64 x, int digits)
{
	for (int i = digits - 1; i >= 0; i--)
	{
		int d = (x >> (4 * i)) & 0xf;
		if (d < 10)
			putchar('0' + d);
		else
			putchar('A' + d - 10);
	}
}

int cmp_rb(repeat_bits& A, repeat_bits& B)
{
	return A.r1 < B.r1;
}

int cmp16(unsigned __int64& A, unsigned __int64& B)
{
	return (A&0xffff) < (B&0xffff);
}

unsigned __int64 reverse_bit64(unsigned __int64 n)
{
	n = ((n & 0xffffffff00000000) >> 32ll) | ((n & 0x00000000ffffffff) << 32ll);
	n = ((n & 0xffff0000ffff0000) >> 16)   | ((n & 0x0000ffff0000ffff) << 16);
	n = ((n & 0xff00ff00ff00ff00) >> 8)    | ((n & 0x00ff00ff00ff00ff) << 8);
	n = ((n & 0xf0f0f0f0f0f0f0f0) >> 4)    | ((n & 0x0f0f0f0f0f0f0f0f) << 4);
	n = ((n & 0xcccccccccccccccc) >> 2)	   | ((n & 0x3333333333333333) << 2);
	n = ((n & 0xaaaaaaaaaaaaaaaa) >> 1)    | ((n & 0x5555555555555555) << 1);
	return n;
}

unsigned __int32 reverse_bit16(unsigned __int32 n)
{
	n = ((n & 0xff00) >> 8) | ((n & 0x00ff) << 8);
	n = ((n & 0xf0f0) >> 4) | ((n & 0x0f0f) << 4);
	n = ((n & 0xcccc) >> 2) | ((n & 0x3333) << 2);
	n = ((n & 0xaaaa) >> 1) | ((n & 0x5555) << 1);
	return n;
}

void create_index(std::vector<unsigned __int64> &rep56_list, unsigned __int16 *st16, unsigned __int16 *ed16)
{
	int j = 0;
	for (int i = 0; i < rep56_list.size(); i++)
	{
		int s = rep56_list[i] & 0xffff;
		for (int k = j; k < s; k++)
		{
			st16[k] = 0;
			ed16[k] = 0;
		}
		st16[s] = i;
		ed16[s] = i + 1;
		for (; i + 1 < rep56_list.size(); i++)
		{
			if (s != (rep56_list[i + 1] & 0xffff))
			{
				break;
			}
			ed16[s]++;
		}
		j = s + 1;
	}
	for (int k = j; k < 65536; k++)
	{
		st16[k] = 0;
		ed16[k] = 0;
	}
	//for (int i = 0; i < 65536; i++)
//{
//	if (st16[i] != 0 || ed16[i] != 0)
//	{
//		print_binary(i, 4);
//		printf(" %d %d\n", st16[i], ed16[i]);
//	}
//}
}

void find_80_offset(read_file &F, std::vector<unsigned __int64> &rep56_list, std::vector<repeat_bits> &rb_list, int thread_num)
{
	unsigned __int16 *st16 = new unsigned __int16[1 << 16];
	unsigned __int16 *ed16 = new unsigned __int16[1 << 16];
	F.set_block(1 << 20);
	unsigned __int32 total_size_mb = F.get_block_num();
	unsigned char *buf;
	unsigned __int64 f0_save_8bytes = 0;
	unsigned __int8 f0_remain_byte = 0;
	unsigned __int8 least_byte = 0;
	create_index(rep56_list, st16, ed16);
	omp_set_num_threads(thread_num);
	F.start();
	for (int j = 0; j < total_size_mb; j++)
	{
		buf = (unsigned char *)F.read();
		#pragma omp parallel for
		for (int l = 0; l <= 7; l++)
		{
			unsigned __int64 save_8bytes = (f0_save_8bytes << l) + (least_byte >> (8 - l));
			unsigned __int8 remain_byte = (f0_remain_byte << l) + ((f0_save_8bytes >> 56) >> (8 - l));
			for (int k = 0; k < (1 << 20); k++)
			{
				unsigned __int8 input_byte;
				if (k == 0)
					input_byte = (least_byte << l);
				else
					input_byte = (buf[k - 1] << l);
				if (l != 0)
				{
					input_byte += (buf[k]) >> (8 - l);
				}
				if (j != 0 || k >= 10)
				{
					unsigned __int64 check56 = input_byte + ((save_8bytes << 8) & 0xffffffffffffff);
					unsigned __int16 index = check56 & 0xffff;
					for (int i = st16[index]; i < ed16[index]; i++)
					{
						if (check56 == rep56_list[i])
						{
							repeat_bits tmp;
							tmp.l = l;
							tmp.offset = (unsigned __int64(j) << 20) + k;
							tmp.s1 = input_byte + (save_8bytes << 8);
							tmp.s2 = (remain_byte << 8) + (save_8bytes >> 56);
							tmp.r1 = reverse_bit64(tmp.s1);
							tmp.r2 = reverse_bit16(tmp.s2);
							#pragma omp critical
							{
								rb_list.push_back(tmp);
							}
						}
					}
				}
				remain_byte = save_8bytes >> 56;
				save_8bytes = (save_8bytes << 8) + input_byte;
			}
		}
		f0_save_8bytes = ((unsigned __int64)buf[0xffff7] << 56) +
			((unsigned __int64)buf[0xffff8] << 48) + ((unsigned __int64)buf[0xffff9] << 40) +
			((unsigned __int64)buf[0xffffa] << 32) + ((unsigned __int64)buf[0xffffb] << 24) +
			((unsigned __int64)buf[0xffffc] << 16) + ((unsigned __int64)buf[0xffffd] << 8) + buf[0xffffe];
		f0_remain_byte = buf[0xffff6];
		least_byte = buf[0xfffff];
		F.free();
		if (rb_list.size() > 1 << 20)
		{
			printf("找到过多重复串，");
			break;
		}
	}
	free(st16);
	free(ed16);
}

void contrast80(read_file &F, std::vector<repeat_bits> &rb_list ,int out_len, char *result_file)
{
	freopen(result_file, "w", stdout);
	std::sort(rb_list.begin(), rb_list.end(), cmp_rb);
	int count[25] = { 0 };
	for (int i = 1; i < rb_list.size(); i++)
	{
		unsigned __int64 d1 = rb_list[i].s1^rb_list[i - 1].s1;
		int same_bit;
		if (d1 == 0)
		{
			unsigned __int16 d2 = rb_list[i].s2^rb_list[i - 1].s2;
			if (d2 == 0)
				same_bit = 80;
			else
				same_bit = 64 + log2(d2 - (d2&(d2 - 1)));
		}
		else
		{
			same_bit = log2(d1 - (d1&(d1 - 1)));
		}
		if (same_bit >= 56)
		{
			count[same_bit - 56]++;
		}
		if (same_bit >= out_len)
		{
			printf("position: ");
			print_binary(rb_list[i].offset - 10, 16);
			printf("-");
			print_binary(rb_list[i].offset, 16);
			printf(" shift: %d\n", rb_list[i].l);
			printf("\n");
			print_binary(rb_list[i].s2, 4);
			print_binary(rb_list[i].s1, 16);

			printf("\nposition: ");
			print_binary(rb_list[i - 1].offset - 10, 16);
			printf("-");
			print_binary(rb_list[i - 1].offset, 16);
			printf(" shift: %d\n", rb_list[i - 1].l);
			printf("\n");
			print_binary(rb_list[i - 1].s2, 4);
			print_binary(rb_list[i - 1].s1, 16);
			printf("\nrepeat length:%d\n\n", same_bit);
		}
	}
	printf("summary:\n");
	printf("repeat length |count   |upper bound|lower bound|\n");
	double e = F.get_total_size();
	e = e * e / pow(2, 53);
	for (int i = 0; i < 24; i++)
	{
		double ei = e / pow(2 , i);
		double xi = ei - 3 * sqrt(ei);
		double yi = ei + 3 * sqrt(ei);
		if (xi < 0)
			xi = 0;
		printf("%14d|%8d|%11d|%11d|\n", i + 56, count[i] - count[i + 1], int(yi+0.5), int(xi+0.5));
	}
	freopen("CON", "w", stdout);
}

int repeat_search(int file_num, char** path_name, char *result_file, int thread_num,bool avx)
{
	LARGE_INTEGER t1, t2, tc;//计时工具
	QueryPerformanceFrequency(&tc);
	QueryPerformanceCounter(&t1);//计时开始
	double time;

	read_file F(file_num, path_name);

	//计算总字节数;
	unsigned __int64 tot_s = F.get_total_size();
	if (tot_s % READ_BLOCK_SIZE != 0)
	{
		printf("waring!! %lld byte in data end will be discard.\n", tot_s - tot_s / READ_BLOCK_SIZE * READ_BLOCK_SIZE);
	}

	//找到所有重复出现的56比特
	std::vector <unsigned __int64> rep56_list;
	rep56 A;
	A.find(F, rep56_list, thread_num, avx);
	std::sort(rep56_list.begin(), rep56_list.end(), cmp16);

	if (rep56_list.size() >= 65536)
	{
		printf("The number of repeated 56 bits exceeds the expected value.\n");
		printf("Please pay attention to whether there are large segments of repeated data.\n");
		printf("The calculation result will only process the top 65535 repeated 56 bits.\n");
	}
	while (rep56_list.size()>= 65536)
	{
		rep56_list.pop_back();
	}
	for (int i = 0; i < rep56_list.size(); i++)
	{
		print_binary(rep56_list[i], 16); putchar('\n');
	}

	std::vector<repeat_bits> rb_list;
	find_80_offset(F, rep56_list, rb_list, thread_num);
	contrast80(F, rb_list, 56, result_file);

	QueryPerformanceCounter(&t2);//计时开始
	time = (double)(t2.QuadPart - t1.QuadPart) / (double)tc.QuadPart;
	printf("time = %lf\n", time);
	return 0;
}
