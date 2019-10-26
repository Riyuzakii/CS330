#include "lib.h"
#include <pthread.h>
#define MAX_OBJS 1000000

struct object {
    int id;
    int size;
    char key[32];
    int indirect_pointers[4];
};
#define BITMAPS 8388608
#define PAGES 32768
pthread_mutex_t create_object_lock;

struct superBlock {
    pthread_mutex_t superBlock_mutex;
    ushort block_bitmap[BITMAPS];

    char dirty[PAGES];
    struct object obj[MAX_OBJS];

    pthread_mutex_t cache_mutex;
    int cache_bitmap[PAGES];
    int cache_index;
    char dummy_key[424];
};
struct superBlock *sobject;
int total_blocks;
#define DATA_START sizeof(struct superBlock)/BLOCK_SIZE


#define malloc_4k(x) do{\
                         (x) = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);\
                         if((x) == MAP_FAILED)\
                              (x)=NULL;\
                     }while(0); 
#define free_4k(x) munmap((x), BLOCK_SIZE)
#define malloc_nk(x, n) do{\
        (x) = mmap(NULL, (n)*BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); \
        if((x) == MAP_FAILED)\
            (x)=NULL;\
    }while(0);
#define free_nk(x, n) munmap((x), (n)*BLOCK_SIZE)

#define malloc_superBlock(x) do{\
        (x) = mmap(NULL, sizeof(struct superBlock), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); \
        if((x) == MAP_FAILED)\
            (x)=NULL;\
    }while(0);
#define free_superBlock(x) munmap((x), sizeof(struct superBlock))

#define ind2ID(x) ({int ret; ret = x+2; ret;})
#define id2Ind(x) ({int ret; ret = x-2; ret;})

//https://gist.github.com/MohamedTaha98/ccdf734f13299efb73ff0b12f7ce429f
unsigned int hash(const char *str)
{
    unsigned int hash = 5381;
    int c;
    while ((c = *str++) != '\0')
        hash = ((hash << 5) + hash) + c;

    return hash % MAX_OBJS;
}
unsigned int get_free_cache_page(struct objfs_state *);
int new_datablock(struct objfs_state *);
void init_bitmap(int);



#ifdef CACHE         // CACHED implementation
static int find_read_cached(struct objfs_state *objfs, int block_num, char *user_buf)
{
    int cache_index;
    char *cache_addr;
    if (sobject->block_bitmap[block_num] == 0) {
        return -1;
    }
    if (sobject->block_bitmap[block_num] == 1) {
        cache_index = get_free_cache_page(objfs);
        sobject->cache_bitmap[cache_index] = block_num;
        sobject->block_bitmap[block_num] = cache_index;
        sobject->dirty[cache_index] = 0;
        cache_addr = objfs->cache + (cache_index << 12);
        read_block(objfs, block_num, user_buf);
        memcpy(cache_addr, user_buf, BLOCK_SIZE);
        return 0;
    }
    cache_index = sobject->block_bitmap[block_num];
    cache_addr = objfs->cache + (cache_index << 12);
    memcpy(user_buf, cache_addr, BLOCK_SIZE);
    return 0;
}


