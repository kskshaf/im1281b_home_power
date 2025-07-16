#!/usr/bin/bash
cc -Wall -Wextra -O3 -static -o im1281b_home *.c -I/usr/local/include -L/usr/local/lib -lserialport -lpthread -lm
