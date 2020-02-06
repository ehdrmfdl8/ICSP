// analysis_PSNR.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <stdio.h>
#include <cstdio>
#include <stdlib.h>
#include <algorithm>
#include <cstring>
#include <math.h>
#include <string>
#include <iostream>
#define Width 352
#define Height 288
#define PI 3.142857

using namespace std;
const long Size = Width * Height * 3 / 2;
int Frame = 90;

int main(int argc, char *argv[])
{
    errno_t err, err2;
    FILE* picture;
    FILE* picture2;
    Frame = atoi(argv[1]);
    string original_file = argv[2];
    string decoding_file = argv[3];
    string encoding_file = argv[4];
    string result = decoding_file + "  PSNR  =  ";
    int encoding_Size = 0;
    int decoding_Size = 0;
    double compression_rate = 0;
    double total_MSE = 0;
    double* MSE = new double[Frame];
    memset(MSE, 0, Frame);
    double PSNR = 0;

    err = fopen_s(&picture, encoding_file.c_str(), "rb");
    if (err == 0) {

        fseek(picture, 0, SEEK_END);
        int fileLength = ftell(picture);
        int* encode_data = new int[fileLength];
        fseek(picture, 0, SEEK_SET);
        fread(encode_data, 4, fileLength, picture);
        encoding_Size = fileLength;
        fclose(picture);
    }
    for (int f = 0; f < Frame; f++)
    {
        unsigned char* original_data = new unsigned char[Size];
        unsigned char* decoding_data = new unsigned char[Size];
        
        err = fopen_s(&picture, original_file.c_str(), "rb");
        if (err == 0) {
            fseek(picture, Size * f, SEEK_SET); 
            fread(original_data, 1, Size, picture);
            fclose(picture);
        }
        err = fopen_s(&picture2, decoding_file.c_str(), "rb");
        if (err == 0) {
            fseek(picture, Size * f, SEEK_SET);
            fread(decoding_data, 1, Size, picture);
            fclose(picture);
        }
        
        for (int i = 0; i < Size; i++)
        {
            MSE[f] += pow(abs((int)original_data[i] - (int)decoding_data[i]), 2);
        }
        MSE[f] /= Size;
    }
    for (int f = 0; f < Frame; f++)
    {
        total_MSE += MSE[f];
    }
    total_MSE /= 300.0;
    PSNR = 10 * log10(65025 / total_MSE);
    result += to_string(PSNR);
    printf("PSNR = %f", PSNR);
    decoding_Size = Frame * Size;
    compression_rate = (double)decoding_Size / (double)encoding_Size;
    printf("   compression_rate = %f", compression_rate);
    result += "\t compression rate = " + to_string(compression_rate) + "\n";
    err = fopen_s(&picture, "PSNR result.txt", "a+b");
    if (err == 0) {
        fwrite(result.c_str(), sizeof(char), strlen(result.c_str()), picture);
        fclose(picture);
    }
    return 0;
}