static int find_write_cached(struct objfs_state *objfs, int block_num, char *user_buf)
{
    if (sobject->block_bitmap[block_num] == 0) {
        return -1;
    }
    int cache_index;
    if (sobject->block_bitmap[block_num] > 1) {
        cache_index = sobject->block_bitmap[block_num];
    } 
    else {
        cache_index = get_free_cache_page(objfs);
        sobject->cache_bitmap[cache_index] = block_num;
        sobject->block_bitmap[block_num] = cache_index;
    }
    char *cache_addr = objfs->cache + (cache_index << 12);
    memcpy(cache_addr, user_buf, BLOCK_SIZE);
    sobject->dirty[cache_index] = 1;
    return 0;
}
static int obj_sync(struct objfs_state *objfs)
{
    int i;
    int block_num;
    for (i=0;i<PAGES;++i) {
        if (sobject->cache_bitmap[i] == 0)
            continue;
        block_num = sobject->cache_bitmap[i];
        if (sobject->dirty[i] == 0) {
            sobject->block_bitmap[block_num] = 1;
            continue;
        }
        write_block(objfs, block_num, objfs->cache + (i << 12));
        sobject->dirty[i] = 0;
        sobject->block_bitmap[block_num] = 1;
        sobject->cache_bitmap[i] = 0;
    }
    return 0;
}
static void flush_cache(int cache_index, struct objfs_state *objfs)
{
    int block_num;
    block_num = sobject->cache_bitmap[cache_index];
    if (sobject->dirty[cache_index] != 0) {
        write_block(objfs, block_num, objfs->cache + (cache_index << 12));
    }

    pthread_mutex_lock(&sobject->cache_mutex);
    sobject->cache_bitmap[cache_index] = 0;
    pthread_mutex_unlock(&sobject->cache_mutex);

    sobject->block_bitmap[block_num] = 1;
}
#else  //uncached implementation
static int find_read_cached(struct objfs_state *objfs, int block_num, char *user_buf)
{
    return -1;
}
static int find_write_cached(struct objfs_state *objfs, int block_num, char *user_buf)
{
    return -1;
}
static int obj_sync(struct objfs_state *objfs)
{
    return 0;
}
static void flush_cache(int cache_index, struct objfs_state *objfs)
{
    return;
}
#endif

unsigned int get_free_cache_page(struct objfs_state *objfs)
{
    pthread_mutex_lock(&sobject->cache_mutex);
    unsigned int new_cache_index = sobject->cache_index + 1;
    new_cache_index %= PAGES;
    if (new_cache_index < 2)
        new_cache_index = 2;
    sobject->cache_index = new_cache_index;

    if (sobject->cache_bitmap[new_cache_index] != 0)
        flush_cache(new_cache_index, objfs);

    pthread_mutex_unlock(&sobject->cache_mutex);
    return new_cache_index;
}

/*
Returns the object ID.  -1 (invalid), 0, 1 - reserved
*/
long find_object_id(const char *key, struct objfs_state *objfs)
{
    int index = hash(key);
    struct object *obj = &sobject->obj[index];
    if (obj->id && !strcmp(obj->key, key))
        return obj->id;
    int previous_index = index;
    index++;
    index %= MAX_OBJS;
    obj = &sobject->obj[index];
    while (previous_index != index) {
        if (obj->id && !strcmp(obj->key, key))
            return obj->id;
        index++;
        index %= MAX_OBJS;
        obj = &sobject->obj[index];
    }
    return -1;
}

/*
  Creates a new object with obj.key=key. Object ID must be >=2.
  Must check for duplicates.

  Return value: Success --> object ID of the newly created object
                Failure --> -1
*/
long create_object(const char *key, struct objfs_state *objfs)
{
    pthread_mutex_lock(&create_object_lock);
    unsigned long index = hash(key);
    struct object *obj = &sobject->obj[index];

    while (obj->id) {
        if (!strcmp(obj->key, key)) {
            pthread_mutex_unlock(&create_object_lock);
            return -1;
        }
        index++;
        index %= MAX_OBJS;
        obj = &sobject->obj[index];
    }

    obj->size = 0;
    obj->id = ind2ID(index);

    strncpy(obj->key, key, 32);
    for (index=0;index<4;++index)
      obj->indirect_pointers[index] = -1;
    pthread_mutex_unlock(&create_object_lock);
    return obj->id;
}

void init_bitmap(int block_num)
{
    pthread_mutex_lock(&sobject->superBlock_mutex);
    sobject->block_bitmap[block_num] = 1;
    pthread_mutex_unlock(&sobject->superBlock_mutex);
}

/*
  One of the users of the object has dropped a reference
  Can be useful to implement caching.
  Return value: Success --> 0
                Failure --> -1
*/
long release_object(int objid, struct objfs_state *objfs)
{
    return 0;
}



