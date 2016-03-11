#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>


int process(char *old_filename, char  *new_filename)
{
   FILE  *ptr_old, *ptr_new;
   int err = 0, err1 = 0;
   int  a;
   int last;
   int count = 0;
   uint64_t shiftreg = 0;
   uint32_t addr;
   uint32_t data;
   int be;

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
         // printf("%02x\n", a);
         // If bit 6 is 1, this is the end of a sequence
         shiftreg = (shiftreg << 6) | (a & 0x3F);
         if (a & 0x40)
         {
            if (a & 0x20)
            {
               // A write has occurred
               // Bits 27..24 are the BE values
               // Bits 23..0 are the address
               be = shiftreg & 15;
               data = (shiftreg >> 6) & 0xFFFFFFFF;
               addr = (shiftreg >> 38) & 0xFFFFFC;
               if (be & 1)
               {
                  fprintf(ptr_new, "WR &%06X &%02X\n", addr, data & 0xFF);
               }
               if (be & 2)
               {
                  fprintf(ptr_new, "WR &%06X &%02X\n", addr + 1, (data >> 8) & 0xFF);
               }
               if (be & 4)
               {
                  fprintf(ptr_new, "WR &%06X &%02X\n", addr + 2, (data >> 16) & 0xFF);
               }
               if (be & 8)
               {
                  fprintf(ptr_new, "WR &%06X &%02X\n", addr + 3, (data >> 24) & 0xFF);
               }
            }
            else
            {
               // PC has changed
               addr = (shiftreg >> 6) & 0xFFFFFF;
               fprintf(ptr_new, "PC &%06X\n", addr);
            }
            shiftreg = 0;
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
