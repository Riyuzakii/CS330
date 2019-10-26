#include "kvstore.h"

long lookup_key(char *key)
{
   /*XXX We want this on stack*/
   char actual_key[512];
   struct stat sbuf;
   MAKE_KEY(actual_key, key);
   if(stat(actual_key, &sbuf) < 0)
        return -1;
   return sbuf.st_size;    
}

long put_key(char *key, char *val, int size)
{
   char actual_key[512];
   int fd;
   MAKE_KEY(actual_key, key);
   fd = open(actual_key, O_RDWR | O_CREAT | O_EXCL, 0666);
   if(fd < 0){
      perror("open");
      return -1;
   }
  
   if(write(fd, val, size) < 0){
      perror("write");
      return -1;
   }
   close(fd);
   return 0;    
   
}

long get_key(char *key, char *val)
{
   char actual_key[512];
   int fd, size;
   struct stat sbuf;

   MAKE_KEY(actual_key, key);
   fd = open(actual_key, O_RDWR, 0666);
   if(fd < 0){
      perror("open");
      return -1;
   }
   
   if(fstat(fd, &sbuf) < 0){
      perror("stat");
      exit(-1);
   }
  
   size = sbuf.st_size;
   if(size < 4096)
      size = 4096;

   if(read(fd, val, size) < 0){
          perror("read");
          return -1;
      }
   close(fd);
   return 0;    
   
}

long rename_key(char *old, char *new)
{
   char actual_old[512], actual_new[512];
   MAKE_KEY(actual_old, old);
   MAKE_KEY(actual_new, new);
   return rename(actual_old, actual_new);
}

long delete_key(char *key)
{
   char actual_key[512];
   MAKE_KEY(actual_key, key);
   return unlink(actual_key);
}
