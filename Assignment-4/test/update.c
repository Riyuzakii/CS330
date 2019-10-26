#include "kvstore.h"
int main(int argc, char **argv)
{
   if(argv[1]){
            if(delete_key(argv[1]) < 0)
                  printf("Update error\n");
    }else{
            printf("Usage: %s <key>\n", argv[0]);
            exit(-1);
    }
   char value[64];
   sprintf(value, "I am [updated] %s. Registered to CS330.", argv[1]);
   if(put_key(argv[1], value, 64) < 0)
             printf("Create error\n");
    return 0;
}
