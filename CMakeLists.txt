cmake_minimum_required(VERSION 3.22)
project(3221A3 C)

set(CMAKE_C_STANDARD 11)
set(CFLAGS = -Wall -g $(shell pkg-config fuse --cflags) -std=gnu11)
set(LDLIBS = $(shell pkg-config fuse --libs))

include_directories(PA3.1)

add_executable(3221A3
        PA3.1/ext2.c
        PA3.1/ext2.h
        PA3.1/ext2dir.c
        PA3.1/ext2file.c
        PA3.1/ext2symlink.c
        PA3.1/ext2test.c)
