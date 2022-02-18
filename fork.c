#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include<sys/times.h>
#include <time.h>

typedef unsigned short WORD; 
typedef unsigned int DWORD; 
typedef unsigned int LONG; 
typedef unsigned char BYTE;
//typedef uint8_t  BYTE;

typedef struct tagBITMAPFILEHEADER 
 { 
 WORD bfType;  //specifies the file type 
 DWORD bfSize;  //specifies the size in bytes of the bitmap file 
 WORD bfReserved1;  //reserved; must be 0 
 WORD bfReserved2;  //reserved; must be 0 
 DWORD bfOffBits;  //species the offset in bytes from the bitmapfileheader to the bitmap bits 
 }BITMAPFILEHEADER; 

typedef struct tagBITMAPINFOHEADER 
 { 
 DWORD biSize;  //specifies the number of bytes required by the struct 
 LONG biWidth;  //specifies width in pixels 
 LONG biHeight;  //species height in pixels 
 WORD biPlanes; //specifies the number of color planes, must be 1 
 WORD biBitCount; //specifies the number of bit per pixel 
 DWORD biCompression;//spcifies the type of compression 
 DWORD biSizeImage;  //size of image in bytes 
 LONG biXPelsPerMeter;  //number of pixels per meter in x axis 
 LONG biYPelsPerMeter;  //number of pixels per meter in y axis 
 DWORD biClrUsed;  //number of colors used by th ebitmap 
 DWORD biClrImportant;  //number of colors that are important 
 }BITMAPINFOHEADER; 

typedef struct RGBTriple{
    BYTE blue;
    BYTE green;
    BYTE red;
}RGBTriple;
void printManPage();
int read_bmp(FILE* fPtr, BITMAPFILEHEADER* bf, BITMAPINFOHEADER* bi);
int write_bmp_header(FILE* outPtr, BITMAPFILEHEADER* bf, BITMAPINFOHEADER* bi, int padding, BYTE* bytearray);
void brighten(BYTE* bytearray, float brightness, long int size_arr);
int write_bmp(FILE* outPtr, BITMAPFILEHEADER* bf, BITMAPINFOHEADER* bi, int padding, BYTE* bytearray);
int write_pixels(FILE* outPtr, int padding, BYTE* bytearray, long int arr_size, BITMAPINFOHEADER* bi);

