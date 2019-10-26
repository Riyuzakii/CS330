#ifndef __KV_STORE_H_
#define __KV_STORE_H_
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>

#define MAX_KEY_LEN 256
#define MNT_PATH "../mnt/"
#define MNT_PATH_LEN 7

#define MAKE_KEY(x, y) do{\
                            strcpy((x), MNT_PATH);\
                            memcpy((x) + MNT_PATH_LEN, (y), strlen((y))+1);\
                       }while(0);

extern long lookup_key(char *key);      //Returns the size if found. returns -1 other wise
extern long put_key(char *key, char *val, int size); //Stores the key and value 
extern long get_key(char *key, char *val);   //Reads the key, stores in value. Size of val should be MAX(4096, actual size in blocks * 4096) 
extern long rename_key(char *oldkey, char *newkey);   //Renames the key       
extern long delete_key(char *key);  //Deletes the key from the store

#endif
