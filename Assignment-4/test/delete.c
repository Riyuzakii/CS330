#include "kvstore.h"
int main(int argc, char **argv)
{
   if(argv[1]){
            if(delete_key(argv[1]) < 0)
                  printf("Delete error\n");
    }else{
            printf("Usage: %s <key>\n", argv[0]);
    }
    return 0;
}