/*
  Destroys an object with obj.key=key. Object ID is ensured to be >=2.

  Return value: Success --> 0
                Failure --> -1
*/
long destroy_object(const char *key, struct objfs_state *objfs)
{
    int id = find_object_id(key, objfs);
    if (id == -1)
        return -1;
    int index = id2Ind(id);
    struct object *obj = &sobject->obj[index];
    if (obj->id == 0)
        return -1;
    for (int i=0;i<4;++i) {
        if (obj->indirect_pointers[i] == -1)
            break;
        // free_block(objfs, obj->indirect_pointers[i]);
        int x, block_num;
        int *tmp;
        malloc_4k(tmp);
        read_block(objfs, obj->indirect_pointers[i], (char *)tmp);
        for (x=0;x<BLOCK_SIZE/4;++x) {
            block_num = tmp[x];
            if (block_num == -1)
                break;
            int cache_index = sobject->block_bitmap[block_num];
            if (cache_index > 1) {

                pthread_mutex_lock(&sobject->cache_mutex);
                sobject->dirty[cache_index] = 0;
                sobject->cache_bitmap[cache_index] = 0;
                pthread_mutex_unlock(&sobject->cache_mutex);

            }
            pthread_mutex_lock(&sobject->superBlock_mutex);
            sobject->block_bitmap[block_num] = 0;
            pthread_mutex_unlock(&sobject->superBlock_mutex);
        }
        pthread_mutex_lock(&sobject->superBlock_mutex);
        sobject->block_bitmap[block_num] = 0;
        pthread_mutex_unlock(&sobject->superBlock_mutex);
        free_4k(tmp);
    }
    obj->id = 0;
    return 0;
}

/*
  Renames a new object with obj.key=key. Object ID must be >=2.
  Must check for duplicates.  
  Return value: Success --> object ID of the newly created object
                Failure --> -1
*/
void read_indirect_block(int block_num, int block_offset, char *buf, int offset, int size, struct objfs_state *objfs)
{

    if (block_num < 0)
      return;
    int new_offset;
    int *block;
    malloc_4k(block);
    if (find_read_cached(objfs, block_num, (char *)block) == -1)
      read_block(objfs, block_num, (char *)block);
    int x=1;
    for (x=block_offset;x<BLOCK_SIZE/4;++x) {
        new_offset = (x-block_offset)*4*1024;
        if (size <= offset+new_offset)
            break;
        if (block[x] == -1)
            break;
        if (find_read_cached(objfs, block[x], buf+offset+new_offset) != -1)
            continue;
        if (read_block(objfs, block[x], buf+offset+new_offset) < 0)
            break;
    }
    free_4k(block);
}

long rename_object(const char *key, const char *newname, struct objfs_state *objfs)
{
    if(strlen(newname) > 32)
        return -1;
    int old_id = find_object_id(key, objfs);
    if (old_id == -1)
        return -1;
    int new_id = create_object(newname, objfs);
    if (new_id == -1)
        return -1;
    struct object *new_obj = &sobject->obj[id2Ind(new_id)];
    struct object *old_obj = &sobject->obj[id2Ind(old_id)];

    new_obj->size = old_obj->size;
    for (int i=0;i<4;++i) {
        new_obj->indirect_pointers[i] = old_obj->indirect_pointers[i];
    }
    pthread_mutex_lock(&create_object_lock);
    old_obj->id = 0;
    pthread_mutex_unlock(&create_object_lock);
    return new_id;
}



