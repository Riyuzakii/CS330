/*
      ......
      syscall user space code
*/
static int main()
{
  void *ptr1;
  char *ptr = (char *) expand(8, MAP_WR)
  
  if(ptr == NULL)
              write("FAILED\n", 7);
  
  *(ptr + 8192) = 'A';   /*Page fault will occur and handled successfully*/
  
  ptr1 = (char *) shrink(7, MAP_WR);
  *ptr = 'A';          /*Page fault will occur and handled successfully*/

  *(ptr + 4096) = 'A';   /*Page fault will occur and PF handler should termminate 
                   the process (gemOS shell should be back) by printing an error message*/
  exit(0);
}


static int main()
{
  void *ptr1;
  char *ptr = (char *) expand(20, MAP_WR);
  
  if(ptr == NULL)
              write("Expand FAILED\n", 7);
  
  *(ptr + 8192) = 'A';   /*Page fault will occur and handled successfully*/
  
  write(ptr+8192,1); /*check if value is written properly */

  ptr1 = (char *) shrink(100, MAP_WR);

  if(ptr1 != NULL)
      write("Shrink not working\n",18);


  *ptr = 'A';          /*Page fault will occur and handled successfully*/

  // ptr1 = (char*)shrink(20,MAP_WR);


  // if(ptr1==NULL)
  //     write("shrink not working\n",18);

  //   /*Page fault will occur and PF handler should termminate 
  //                  the process (gemOS shell should be back) by printing an error message*/
  // char *ptr2 = (char *) expand(12, MAP_RD);

  // if(ptr2==NULL)
  //   write("expand not working\n",18);

  // ptr = (char*)expand(5,MAP_WR);

  // long k =write(ptr+1,1);  // write should get failed

  // if(k!=-1)
  //   write("write not working\n",17);

  // k=write("Hello\n",7);

  // if(k!=-1)
  //   write("write working\n",14);

  // u64* t = (u64*)(0x7FF000000+16);
  // *t = 64;

  // write("stack page fault handled\n",25);

  // int* ptr3 = (int *) expand(20, MAP_RD);

  // int x=*(ptr3+1);

  // if(x==0){
  //   write("read only page fault handled\n",29);
  // }

  // *(ptr3 + 1024)  = 100;   /*page fault will occur and not handled successfully"*/

  exit(0);
}