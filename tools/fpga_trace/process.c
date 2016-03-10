#include <stdio.h>
#include <stdlib.h>

int process(char *old_filename, char  *new_filename)
{
   FILE  *ptr_old, *ptr_new;
   int err = 0, err1 = 0;
   int  a;
   int last;
   int count = 0;
   int address = -1;

   ptr_old = fopen(old_filename, "rb");
   if(ptr_old == 0)
   {
      perror(old_filename);
      exit(1);
   }

   ptr_new = fopen(new_filename, "wb");
   if(ptr_new == 0)
   {
      fclose(ptr_old);
      perror(new_filename);
      exit(1);
   }
   
   last = -1;
   while ((a = fgetc(ptr_old)) != EOF)
   {

      // Read one byte then skip three
      if ((count++ & 3) != 0)
      {
         continue;
      } 
      // Look for a 0->1 transition on bit 7
      if (last >= 0 && !(last & 0x80) && (a & 0x80))
      {
         // If bit 6 is 1, this is the start of a new character
         if (a & 0x40)
         {
            if (address != -1)
            {
               fprintf(ptr_new, "&%06X\n", address);
            }
            address = a & 0x3F;
         }
         else
         {
            address = (address << 6) | (a & 0x3F);
         }
      }
      last = a;
   }
   
   fclose(ptr_new);
   fclose(ptr_old);
   return  0;
}

int  main(int argc, char *argv[])
{
   if (argc != 3)
   {
      fprintf(stderr, "usage: %s <bin file> <hex file>\n", argv[0]);
      exit(1);
   }
   process(argv[1], argv[2]);
}