/*
  Reads the content of the object onto the buffer with objid = objid.
  Return value: Success --> #of bytes written
                Failure --> -1
*/
long objstore_read(int objid, char *buf, int size, struct objfs_state *objfs, off_t offset)
{
    if (objid < 2)
        return -1;

    struct object *obj = &sobject->obj[id2Ind(objid)];
    if (obj->id == 0) {
        return -1;
    }
    if (size > (obj->size - offset))
        size = obj->size - offset;

    int block_offset = offset/BLOCK_SIZE;
    int dbp_index = block_offset/1024;
    block_offset %= 1024;

    int my_offset;
    char *aux_buf;
    int aux_size;
    if (size % BLOCK_SIZE) {
        aux_size = size/BLOCK_SIZE + 1;
    } else {
        aux_size = size/BLOCK_SIZE;
    }
    malloc_nk(aux_buf, aux_size);
    int i;
    for (i=dbp_index;i<4;++i) {
        my_offset = (i- dbp_index)*BITMAPS/2;
        if (my_offset >= size)
            break;
        read_indirect_block(obj->indirect_pointers[i], block_offset, aux_buf, my_offset, aux_size*BLOCK_SIZE, objfs);

        block_offset = 0; 
    }
    for (i=0;i<size;++i)
        buf[i] = aux_buf[i+(offset%BLOCK_SIZE)];
    return size;
}

