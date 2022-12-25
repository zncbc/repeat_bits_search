#include "repeat_test.h"
#pragma warning(disable:4996)

#define MAX_SORT_BUF_SIZE (1<<29)
#define MAX_SORT_BLOCK_NUM (1<<16)
#define MAX_SORT_BLOCK_SIZE (1<<13)
#define SORT_BLOCK_HEAD(i) ((i)<<13)
#define FIND_CHK_SIZE (1<<13)
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

	std::string file_name = path_name + "00000.bin";
	const int buf_size = 1 << 18;//4*2^18=2^20B=1MB
	unsigned int *buf = new unsigned int[buf_size];
	std::ranlux48 mt_rd(seed);
	std::ofstream fout;
	int file_name_len = file_name.size();
	for (int i = 0; i < file_num; i++)
	{
		file_name[file_name_len - 5] = i % 10 + '0';
		file_name[file_name_len - 6] = i / 10 % 10 + '0';
		file_name[file_name_len - 7] = i / 10 % 10 + '0';
		file_name[file_name_len - 8] = i / 10 % 10 + '0';
		printf("generating %d\n",file_name);
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

int ins(unsigned __int64 data, unsigned __int32 *sort_array, unsigned __int32 *sort_tail)
{
	unsigned __int32 dat1 = data >> 32ll, dat2 = data & 0xffffffff;
	unsigned __int32 p = SORT_BLOCK_HEAD(dat1) + sort_tail[dat1];

	sort_array[SORT_BLOCK_HEAD(dat1) + sort_tail[dat1]] = dat2;
	sort_tail[dat1]++;
	if (sort_tail[dat1] == MAX_SORT_BLOCK_SIZE)
	{
		printf("发现过多相似数据\n");
		G_flag = 1;
		return 1;
	}
	return 0;
}

void sort_in(int u, unsigned __int32 *sort_array, unsigned __int32 *sort_tail, std::vector<unsigned __int64> &save)
{
	int find_rep48_count = 0;
	for (int i = 0; i < MAX_SORT_BLOCK_NUM; i++)
	{
		std::sort(sort_array + SORT_BLOCK_HEAD(i), sort_array + SORT_BLOCK_HEAD(i) + sort_tail[i]);
		for (int j = SORT_BLOCK_HEAD(i) + 1; j < SORT_BLOCK_HEAD(i) + sort_tail[i]; j++)
		{
			if (sort_array[j - 1] == sort_array[j])
			{
				find_rep48_count++;
				save.push_back((((unsigned __int64)i) << 40ll) + (((unsigned __int64)(sort_array[j])) << 8) + u);
				//去掉可能重复的数据，之记录一次
				while ((j + 1 < SORT_BLOCK_HEAD(i) + sort_tail[i]) && sort_array[j] == sort_array[j + 1])
					j++;
			}
		}
	}
	printf("find48 in %d %d\n", u, find_rep48_count);
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

int repeat_search(int file_num, char** path_name)
{
	LARGE_INTEGER t1, t2, tc;//计时工具
	QueryPerformanceFrequency(&tc);
	QueryPerformanceCounter(&t1);//计时开始
	//计算总字节数
	unsigned __int64 total_size = 0;
	unsigned __int32 total_size_mb;
	{
		std::ifstream fin;
		for (int i = 0; i < file_num; i++)
		{
			fin.open(path_name[i], std::ios::in | std::ios::binary);
			fin.seekg(0, std::ios::end);
			total_size += fin.tellg();
			fin.close();
		}
		total_size_mb = total_size >> 20;
		if (total_size % (1 << 20) != 0)
		{
			printf("警告，数据大小不是MB的整数倍，少于MB的数据末尾将被丢弃\n");
			printf("警告，数据大小不是MB的整数倍，少于MB的数据末尾将被丢弃\n");
		}
	}
	std::vector <unsigned __int64> rep56_list1, rep56_list2, rep56_list3, rep56_list4;
	//找到所有重复出现的56比特
	{
		unsigned __int8 cmp_head[1024];
		unsigned __int8 cmp_tail[1024];
		unsigned __int32 *sort_array1, *sort_array2, *sort_array3, *sort_array4;
		unsigned __int32 *sort_tail1, *sort_tail2, *sort_tail3, *sort_tail4;
		sort_array1 = new unsigned __int32[1 << 29];
		sort_tail1 = new unsigned __int32[1 << 16];
		sort_array2 = new unsigned __int32[1 << 29];
		sort_tail2 = new unsigned __int32[1 << 16];
		sort_array3 = new unsigned __int32[1 << 29];
		sort_tail3 = new unsigned __int32[1 << 16];
		sort_array4 = new unsigned __int32[1 << 29];
		sort_tail4 = new unsigned __int32[1 << 16];
		char *buf;
		read_file F(file_num, path_name);
		for (int i = 0; i < 256; i += 4)
		{
			for (int th = 0; th < 4; th++)
			{
				for (int j = 0; j < 256; j++)
				{
					cmp_head[j + (th << 8)] = 0;
					cmp_tail[j + (th << 8)] = 0;
					unsigned __int32 x, y;
					for (x = j, y = i + th; x != y && cmp_head[j + (th << 8)] < 8; cmp_head[j + (th << 8)]++)
					{
						x = (x & ~(1 << (7 - cmp_head[j + (th << 8)]))) & 0xff;
						y >>= 1;
						if (x == y)
							break;
					}
					for (x = i + th, y = j; (cmp_tail[j + (th << 8)] == 0) || (x != y && cmp_tail[j + (th << 8)] < 8); cmp_tail[j + (th << 8)]++)
					{
						x = (x & ~(1 << (7 - cmp_tail[j + (th << 8)]))) & 0xff;
						y >>= 1;
						if (x == y && (cmp_tail[j + (th << 8)] != 0))
							break;
					}
					cmp_tail[j + (th << 8)] = 8 - cmp_tail[j + (th << 8)];
				}
			}
			memset(sort_tail1, 0, 1 << 18);
			memset(sort_tail2, 0, 1 << 18);
			memset(sort_tail3, 0, 1 << 18);
			memset(sort_tail4, 0, 1 << 18);
			F.start();
			unsigned __int64 count1 = 0;
			unsigned __int64 count2 = 0;
			unsigned __int64 count3 = 0;
			unsigned __int64 count4 = 0;
			unsigned __int64 save_8bytes = 0;
			unsigned __int8 input_byte = 0;
			for (int j = 0; j < total_size_mb; j++)
			{
				buf = F.read();
				unsigned __int8 *cph1 = cmp_head;
				unsigned __int8 *cpt1 = cmp_tail;
				unsigned __int8 *cph2 = cmp_head + 256;
				unsigned __int8 *cpt2 = cmp_tail + 256;
				unsigned __int8 *cph3 = cmp_head + 512;
				unsigned __int8 *cpt3 = cmp_tail + 512;
				unsigned __int8 *cph4 = cmp_head + 768;
				unsigned __int8 *cpt4 = cmp_tail + 768;
				#pragma omp parallel sections num_threads(4)
				{
					#pragma omp section
					{
						unsigned __int64 save_8bytes1 = save_8bytes;
						unsigned __int8 input_byte1 = input_byte;
						for (int k = 0; k < (1 << 20); k++)
						{
							input_byte1 = buf[k];
							for (int l = cph1[save_8bytes1 & 0xff]; l <= cpt1[input_byte1]; l++)
							{
								if ((0xff & (((save_8bytes1 & 0xff) << l)) + (input_byte1 >> (8 - l))) == i)
								{
									ins(((save_8bytes1 << l) >> 8) & 0xffffffffffffll, sort_array1, sort_tail1);
									count1++;
								}
							}
							save_8bytes1 = (save_8bytes1 << 8) + input_byte1;
						}
					}
					#pragma omp section
					{
						unsigned __int64 save_8bytes2 = save_8bytes;
						unsigned __int8 input_byte2 = input_byte;
						for (int k = 0; k < (1 << 20); k++)
						{
							input_byte2 = buf[k];
							for (int l = cph2[save_8bytes2 & 0xff]; l <= cpt2[input_byte2]; l++)
							{
								if ((0xff & (((save_8bytes2 & 0xff) << l)) + (input_byte2 >> (8 - l))) == i + 1)
								{
									ins(((save_8bytes2 << l) >> 8) & 0xffffffffffffll, sort_array2, sort_tail2);
									count2++;
								}
							}
							save_8bytes2 = (save_8bytes2 << 8) + input_byte2;
						}
					}
					#pragma omp section
					{
						unsigned __int64 save_8bytes3 = save_8bytes;
						unsigned __int8 input_byte3 = input_byte;
						for (int k = 0; k < (1 << 20); k++)
						{
							input_byte3 = buf[k];
							for (int l = cph3[save_8bytes3 & 0xff]; l <= cpt3[input_byte3]; l++)
							{
								if ((0xff & (((save_8bytes3 & 0xff) << l)) + (input_byte3 >> (8 - l))) == i + 2)
								{
									ins(((save_8bytes3 << l) >> 8) & 0xffffffffffffll, sort_array3, sort_tail3);
									count3++;
								}
							}
							save_8bytes3 = (save_8bytes3 << 8) + input_byte3;
						}
					}
					#pragma omp section
					{
						unsigned __int64 save_8bytes4 = save_8bytes;
						unsigned __int8 input_byte4 = input_byte;
						for (int k = 0; k < (1 << 20); k++)
						{
							input_byte4 = buf[k];
							for (int l = cph4[save_8bytes4 & 0xff]; l <= cpt4[input_byte4]; l++)
							{
								if ((0xff & (((save_8bytes4 & 0xff) << l)) + (input_byte4 >> (8 - l))) == i + 3)
								{
									ins(((save_8bytes4 << l) >> 8) & 0xffffffffffffll, sort_array4, sort_tail4);
									count4++;
								}
							}
							save_8bytes4 = (save_8bytes4 << 8) + input_byte4;
						}
						save_8bytes = save_8bytes4;
						input_byte = input_byte4;
					}
				}
				F.free();
				if (G_flag)
				{
					printf("相似的数据太多，超出了理论预期，建议检查数据的均匀性！\n");
					return excessive_samedata;
				}
				//printf("%d\n", j);
			}
			printf("%d %d %d %d\n", count1, count2, count3, count4);
			F.end();
			#pragma omp parallel sections
			{
			#pragma omp section
				{
					sort_in(i, sort_array1, sort_tail1, rep56_list1);
				}
				#pragma omp section
				{
					sort_in(i + 1, sort_array2, sort_tail2, rep56_list2);
				}
				#pragma omp section
				{
					sort_in(i + 2, sort_array3, sort_tail3, rep56_list3);
				}
				#pragma omp section
				{
					sort_in(i + 3, sort_array4, sort_tail4, rep56_list4);
				}
			}
		}
		free(sort_array1);
		free(sort_array2);
		free(sort_array3);
		free(sort_array4);
		free(sort_tail1);
		free(sort_tail2);
		free(sort_tail3);
		free(sort_tail4);
		for (int i = 0; i < rep56_list1.size(); i++)
		{
			print_binary(rep56_list1[i], 16); putchar('\n');
		}
		for (int i = 0; i < rep56_list2.size(); i++)
		{
			print_binary(rep56_list2[i], 16); putchar('\n');
		}
		for (int i = 0; i < rep56_list3.size(); i++)
		{
			print_binary(rep56_list3[i], 16); putchar('\n');
		}
		for (int i = 0; i < rep56_list4.size(); i++)
		{
			print_binary(rep56_list4[i], 16); putchar('\n');
		}
	}
	//将所有重复的56比特放入查找表，记录所有16位相同的56比特的开始结束位置
	unsigned __int64 *rep56_save = new unsigned __int64[1 << 16];
	unsigned __int16 *st16 = new unsigned __int16[1 << 16];
	unsigned __int16 *ed16 = new unsigned __int16[1 << 16];
	{
		int save_n = 0, j = 0;
		for (int i = 0; i < rep56_list1.size(); i++)
		{
			rep56_save[save_n++] = rep56_list1[i];
		}
		for (int i = 0; i < rep56_list2.size(); i++)
		{
			rep56_save[save_n++] = rep56_list2[i];
		}
		for (int i = 0; i < rep56_list3.size(); i++)
		{
			rep56_save[save_n++] = rep56_list3[i];
		}
		for (int i = 0; i < rep56_list4.size(); i++)
		{
			rep56_save[save_n++] = rep56_list4[i];
		}
		std::sort(rep56_save, rep56_save + save_n, cmp16);
		for (int i = 0; i < save_n; i++)
		{
			printf("%d ", i);
			print_binary(rep56_save[i], 16);
			printf("\n");
		}
		if (save_n >= 65536)
		{
			printf("重复的56比特数量超过理论预期，请注意数据是否有大段重复\n");
			printf("后面的计算结果将仅输出前65535个重复的56比特\n");
			save_n = 65535;
			G_flag = excessive_rep56;
		}
		for (int i = 0; i < save_n; i++)
		{
			int s = rep56_save[i] & 0xffff;
			for (int k = j; k < s; k++)
			{
				st16[k] = 0;
				ed16[k] = 0;
			}
			st16[s] = i;
			ed16[s] = i + 1;
			for (; i + 1 < save_n; i++)
			{
				if (s != (rep56_save[i + 1] & 0xffff))
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
		for (int i = 0; i < 65536; i++)
		{
			if (st16[i] != 0 || ed16[i] != 0)
			{
				print_binary(i, 4);
				printf(" %d %d\n", st16[i], ed16[i]);
			}
		}
	}
	//找出所有重复56比特对应位置的的80比特
	std::vector<repeat_bits> rb_list;
	{
		read_file F(file_num, path_name);
		unsigned char *buf;
		F.start();
		unsigned __int64 f0_save_8bytes = 0;
		unsigned __int8 f0_remain_byte = 0;
		unsigned __int8 least_byte = 0;
		omp_set_num_threads(8);
		for (int j = 0; j < total_size_mb; j++)
		{
			buf = (unsigned char *)F.read();
			#pragma omp parallel for
			for (int l = 0; l <= 7; l++)
			{
				unsigned __int64 save_8bytes = (f0_save_8bytes << l) + (least_byte >> (8 - l));
				unsigned __int8 remain_byte = (f0_remain_byte << l) + ((f0_save_8bytes>>56) >> (8 - l));
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
							if (check56 == rep56_save[i])
							{
								repeat_bits tmp;
								tmp.l = l;
								tmp.offset = k + (j << 20);
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
			if (rb_list.size()> 1<<20)
			{
				printf("找到过多重复串，");
				G_flag = excessive_repstring;
				break;
			}
		}
		free(rep56_save);
		free(st16);
		free(ed16);
	}
	//比较80比特数据，输出结果
	{
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
			if (same_bit >= 64)
			{
				printf("位置: ");
				print_binary(rb_list[i].offset - 10, 16);
				printf("-");
				print_binary(rb_list[i].offset, 16);
				printf("偏移: %d\n", rb_list[i].l);
				printf("\n");
				print_binary(rb_list[i].s2, 4);
				print_binary(rb_list[i].s1, 16);

				printf("\n位置: ");
				print_binary(rb_list[i - 1].offset - 10, 16);
				printf("-");
				print_binary(rb_list[i - 1].offset, 16);
				printf("偏移: %d\n", rb_list[i - 1].l);
				printf("\n");
				print_binary(rb_list[i - 1].s2, 4);
				print_binary(rb_list[i - 1].s1, 16);

				printf("\n重复长度%d\n\n", same_bit);
				
			}
		}
		printf("汇总:");
		for (int i = 0; i < 24; i++)
		{
			printf("%4d:%4d\n", i + 56, count[i] - count[i + 1]);
		}
	}
	QueryPerformanceCounter(&t2);
	double time = (double)(t2.QuadPart - t1.QuadPart) / (double)tc.QuadPart;
	printf("time = %lf\n", time);
	return G_flag;
}
