cmake_minimum_required(VERSION 3.9)
project(raspr C)

set(CMAKE_C_STANDARD 99)
add_executable(raspr common.h ipc.c ipc_worker.c ipc_worker.h ipc.h ipc_structs.h
        pa23.c banking.h bank_robbery.c pa2345.h)