int new_datablock(struct objfs_state *objfs)
{
    pthread_mutex_lock(&sobject->superBlock_mutex);
    for (int i=DATA_START;i<total_blocks;++i) {
        if (sobject->block_bitmap[i] == 0) {
            sobject->block_bitmap[i] = 1; 
            pthread_mutex_unlock(&sobject->superBlock_mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&sobject->superBlock_mutex);
    return -1;
}

int write_indirect_block(int block_num, int block_offset, char *buf, int offset, int size, struct objfs_state *objfs)
{
    if (block_num < 0) {
        block_num = new_datablock(objfs);
        init_bitmap(block_num);
        int *block;
        malloc_4k(block);
        for (int x=0;x<BLOCK_SIZE/4;++x)
            block[x] = -1;
        write_block(objfs, block_num, (char *)block);
        free_4k(block);
    }
    int new_offset;
    int *block;
    malloc_4k(block);
    char *tmp;
    malloc_4k(tmp);
    if (find_read_cached(objfs, block_num, (char *)block) == -1) {
        read_block(objfs, block_num, (char *)block);
    }
    int i;
    for (i=block_offset;i<BLOCK_SIZE/4;++i) {
        new_offset = (i-block_offset)*4*1024;
        if (size <= offset+new_offset)
            break;
        if (block[i] == -1) {
            block[i] = new_datablock(objfs);
            init_bitmap(block[i]);
        }
        for (int j=0;j<BLOCK_SIZE;++j)
            tmp[j] = buf[offset+new_offset+j];
        if (find_write_cached(objfs, block[i], buf+offset+new_offset) != -1)
            continue;
        if (write_block(objfs, block[i], tmp) < 0)
            break;
    }
    while (i < BLOCK_SIZE/4) {
        block[i++] = -1;
    }
    if (find_write_cached(objfs, block_num, (char *)block) == -1) {
        write_block(objfs, block_num, (char *)block);
    }
    if (find_read_cached(objfs, block_num, (char *)block) == -1) {
        read_block(objfs, block_num, (char *)block);
    }
    free_4k(tmp);
    free_4k(block);
    return block_num;
}

int read_helper(struct objfs_state *objfs, int block_num, int block_offset, char *buf)
{
    char *block;
    malloc_4k(block);
    if (find_read_cached(objfs, block_num, block) == -1)
        read_block(objfs, block_num, block);
    int new_block_num = block[block_offset];
    if (new_block_num < 0) {
        return 0;
    }
    if (find_read_cached(objfs, new_block_num, buf) == -1)
        read_block(objfs, new_block_num, buf);
    free_4k(block);
    return 0;
}

/*
  Writes the content of the buffer into the object with objid = objid.
  Return value: Success --> #of bytes written
                Failure --> -1
*/
long objstore_write(int objid, const char *buf, int size, struct objfs_state *objfs, off_t offset)
{
    if (objid < 2)
        return -1;
    struct object *obj = &sobject->obj[id2Ind(objid)];
    if (obj->id == 0)
        return -1;
    if (size > BITMAPS*2-offset)
        size = BITMAPS*2-offset;

    int block_offset = offset/BLOCK_SIZE; 
    int dbp_index = block_offset / 1024;
    block_offset %= 1024;

    char *aux_buf;
    int aux_size;
    if (size % BLOCK_SIZE) {
        aux_size = size/BLOCK_SIZE + 1;
    } else {
        aux_size = size/BLOCK_SIZE;
    }
    malloc_nk(aux_buf, aux_size);
    int i, my_offset;

    if ((offset%BLOCK_SIZE) > 0)
        read_helper(objfs, obj->indirect_pointers[dbp_index], block_offset, aux_buf);

    for (i=0;i<size;++i) {
        aux_buf[i+(offset%BLOCK_SIZE)] = buf[i];
    }
    for (i=dbp_index;i<4;++i) {
        my_offset = (i- dbp_index )*BITMAPS/2;
        if (my_offset >= size)
            break;
        obj->indirect_pointers[i] = write_indirect_block(obj->indirect_pointers[i], block_offset, aux_buf, my_offset, aux_size*BLOCK_SIZE, objfs);
        block_offset = 0; 
    }
    while (i < 4) {
        obj->indirect_pointers[i++] = -1;
    }

    free_nk(aux_buf, aux_size);
    obj->size = offset + size;
    return size;
}


/*
  Reads the object metadata for obj->id = buf->st_ino
  Fillup buf->st_size and buf->st_blocks correctly
  See man 2 stat 
*/
int fillup_size_details(struct stat *buf, struct objfs_state *objfs)
{
    int id = buf->st_ino;
    if (id < 2)
        return -1;
    int index = id2Ind(id);
    struct object *obj = &sobject->obj[index];
    if (obj->id == 0)
        return -1;
    buf->st_size = obj->size;
    int num_blocks = 0;
    for (int i=0;i<4;++i) {
        if (obj->indirect_pointers[i] < 0)
            break;
        int block_num = obj->indirect_pointers[i];
        if (block_num < 0) {
            return 0;
        }
        int res = 0;
        int *block;
        malloc_4k(block);
        if (find_read_cached(objfs, block_num, (char *)block) == -1)
            read_block(objfs, block_num, (char *)block);
        for (int x=0;x<BLOCK_SIZE/4;++x) {
            if (block[x] < 0)
                break;
            res++;
        }
        free_4k(block);
        num_blocks += res; 
    }
    buf->st_blocks = num_blocks;
    return 0;
}

/*
   Set your private pointeri, anyway you like.
*/
int objstore_init(struct objfs_state *objfs)
{
    malloc_superBlock(sobject);
    if(!sobject){
        dprintf("%s: malloc\n", __func__);
        return -1;
    }

    int i = 0;
    int j = 0;
    for(i =0;i < sizeof(struct superBlock); i += BLOCK_SIZE ){
        if (read_block(objfs, j, (char *)sobject + i) < 0)
          return -1;
        j++;
    }

    pthread_mutex_init(&sobject->superBlock_mutex, NULL);
    pthread_mutex_init(&sobject->cache_mutex, NULL);
    pthread_mutex_init(&create_object_lock, NULL);

    sobject->cache_index = 1;
    for (i=0;i<PAGES;++i) {
        sobject->cache_bitmap[i] = 0;
        sobject->dirty[i] = 0;
    }

    struct stat sbuf;
    objfs->objstore_data = sobject;
    if(fstat(objfs->blkdev, &sbuf) < 0){
        perror("fstat");
        exit(-1);
    }

    total_blocks = sbuf.st_size/BLOCK_SIZE;

    dprintf("Done objstore init\n");
    return 0;
}
/*
   Cleanup private data. FS is being unmounted
*/
int objstore_destroy(struct objfs_state *objfs)
{
    pthread_mutex_destroy(&sobject->superBlock_mutex);
    pthread_mutex_destroy(&sobject->cache_mutex);
    pthread_mutex_destroy(&create_object_lock);

    obj_sync(objfs);
    objfs->objstore_data = NULL;
    for (int i=0;i<sizeof(struct superBlock);i+=BLOCK_SIZE) {
        write_block(objfs, i/BLOCK_SIZE, (char *)(sobject)+i);
    }
    free_superBlock(sobject);
    dprintf("Done objstore destroy\n");
    return 0;
}
