# repeat_bits_search
This program is used to find duplicate string pairs in data.<br>
This program assumes that most of the data are random, so there will be no expected results for some regular data.<br>
This program may help to random test and find that data is copied incorrectly.<br>
The maximum input data of this program should not exceed 10GB.<br>
The program will count all repeated string pairs with a length of more than 56 and give all repeated string pairs with a length of more than 64.<br>
This program uses the thread acceleration of openmp and c++11. It occupies 10GB of memory and up to 8 cores. It takes 70 minutes to process 10GB on my computer.<br>
