#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main()
{
    char* filename = "out";
    char* filepath = "/home/cnc-nqco/Documents/UHD_Test/Recv_file_fr_USRP";
    char filename_full[256];
    
    FILE* fp;

    sprintf(filename_full, "%s/%s.dat", filepath, filename);
    
    fp = fopen(filename_full, "rb");

    fseek(fp, 0, SEEK_END);
    long int filesize_in_byte = ftell(fp);

    printf("Kich thuoc file = %ld (bytes)\n",filesize_in_byte);

    int num_sample = filesize_in_byte/8;

    float* data = (float*)calloc(num_sample*2, sizeof(float));

    FILE* fp_I;
    FILE* fp_Q;

    sprintf(filename_full, "%s/%s_I.txt", filepath, filename);
    fp_I = fopen(filename_full, "wb");

    sprintf(filename_full, "%s/%s_Q.txt", filepath, filename);
    fp_Q = fopen(filename_full, "wb");

    fseek(fp, 0, SEEK_SET);
    fread(data, 4, num_sample*2, fp);

    for (int i=0; i<num_sample; i++)
    {
        fprintf(fp_I, "%f\n", data[2*i]);
        fprintf(fp_Q, "%f\n", data[2*i+1]);
    }

    fclose(fp);
    fclose(fp_I);
    fclose(fp_Q);

    return 0;
}