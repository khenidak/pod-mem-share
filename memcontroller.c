#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "common.h"

#define CHUNK 1024 * 4 * 10  // 10 pages
// file we are mapping
const char* source_file_path = "./data.file";

int main (int argc, char *argv[]){
  int source_file_fd;
  int shared_mem_object_fd;
  void* shared_mmap;


  struct stat source_file_statbuf = {0};
  int ret;
  int res;

  // open the source file
  source_file_fd = open (source_file_path, O_RDONLY);
  if(source_file_fd < 0){
    printf("failed to source file %s errno:%d\n", source_file_path, errno);
    ret = -1;
    goto done;
  }

  printf("opened:%s\n", source_file_path);
  // state it to get the size of the
  // shared map
  res = fstat(source_file_fd, &source_file_statbuf);
  if(res < 0){
    printf("failed to stat:%s errno:%d \n", source_file_path, errno);
    ret = -1;
    goto clean;
  }
  printf("stated:%s size:%ldmb\n", source_file_path, source_file_statbuf.st_size / (1024* 1024));

  // be nice and tell the kernel what
  // we are going to do with the file
  res = posix_fadvise(source_file_fd, 0, source_file_statbuf.st_size - 1, POSIX_FADV_NOREUSE);
  if(res != 0){
    ret = -1;
    printf("failed to fadvise %s errno:%d\n", source_file_path, res);
    goto clean;
  }
  printf("advised:%s\n", source_file_path);

  // create the shared map object
  // read and write..
  shared_mem_object_fd = shm_open(shared_mem_path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (shared_mem_object_fd < 0 ){
     ret = -1;
     printf("failed to create shared mem object %s errno:%d\n", shared_mem_path, errno);
     goto clean;
  }
  res = ftruncate(shared_mem_object_fd, (size_t) source_file_statbuf.st_size);
  if(res < 0){
    ret = -1;
     printf("failed to truncate mem object:%s to:%ld errno:%d\n", shared_mem_path, source_file_statbuf.st_size, errno);
     goto clean;
  }

  printf("created shared mem object:%s\n", shared_mem_path);

  // technically we don't need MAP_LOCKED here if we
  // are to munmap(2) once we load the file into it.
  shared_mmap = mmap(NULL, (size_t) source_file_statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, shared_mem_object_fd, 0);
  if (shared_mmap == MAP_FAILED){
    ret = -1;
    printf("failed to create shared mem map errno:%d", errno);
    goto clean;
  }

  printf("created mem map based on the shared mem segment at:%p size:%ldmb \n",
          shared_mmap, source_file_statbuf.st_size / (1024* 1024));

  // in a typical prod scenario you will check if the mem was
  // previously written correctly previously before writing to it
  // read file into mapped mem
  char* write_at = (char*) shared_mmap;
  size_t remaining = ((size_t) source_file_statbuf.st_size - 1);
  int ops = 0;
  size_t to_read;

  while(remaining > 0){
    ops++;
    // read at step if we can or whatever remaining
    to_read = CHUNK > remaining ? remaining : CHUNK;
    printf("loading: %ld from remaining:%ld\n", to_read, remaining);
    // avoid copying things around and write directly
    // to the mapped mem.
    ssize_t done_read = read(source_file_fd, write_at, to_read);
    if(done_read < 0){
       ret = -1;
       printf("failed to read from file into mmap errno:%d\n", errno);
       goto clean;
    }
    remaining = remaining - (size_t) done_read;
    write_at = write_at + (size_t) done_read;
  }

  printf("read the file using %d read ops\n", ops);

  // close the file. this allows kernel to flush
  // any cached read pages, you can get fancy
  // and use O_DIRCT hints, or go as far as
  // DAX style FS/blkdev but that can also
  // be achieved by the hint we have in fadvise
  // and quickly closing after we are done.
  close(source_file_fd);
  source_file_fd = -1;

  // you can also close the map and shared object(do not unlink it)
  // note: the map takes no mem (it is mapped from the shared object)
  //       but it does have some resources allocated it in kernel, though
  //       minimal
  //
  // note 2: the shared object is two parts the one that lives outside
  //         this process and that is controlled by xx_unlink(3) func
  //         and the fd which lives in this process, closing the fd
  //         has no impact on the shared mem object/segment. IOW
  //         it has a lifecycle outside the process lifecycle
  printf("done, waiting\n");
  while(1){
    printf("sleeping..\n");
    sleep(5);
  }

clean:
  if(source_file_fd > 0) close(source_file_fd);
  if(shared_mem_object_fd) close(shared_mem_object_fd); // that won't unlink it
  // mapped me will drop off as the process exists, no need to close anything
done:
  return ret;
}
