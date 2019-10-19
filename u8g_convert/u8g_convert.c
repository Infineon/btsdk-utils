/*

conversion of graphic file to C table compatible with SDD1351 graphic controller

*/

#include <stdlib.h>
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
void convertRGBtoMode(unsigned char r, unsigned char g, unsigned char b, char mode, unsigned char* ret)
{
  if ( mode == '1' ) //  RGB compressed to 1 byte
  {
    r &= 0x0e0;
    g &= 0x0e0;
    g >>= 3;
    b >>= 6;
   *ret = r | g | b;
  }
  else if ( mode == '2' ) // RGB compressed to 2 bytes
  {
      r &= ~7;
      g >>= 2;
      b >>= 3;
      ret[1] = b;
      ret[1] |= (g & 7) << 5;
      ret[0] = r;
      ret[0] |= (g>>3) & 7;
  }
  else
  {
      printf("Wrong conversion mode !!!\n");
      exit (-1);
  }
}

void decompressFileToTable(char * filename, char *tablename, char mode)
{
    FILE *f, *o;
    int lineCnt=0;
    int i, x,y,comp;
    unsigned char *outbuf;

    printf("%s %s\n",filename, tablename);
    f = fopen(filename, "rb");
    o= fopen(tablename, "wb");
    outbuf= stbi_load_from_file(f,&x,&y,&comp,0);
    fprintf(o,"unsigned char %s[]={\n", tablename);
    for (i=0; i <x*y*comp;i++)
    {
        if(mode)
        {
            unsigned char buf[2];
            if(comp != 3)
            {
                printf("Cannot convert from non RGB file !!!\n");
                exit(-1);
            }
            convertRGBtoMode(outbuf[i+2],outbuf[i+1],outbuf[i], mode, buf);
            if(mode == '2')
            {
                fprintf(o," 0x%02X, 0x%02X,", buf[0], buf[1]);
            }
            else
            {
                fprintf(o," 0x%02X,", *buf);
            }
            i+=2;
        }
        else
        {
            fprintf(o," 0x%02X,",outbuf[i]);
        }
        if(++lineCnt >=10)
        {
            fprintf(o,"\n");
            lineCnt=0;
        }
    }
    if(!lineCnt)
    {
        fprintf(o,"\n");
    }
    fprintf(o,"};\n");
    fclose(f);
    fclose(o);
}

int main(int argc, char *argv[])
{
  if (argc <4)
  {
      printf("Usage:\n  u8g_convert <src_file> <out_file> <mode>\n    where <mode> can be:\n");
      printf("        1  - compress RGB to 1 byte\n");
      printf("        2  - compress RGB to 2 bytes\n");
      exit(-1);
  }
  else
  {
      decompressFileToTable(argv[1], argv[2], argc>3 ? *argv[3]:0);

      exit(0);
  }

}
