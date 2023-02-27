#pragma warning(disable:4996)

#include <iostream>
#include <algorithm>
#include <random>
#include <windows.h>
#include "repeat_test.h"
#include "read_file.h"
#include "rep56.h"

void help()
{
	printf("usage: rep_test [file_name1 file_name2 ...] [-j thread_num (select in 1,2,4,8)] [-o result_file] [-avx]");
	printf("default parameters: -j 1 -o result.txt");
	system("pause");
}

int para_resolve(int args, char **argv, char *result_file,int& thread_num, int& file_num ,bool &avx)
{
	if (args == 1)
	{
		help();
		return 0;
	}
	file_num = 0;
	thread_num = 1;
	result_file[0] = '\0';
	avx = false;
	printf("find file:\n");
	const char *defualt_out_file = "result.txt";
	int i = 1;
	for (i = 1; i < args; i++)
	{
		if (argv[i][0] == '-')
			break;
		file_num++;
		printf("%s\n", argv[i]);
	}
	for (i = 1; i < args; i++)
	{
		if (strlen(argv[i]) == 2 && argv[i][0] == '-' && argv[i][1]=='j')
		{
			thread_num = argv[i + 1][0] - '0';
			thread_num += ((argv[i + 1][1] == '\0') ? 0 : (thread_num * 9 + argv[i + 1][1] - '0'));
		}
		if (strlen(argv[i]) == 2 && argv[i][0] == '-' && argv[i][1] == 'o')
		{
			memcpy(result_file, argv[i + 1], strlen(argv[i + 1]));
		}
		if (strlen(argv[i]) == 4 && argv[i][0] == '-' && argv[i][1] == 'a' && argv[i][2] == 'v' && argv[i][3] == 'x')
		{
			avx = true;
		}
	}
	if (result_file[0] == '\0')
	{
		memcpy(result_file, defualt_out_file, 11);
	}
	if (thread_num >= 8)
		thread_num = 8;
	else if (thread_num >= 4)
		thread_num = 4;
	else if (thread_num >= 2)
		thread_num = 2;
	else
		thread_num = 1;
	read_file F(file_num, &argv[1]);
	printf("total size :%lld\n", F.get_total_size());
	printf("thread num :%d\n", thread_num);
	printf("result file path :%s\n", result_file);
	printf("use avx :%c\n", avx ? 'Y' : 'N');
	printf("start test? [Y/N]");
	char c;
	scanf("%c", &c);
	if (c == 'y' || c == 'Y')
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int main(int args, char **argv)
{
	int thread_num, file_num;
	char result_file[50];
	bool avx;
	if (para_resolve(args, argv, result_file, thread_num, file_num,avx))
	{
		repeat_search(file_num, argv + 1, result_file, thread_num, avx);
	}
}