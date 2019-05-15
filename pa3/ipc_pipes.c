#include <stdio.h>              ////for printf
#include <stdlib.h>             ////for malloc
#include <unistd.h>             ////for pipe
#include <fcntl.h>

#include "ipc_pipes.h"

#define FD_SIZE 2

/**
 * Prepare array before filling it with descriptors. Allocate memory for structures' array
 * @param init_info     Pointer to structure with descriptors' array we need to allocate memory
 */
void prepare_decriptors_array(InitInfo* init_info)
{
    for (int i = 0; i < init_info->processes_count; ++i)
    {
        for (int j = 0; j < init_info->processes_count; ++j)
        {
            if (i != j)                         ////because we do not need descriptor to itself
            {
                init_info->descriptors[i][j] = (ConnectionInfo*)malloc(sizeof(ConnectionInfo));
            }
        }
    }
}

/**
 * Open n*(n-1) pipes. Write info about it to file pipes.log
 * @param init_info     Pointer to structure with all needed info
 */
void open_pipes(InitInfo* init_info)
{
    int fd[FD_SIZE];                                                 ////here we will put read and write descriptors
    for (int i = 0; i < init_info->processes_count; ++i)
    {
        for (int j = 0; j < init_info->processes_count; ++j)
        {
            if (i != j)
            {
                int return_status = pipe(fd);
                if (return_status < 0)
                {
                    perror("Can not open pipe");
                    return;
                }
                fcntl(fd[0], F_SETFL, O_NONBLOCK);
                fcntl(fd[1], F_SETFL, O_NONBLOCK);
                //// now in fd we have descriptors for pipe. in fd[0] we have read end, in f[1] we have write end
                init_info->descriptors[j][i]->read_fd = fd[0];
                init_info->descriptors[i][j]->write_fd = fd[1];

                write_info_to_pipes_log_file(open_pipe, init_info->process_id, i, j,
                        init_info->descriptors[i][j]->write_fd);
//                printf("write %d %d fd = %d\n", i, j, init_info->descriptors[i][j]->write_fd);
                write_info_to_pipes_log_file(open_pipe, init_info->process_id, j, i,
                        init_info->descriptors[j][i]->read_fd);
            }
        }
//    printf("Leave open_pipes\n");
    }
}

/**
 * Function for closing unnessesary pipes. There are pipes which are not connected to current process.
 * @param init_info     Pointer to structure with all needed info
 */
void close_some_pipes(InitInfo* init_info)
{
//    printf("Leave close_some_pipes\n");

    for (int j = 0; j < init_info->processes_count; ++j)
    {
        if (j != init_info->process_id)                 //// if j == process_id then pipe is needed and connected to current process
        {
            for (int k = 0; k < init_info->processes_count; ++k)
            {
                if (j != k)                             //// because here we do not have anything
                {
                    write_info_to_pipes_log_file(close_pipe, init_info->process_id, j, k, init_info->descriptors[j][k]->write_fd);
                    write_info_to_pipes_log_file(close_pipe, init_info->process_id, j, k, init_info->descriptors[j][k]->read_fd);

                    close(init_info->descriptors[j][k]->read_fd);
                    close(init_info->descriptors[j][k]->write_fd);
                    free(init_info->descriptors[j][k]);
                }

            }
        }
    }
//    printf("Leave close_some_pipes\n");
}

/**
 * Function for closing pipes connected to this process. Call after process did useful job.
 * @param init_info     Pointer to structure with all needed info
 */
void close_its_pipes(InitInfo* init_info)
{
    for (local_id i = 0; i < init_info->processes_count; ++i)
    {
        if (i != init_info->process_id)                 ////to avoid situations like [i][i]. If proc_id == 2, we will remove [2][1] and [2][0]
        {
            write_info_to_pipes_log_file(close_pipe, init_info->process_id, init_info->process_id, i,
                                         init_info->descriptors[init_info->process_id][i]->write_fd);
            write_info_to_pipes_log_file(close_pipe, init_info->process_id, init_info->process_id, i,
                                         init_info->descriptors[init_info->process_id][i]->read_fd);

            close(init_info->descriptors[init_info->process_id][i]->read_fd);
            close(init_info->descriptors[init_info->process_id][i]->write_fd);
            free(init_info->descriptors[init_info->process_id][i]);
        }
    }
//    printf("Leave close_its_pipes\n");
}
