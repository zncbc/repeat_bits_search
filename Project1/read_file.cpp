#include "read_file.h"
using std::thread;
using std::cout;


read_file::read_file(int n, char** path)
{
	file_n = n;
	path_name = path;
	buf = new char[MAX_ROUND * READ_BLOCK_SIZE];
	is_start = false;
	total_size = 0;
	for (int i = 0; i < n; i++)
	{
		fin.open(path_name[file_i], std::ios::binary | std::ios::in);
		fin.seekg(0, std::ios::end);
		total_size += fin.tellg();
		fin.seekg(0, std::ios::beg);
		fin.close();
	}
}

int read_file::start()
{
	if (is_start == true)
	{
		return ERR_REP_STR;
	}
	file_i = 0;
	fin.open(path_name[file_i], std::ios::binary | std::ios::in);
	fin.seekg(0, std::ios::end);
	file_size = fin.tellg();
	fin.seekg(0, std::ios::beg);
	data_block_i = 0;
	is_start = true;
	data_block_e = 0;
	thread T(&read_file::read_thread, this);
	T.detach();
	//SLEEP(5);
}

int read_file::end()
{
	is_start = false;
	return 0;
}

int read_file::reset()
{
	end();
	SLEEP(5);
	start();
	return 0;
}
bool read_file::read_if()
{
	return (data_block_e & (1 << data_block_i)) != 0;
}

__int64 read_file::get_total_size()
{
	return total_size;
}

__int32 read_file::get_block_num()
{
	return total_size / READ_BLOCK_SIZE;
}
char* read_file::read()
{
	std::unique_lock<std::mutex> lk(data_flag[data_block_i]);
	data_rd.wait(lk, [this] {return (data_block_e & (1 << data_block_i)) != 0; });
	//printf("read %d\n", data_block_i);
	return buf + data_block_i * READ_BLOCK_SIZE;
}
void read_file::free()
{
	std::unique_lock<std::mutex> lk(data_flag[data_block_i]);
	data_block_e &= ~(1 << data_block_i);
	//printf("free %d\n", data_block_i);
	data_et.notify_all();
	data_block_i = (data_block_i + 1) % MAX_ROUND;
}

int read_file::read_thread()
{
	int need_read_block_i = 0;
	while (is_start)
	{
		{
			std::unique_lock<std::mutex> lk(data_flag[need_read_block_i]);
			//printf("write %d", need_read_block_i);
			if ((data_block_e & (1 << need_read_block_i)) == 0)
			{
				if (fin.tellg() == file_size)
				{
					file_i++;
					fin.close();
					if (file_i == file_n)
					{
						goto FILE_END;
					}
					fin.open(path_name[file_i], std::ios::binary | std::ios::in);
				}
				//printf("ses %d\n", need_read_block_i);
				fin.read(buf + need_read_block_i * READ_BLOCK_SIZE, READ_BLOCK_SIZE);
				data_block_e |= (1 << need_read_block_i);
				data_rd.notify_all();
				need_read_block_i = (need_read_block_i + 1) % MAX_ROUND;
			}
			else
			{
				//printf("wait %d\n", need_read_block_i);
				data_et.wait(lk, [this, need_read_block_i] {return (data_block_e & (1 << need_read_block_i)) == 0; });
			}
		}
	}
FILE_END:
	return 0;
}