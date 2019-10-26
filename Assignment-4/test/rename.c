#include "kvstore.h"
int main(int argc, char **argv)
{
   if(argc != 3){
            printf("Usage: %s <oldkey> <newkey>\n", argv[0]);
            exit(-1);
   }
   if(rename_key(argv[1], argv[2]) < 0){
            printf("Rename failed. <old:%s> <new:%s>\n", argv[1], argv[2]);
   }
    return 0;
}