int main(int argc, char *argv[]){
    if(argc!=5){
        printManPage();
        return 1;}
    char *input = argv[1];
    FILE *inPtr = fopen(input, "rb");
    if (inPtr == NULL){
        fprintf(stderr, "Could not open %s.\n", input);
        printManPage();
        return 1;
    }
    
    BITMAPFILEHEADER *bf = (BITMAPFILEHEADER *)mmap(NULL, sizeof(BITMAPFILEHEADER *), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    BITMAPINFOHEADER *bi = (BITMAPINFOHEADER *)mmap(NULL, sizeof(BITMAPINFOHEADER *), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    
    float brightness = atof(argv[2]);
    read_bmp(inPtr, bf, bi);
    int padding = (4 - (bi->biWidth * sizeof(RGBTriple)) % 4) % 4;
    long int total_bytes = (bi->biHeight)*(bi->biWidth*3+padding);
    BYTE* bytearray = (BYTE *)mmap(NULL, sizeof(BYTE *)*total_bytes, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    fread(bytearray, bf->bfSize, 1, inPtr);
    fclose(inPtr);
    // long int maybe for how big lion image is? Lion segfaults at fork too.
    long int i = 0;
    long int k = 0;
    long int j = 0;
    long int l = 0;
    //BYTE bytearray1[total_bytes/2];
    //BYTE bytearray2[total_bytes/2];
    BYTE* bytearray1 = (BYTE *)mmap(NULL, sizeof(BYTE *)*total_bytes/2, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    BYTE* bytearray2 = (BYTE *)mmap(NULL, sizeof(BYTE *)*total_bytes/2, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

    //start clock
    struct tms time1;
    struct tms time2;
    times(&time1);//start
    long double clktck = sysconf(_SC_CLK_TCK);

    if(atoi(argv[3])){
        int pid = fork();
        if(pid==0){
            
            j=0;
            //fprintf(stderr, "Child\n");
            for(l=0; l< total_bytes; l++){
                if(l>=total_bytes/2){
                    bytearray2[j++] = bytearray[l];
                }
            }
            brighten(bytearray2, brightness, total_bytes/2);
            //fprintf(stderr, "Child Done\n");
            return 0;
            
        }
        else{
            k=0;
            //fprintf(stderr, "Parent\n");
            for(i=0; i< total_bytes; i++){
                if(i<total_bytes/2){
                    bytearray1[k++] = bytearray[i];
                }
            }
            brighten(bytearray1, brightness, total_bytes/2);
        }
        wait(0);
        times(&time2);
        fprintf(stderr,"time difference with times struct(cstime) \t \t%Lf \n",(time2.tms_cstime - time1.tms_cstime)/clktck);
        fprintf(stderr,"time difference with times struct(cutime) \t \t%Lf \n",(time2.tms_cutime - time1.tms_cutime)/clktck);
        fprintf(stderr,"time difference with times struct (stime)\t \t%Lf \n",(time2.tms_stime - time1.tms_stime)/clktck);
        fprintf(stderr,"time difference with times struct (utime)\t \t%Lf \n",(time2.tms_utime - time1.tms_utime)/clktck);

        char *output = argv[4];
        FILE *outPtr = fopen(output, "wb");
        if (outPtr == NULL){
            fprintf(stderr, "Could not open %s.\n", output);
            printManPage();
            return 1;
        }
        write_bmp_header(outPtr, bf, bi, padding, bytearray);
        write_pixels(outPtr, padding, bytearray1, total_bytes/2, bi);
        write_pixels(outPtr, padding, bytearray2, total_bytes/2, bi);
        munmap(bf, sizeof(BITMAPFILEHEADER*));
        munmap(bi, sizeof(BITMAPINFOHEADER*));
        munmap(bytearray, sizeof(BYTE *)*total_bytes);
        munmap(bytearray1, sizeof(BYTE *)*total_bytes/2);
        munmap(bytearray2, sizeof(BYTE *)*total_bytes/2);
        fclose(outPtr);
        
        return 0;
    }else{
        brighten(bytearray, brightness, total_bytes);
        times(&time2);
        fprintf(stderr,"time difference with times struct(cstime) \t \t%Lf \n",(time2.tms_cstime - time1.tms_cstime)/clktck);
        fprintf(stderr,"time difference with times struct(cutime) \t \t%Lf \n",(time2.tms_cutime - time1.tms_cutime)/clktck);
        fprintf(stderr,"time difference with times struct (stime)\t \t%Lf \n",(time2.tms_stime - time1.tms_stime)/clktck);
        fprintf(stderr,"time difference with times struct (utime)\t \t%Lf \n",(time2.tms_utime - time1.tms_utime)/clktck);

        char *output = argv[4];
        FILE *outPtr = fopen(output, "wb");
        if (outPtr == NULL){
            fprintf(stderr, "Could not open %s.\n", output);
            printManPage();
            return 1;
        }
        write_bmp_header(outPtr, bf, bi, padding, bytearray);
        write_pixels(outPtr, padding, bytearray, total_bytes, bi);
        munmap(bf, sizeof(BITMAPFILEHEADER*));
        munmap(bi, sizeof(BITMAPINFOHEADER*));
        munmap(bytearray, sizeof(BYTE *)*total_bytes);
        munmap(bytearray1, sizeof(BYTE *)*total_bytes/2);
        munmap(bytearray2, sizeof(BYTE *)*total_bytes/2);
        fclose(outPtr);
        
        return 0;

    }

}

int read_bmp(FILE* inPtr, BITMAPFILEHEADER* bf, BITMAPINFOHEADER* bi){
    // bf
    fread(&(bf->bfType), sizeof(WORD), 1, inPtr);
    fread(&(bf->bfSize), sizeof(DWORD), 1, inPtr);
    fread(&(bf->bfReserved1), sizeof(WORD), 1, inPtr);
    fread(&(bf->bfReserved2), sizeof(WORD), 1, inPtr);
    fread(&(bf->bfOffBits), sizeof(DWORD), 1, inPtr);
    // bi
    fread(&(bi->biSize), sizeof(DWORD), 1, inPtr);
    fread(&(bi->biWidth), sizeof(LONG), 1, inPtr);
    fread(&(bi->biHeight), sizeof(LONG), 1, inPtr);
    fread(&(bi->biPlanes), sizeof(WORD), 1, inPtr);
    fread(&(bi->biBitCount), sizeof(WORD), 1, inPtr);
    fread(&(bi->biCompression), sizeof(DWORD), 1, inPtr);
    fread(&(bi->biSizeImage), sizeof(DWORD), 1, inPtr);
    fread(&(bi->biXPelsPerMeter), sizeof(LONG), 1, inPtr);
    fread(&(bi->biYPelsPerMeter), sizeof(LONG), 1, inPtr);
    fread(&(bi->biClrUsed), sizeof(DWORD), 1, inPtr);
    fread(&(bi->biClrImportant), sizeof(DWORD), 1, inPtr);
    
    return 0;
}

int write_pixels(FILE* outPtr, int padding, BYTE* bytearray, long int arr_size, BITMAPINFOHEADER* bi){
    int i=0, k=0, j=0;
    for(i; i< arr_size; i++){
        if(k >= bi->biWidth*3){
            for(j; j<padding;j++){
                fputc(0x00, outPtr);
            }
            i+=padding-1;
            k=0,j=0;;
        }
        else{
            BYTE byte = bytearray[i];
            fwrite(&byte, sizeof(BYTE), 1, outPtr);
            k++;
        }
    }
}
void printManPage(){
    fprintf(stderr, "NAME\n     fork - a program to brigthen bmp images\nSYNOPSIS\n     program [bmpFile] [Brightness(0-1)] [Parallel(0/1)] [bmpOutFile]\nEXAMPLE\n     ./fork wolf.bmp 1 0 brightWolf.bmp\n");
    return;
}



void brighten(BYTE* bytearray, float brightness, long int size_arr){
    int i, j, k;
    for(i=0; i<size_arr;i++){
        int byte = (int)bytearray[i];
        byte = byte + brightness*225;
        if(byte>225){byte = 225;}
        bytearray[i] = (BYTE) byte; 
    }
    return;
}
int write_bmp_header(FILE* outPtr, BITMAPFILEHEADER* bf, BITMAPINFOHEADER* bi, int padding, BYTE* bytearray){
    fwrite(&(bf->bfType), sizeof(WORD), 1, outPtr);
    fwrite(&(bf->bfSize), sizeof(DWORD), 1, outPtr);
    fwrite(&(bf->bfReserved1), sizeof(WORD), 1, outPtr);
    fwrite(&(bf->bfReserved2), sizeof(WORD), 1, outPtr);
    fwrite(&(bf->bfOffBits), sizeof(DWORD), 1, outPtr);

    fwrite(&(bi->biSize), sizeof(DWORD), 1, outPtr);
    fwrite(&(bi->biWidth), sizeof(LONG), 1, outPtr);
    fwrite(&(bi->biHeight), sizeof(LONG), 1, outPtr);
    fwrite(&(bi->biPlanes), sizeof(WORD), 1 , outPtr);
    fwrite(&(bi->biBitCount), sizeof(WORD), 1, outPtr);
    fwrite(&(bi->biCompression), sizeof(DWORD), 1, outPtr);
    fwrite(&(bi->biSizeImage), sizeof(DWORD), 1, outPtr);
    fwrite(&(bi->biXPelsPerMeter), sizeof(LONG), 1, outPtr);
    fwrite(&(bi->biYPelsPerMeter), sizeof(LONG), 1, outPtr);
    fwrite(&(bi->biClrUsed), sizeof(DWORD), 1, outPtr);
    fwrite(&(bi->biClrImportant), sizeof(DWORD), 1, outPtr);
    return 0;
}

int write_bmp(FILE* outPtr, BITMAPFILEHEADER* bf, BITMAPINFOHEADER* bi, int padding, BYTE* bytearray){
    fwrite(&(bf->bfType), sizeof(WORD), 1, outPtr);
    fwrite(&(bf->bfSize), sizeof(DWORD), 1, outPtr);
    fwrite(&(bf->bfReserved1), sizeof(WORD), 1, outPtr);
    fwrite(&(bf->bfReserved2), sizeof(WORD), 1, outPtr);
    fwrite(&(bf->bfOffBits), sizeof(DWORD), 1, outPtr);

    fwrite(&(bi->biSize), sizeof(DWORD), 1, outPtr);
    fwrite(&(bi->biWidth), sizeof(LONG), 1, outPtr);
    fwrite(&(bi->biHeight), sizeof(LONG), 1, outPtr);
    fwrite(&(bi->biPlanes), sizeof(WORD), 1 , outPtr);
    fwrite(&(bi->biBitCount), sizeof(WORD), 1, outPtr);
    fwrite(&(bi->biCompression), sizeof(DWORD), 1, outPtr);
    fwrite(&(bi->biSizeImage), sizeof(DWORD), 1, outPtr);
    fwrite(&(bi->biXPelsPerMeter), sizeof(LONG), 1, outPtr);
    fwrite(&(bi->biYPelsPerMeter), sizeof(LONG), 1, outPtr);
    fwrite(&(bi->biClrUsed), sizeof(DWORD), 1, outPtr);
    fwrite(&(bi->biClrImportant), sizeof(DWORD), 1, outPtr);

    int i, j, k, biHeight;

    for(i = 0, biHeight = abs(bi->biHeight); i < biHeight; i++)
    {
        for(j = 0; j < bi->biWidth-padding; j++)
        {
            BYTE blue = bytearray[j*3+i*bi->biWidth];
            BYTE grn = bytearray[j*3+i*bi->biWidth +1];
            BYTE red = bytearray[j*3+i*bi->biWidth+2];
            fwrite(&blue, sizeof(BYTE), 1, outPtr);
            fwrite(&grn, sizeof(BYTE), 1, outPtr);
            fwrite(&red, sizeof(BYTE), 1, outPtr);
        }
    }
    return 0;
}
