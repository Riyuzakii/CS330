/*
     This is a template code to implement a persistent object store as part of an 
     an assignment in the OS course (CS330) at IIT Kanpur
*/

#include "lib.h"
#define ROOT_OBJID 1
    
static struct objfs_state *objfs;



/* Refer /usr/include/fuse/fuse.h for expanations*/

/** Get file attributes.
 *
 * Similar to stat().  
 * Relevant fields to be filled up are as follows
 * - st_ino: is the unique object ID
 * - st_size: object size in bytes 
 */
int objfs_getattr(const char *key, struct stat *statbuf)
{
    int retval;
    dprintf("%s: key=%s\n", __func__, key);
    
    if(!strcmp(key, "/")){
           *statbuf = objfs->def_dirstat; 
           statbuf->st_ino = ROOT_OBJID;
    }else{
           *statbuf = objfs->def_fstat;
           retval = find_object_id(key+1, objfs);
           if(retval < 0)
                 return -ENOENT;
           statbuf->st_ino = retval;
           if(fillup_size_details(statbuf) < 0)
                return -EBADF;
    }
    statbuf->st_uid = getuid();
    statbuf->st_gid = getgid(); 
    return 0;
}


/** Remove the object with key = key*/
int objfs_unlink(const char *key)
{
    dprintf("%s: key=%s\n", __func__, key);
    destroy_object(key+1, objfs);
    return 0;
}


/** Update the key string*/
int objfs_rename_key(const char *oldkey, const char *newkey)
{
    dprintf("%s: oldkey=%s\n", __func__, oldkey);
    rename_object(oldkey, newkey, objfs); 
    return -EINVAL;
}

/*
   Create and open an handle for the object
*/
int objfs_create (const char *key, mode_t mode, struct fuse_file_info *fi)
{
    long retval;
    dprintf("%s: key=%s\n", __func__, key);
    retval = create_object(key+1, objfs);
    if(retval < 0)
          return -EEXIST;
    fi->fh = retval;
    return 0;
}

/* 
 * Open a object with key = key. Fill up the object handle in fi->fh.
 * Read/Write/Release will come with the same fi->fh
 */

int objfs_open(const char *key, struct fuse_file_info *fi)
{
    int retval;
    dprintf("%s: key=%s\n", __func__, key);
    retval = find_object_id(key+1, objfs);
    if(retval < 0){
           return -ENOENT;
    }
    fi->fh = retval;
    return 0;
}

/*
     Read data from an open object
 */
int objfs_read(const char *key, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retval;
    dprintf("%s: key=%s fh=%ld\n", __func__, key, fi->fh);
    retval = objstore_read(fi->fh, buf, size, objfs);
    if(retval < 0)
         return -EINVAL;
    return retval;
}

/** 
   Write data to an object
 */
int objfs_write(const char *key, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retval;
    dprintf("%s: key=%s fh=%ld\n", __func__, key, fi->fh);
    retval = objstore_write(fi->fh, buf, size, objfs);
    if(retval < 0)
         return -EINVAL;
    return retval;
}


/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int objfs_release(const char *key, struct fuse_file_info *fi)
{
    dprintf("%s: key=%s\n", __func__, key);
    release_object(fi->fh, objfs);
    return -EINVAL;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
// Undocumented but extraordinarily useful fact:  the fuse_context is
// set up before this function is called, and
// fuse_get_context()->private_data returns the user_data passed to
// fuse_main().  Really seems like either it should be a third
// parameter coming in here, or else the fact should be documented
// (and this might as well return void, as it did in older versions of
// FUSE).
void *objfs_init(struct fuse_conn_info *conn)
{
   dprintf("%s\n", __func__);
   objstore_init(objfs); 
   return objfs;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void objfs_destroy(void *userdata)
{
    dprintf("%s\n", __func__);
    objstore_destroy(objfs); 
    fclose(objfs->logfd);
    close(objfs->blkdev);
    munmap(objfs->cache, CACHE_SIZE);
    free(objfs);
}

#if 0
/*
   Do not change the following code. Note that objid = 0 is the
   root directory. Don't use it. 
*/	

int objfs_opendir (const char *key, struct fuse_file_info *fi)
{
    dprintf("%s: key=%s\n", __func__, key);
    fi->fh = 0;
    return 0;  
}
int objfs_releasedir(const char *key, struct fuse_file_info *fi)
{
    dprintf("%s: key=%s\n", __func__, key);
    return 0;  
}

int objfs_readdir(const char *key, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    dprintf("%s: key=%s\n", __func__, key);
    if(fi->fh)
          return -ENOTSUP;
    return 0;
}
#endif

static struct fuse_operations objfs_operations = {
  .getattr = objfs_getattr,
  .create = objfs_create,
  .unlink = objfs_unlink,
  .open = objfs_open,
  .read = objfs_read,
  .write = objfs_write,
  .release = objfs_release,
  .init = objfs_init,
  .destroy = objfs_destroy,
//  .opendir = objfs_opendir,
//  .readdir = objfs_readdir,
//  .releasedir = objfs_releasedir,
};


int main(int argc, char *argv[])
{
    int retval;

    printf("Fuse library version %d.%d\n", FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION);
    
    if ((argc < 2)){
	printf("Usage: %s [mount point]\n", argv[0]);
        exit(-1);
    }

    objfs = malloc(sizeof(struct objfs_state));
    if (objfs == NULL) {
	perror("malloc");
	exit(-1);
    }

    if(do_init(objfs) < 0){
	printf("Block device initialization failed!\n");
	exit(-1);
    }
         
    printf("%s\n", __func__);
    retval = fuse_main(argc, argv, &objfs_operations, objfs);
    dprintf("fuse main returned %d\n", retval);

    return retval;
}
