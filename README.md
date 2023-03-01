# repeat_bits_search
This program is used to find duplicate string pairs in data.<br>
This program assumes that most of the data are random, so there will be no expected results for some regular data.<br>
This program may help to random test and find that data is copied incorrectly.<br>
The maximum input data of this program should not exceed 10GB.<br>
The program will count all repeated string pairs with a length of more than 56 and give all repeated string pairs with a length of more than 64.<br>
This program uses the thread acceleration of openmp and c++11. <br>

# How to use 
[.exe name] [file_name1 file_name2 ...] [-j thread_num (select in 1,2,4,8)] [-o result_file] [-avx] <br>
sample: Project1.exe 0.bin -j 8 -o out.txt -avx <br>

# Run time
|data size|4thread|4thread avx|8thread|8thread avx|
|----|-----|-----|-----|-----|
|1GB|300s|185s|210s|145s|
|10GB||2306s|2400s|1900s|
