#include "lib.h"

extern int do_init(struct objfs_state *objfs)
{
   char *ptr;
   char buf[1024];
   ptr = getcwd(buf, 1024);
   if(!ptr){
       perror("getcwd"); 
       return -1;
   }
   if(stat(ptr, &objfs->def_dirstat)){
       perror("stat"); 
       return -1;
   }

   objfs->blkdev = open("disk.img", O_RDWR |   O_DIRECT);
   if(objfs->blkdev < 0){
       perror("blkdev open"); 
       return -1;
   }

   objfs->logfd = fopen("objfs.log", "w+");
   
   if(objfs->logfd == NULL){
       perror("logfile open"); 
       return -1;
   }
    
   if(stat("objfs.log", &objfs->def_fstat)){
       perror("stat"); 
       return -1;
   }
   setvbuf(objfs->logfd, NULL, _IOLBF, 0);

   objfs->cache = mmap(NULL, CACHE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
   if(objfs->cache == MAP_FAILED){
       perror("mmap");
       return -1;
   }
   return 0;
}

int read_block(struct objfs_state *objfs, long block_offset, char *buf)
{
     unsigned long addr = (unsigned long) buf;
     if(((addr >> BLOCK_SHIFT) << BLOCK_SHIFT) != addr){
          dprintf("Buffer is not at block boundary\n");
          return -1;
     }
    block_offset <<= BLOCK_SHIFT;
    #if 0
    if(lseek(objfs->blkdev, block_offset, SEEK_SET) < 0){
          dprintf("%s: lseek error\n", __func__);
          return -1;
    }    
    if(read(objfs->blkdev, buf, BLOCK_SIZE) < 0){
          dprintf("%s: read error\n", __func__);
          return -1;
    }
    #else
    if(pread(objfs->blkdev, buf, BLOCK_SIZE, block_offset) < 0){
          dprintf("%s: read error\n", __func__);
          return -1;
    }
    #endif
    return 0;
}
int write_block(struct objfs_state *objfs, long block_offset, char *buf)
{
     unsigned long addr = (unsigned long) buf;
     if(((addr >> BLOCK_SHIFT) << BLOCK_SHIFT) != addr){
          dprintf("Buffer is not at block boundary\n");
          return -1;
     }
    block_offset <<= BLOCK_SHIFT;
    #if 0
    if(lseek(objfs->blkdev, block_offset, SEEK_SET) < 0){
          dprintf("%s: lseek error\n", __func__);
          return -1;
    }    
    if(write(objfs->blkdev, buf, BLOCK_SIZE) < 0){
          dprintf("%s: read error\n", __func__);
          return -1;
    }
    #else

  

     if(pwrite(objfs->blkdev, buf, BLOCK_SIZE, block_offset) < 0){
  
          dprintf("%s: read error\n", __func__);
          return -1;
     }
    #endif
    return 0;
}
