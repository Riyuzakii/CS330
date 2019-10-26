#ifndef __LIB_H_
#define __LIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#ifdef DBG
     #define dprintf(str, ...) fprintf(objfs->logfd, str, ##__VA_ARGS__)
#else
     #define dprintf(str, ...)
#endif

#define BLOCK_SIZE 4096
#define BLOCK_SHIFT 12
#define CACHE_SIZE 128*1024*1024

struct objfs_state{
    int blkdev;
    FILE *logfd;
    struct stat def_fstat;
    struct stat def_dirstat;
    char *cache;
    int cachesize;              
    void *objstore_data;
    long disksize;  //# of blocks
}; 

extern int do_init(struct objfs_state *objfs);
extern int read_block(struct objfs_state *objfs, long block_offset, char *buf);
extern int write_block(struct objfs_state *objfs, long block_offset, char *buf);

/*Object interface*/
extern int  objstore_init(struct objfs_state *objfs);   
extern int  objstore_destroy(struct objfs_state *objfs);   
extern long find_object_id(const char *, struct objfs_state *);
extern long create_object(const char *, struct objfs_state *);
extern long release_object(int objid, struct objfs_state *objfs);
extern long destroy_object(const char *, struct objfs_state *);
extern long rename_object(const char *, const char *, struct objfs_state *);
extern long objstore_write(int objid, const char *, int, struct objfs_state *, off_t offset);
extern long objstore_read(int objid, char *, int, struct objfs_state *, off_t offset);
extern int  fillup_size_details(struct stat *buf, struct objfs_state *);

#endif
