#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "common.h"

#define CHUNK 1024*1024 //1mb

int main (int argc, char *argv[]){
  int shared_mem_object_fd;
  void* shared_mmap = NULL;
  int ret = 0;
  int res = 0;

  printf("consumer starting..\n");
   struct stat shared_mem_object_statbuf = {0};

  // this will fail if the mem controller didn't start first
  // and create the shared mem object. one way around that
  // is back-off style waiting for controlelr to start.
  shared_mem_object_fd = shm_open(shared_mem_path, O_RDONLY, S_IRUSR);
  if(shared_mem_object_fd < 0 ){
     ret = -1;
     printf("failed to create shared mem object %s errno:%d\n", shared_mem_path, errno);
     goto clean;
  }

  res = fstat(shared_mem_object_fd, &shared_mem_object_statbuf);
  if(res < 0){
    printf("failed to stat shared mem object:%s errno:%d \n", shared_mem_path, errno);
    ret = -1;
    goto clean;
  }

  printf("stated:%s size:%ldmb\n", shared_mem_path, shared_mem_object_statbuf.st_size / (1024* 1024));


  shared_mmap = mmap(NULL, shared_mem_object_statbuf.st_size, PROT_READ, MAP_SHARED | MAP_LOCKED, shared_mem_object_fd, 0);
  if (shared_mmap == MAP_FAILED){
    ret = -1;
    printf("failed to create shared mem map errno:%d", errno);
    goto clean;
  }

  printf("created mem map based on the shared mem segment %p\n", shared_mmap);

  printf("reading...\n");

  // read char by char. for the fun of it
  // none of the below calls should do any IO
  // against any disk this box has
  while(1){
    char* read_at = (char *) shared_mmap;
    char* read_end = read_at + (shared_mem_object_statbuf.st_size - 1);
    while(read_at <= read_end){
      printf("value at:%p is %d. sleeping 1s\n", read_at, *read_at);
      read_at++;
      sleep(1);
    }
  }


clean:
  if(shared_mem_object_fd > 0 ) close(shared_mem_object_fd);

  return ret;
}
