#include <stdio.h>
#include <cstdio>
#include <stdlib.h>
#include <algorithm>
#include <cstring>
#include <math.h>

#define Width 352
#define Height 288
#define PI 3.142857

using namespace std;
const long Size = Width * Height * 3 / 2;
const long YSize = Width * Height;
const long MPM_BUF_Size = 1584;
const long USize = Width * Height * 1 / 4;
const long VSize = Width * Height * 1 / 4;
const int QP_DC = 8;//2~32
const int QP_AC = 16;//2~64
const int intra_period = 5; //0~31
int** ZigZag_Scan(int matrix[8][8]);
int* entropy_encoding(int**);
int** DPCM_Mode1(int matrix[36][44]) {
	int** MODE1_matrix = new int* [36];
	for (int i = 0; i < 36; i++)
	{
		MODE1_matrix[i] = new int[44];
	}
	for (int i = 0; i < 36; i++)
	{
		for (int j = 0; j < 44; j++)
		{
			if (i != 0 && j != 0) {
				MODE1_matrix[i][j] = matrix[i][j] - (matrix[i][j - 1] + matrix[i - 1][j]) / 2;
			}
			else if (i == 0 && j != 0) {//1번째 행의 블록 수행
				MODE1_matrix[i][j] = matrix[i][j] - (matrix[i][j - 1] + 1024) / 2;
			}
			else if (j == 0 && i != 0) {//1번째 열의 블록 수행
				MODE1_matrix[i][j] = matrix[i][j] - (matrix[i - 1][j] + 1024) / 2;
			}
			else {// k > 44 || ((k % 44) != 0) 인 블록
				MODE1_matrix[i][j] = matrix[i][j] - 1024;
			}
		}
	}
	return MODE1_matrix;
}
int* DC_DPCM(int* buffer) {
	int* new_buffer = new int[YSize];
	int matrix[36][44] = { 0, };

	int** MODE1_matrix = new int* [36];
	for (int i = 0; i < 36; i++)
	{
		MODE1_matrix[i] = new int[44];
	}

	int n = -1;
	int m = 0;
	for (int k = 0; k < 1584; k++)
	{
		if (k % 44 == 0)
		{
			n++;
			m = 0;
		}
		else // k가 1일때 부터 m값 증가
			m++;
		//8X8 블록화
		matrix[n][m] = buffer[Width * (8 * n) + 8 * m];
	}
	MODE1_matrix = DPCM_Mode1(matrix);
	new_buffer = buffer;
	for (int n = 0; n < 36; n++)
	{
		for (int m = 0; m < 44; m++)
		{
			new_buffer[Width * (8 * n) + 8 * m] = MODE1_matrix[n][m];
			//printf("%d", MODE1_matrix[n][m]);
		}
	}
	for (int i = 0; i < 36; i++)
		delete[] MODE1_matrix[i];
	delete[] MODE1_matrix;
	return new_buffer;
}
int** Reverse_DPCM_Mode1(int matrix[36][44]) {
	int** MODE1_matrix = new int* [36];
	for (int i = 0; i < 36; i++)
	{
		MODE1_matrix[i] = new int[44];
	}
	for (int i = 0; i < 36; i++)
	{
		for (int j = 0; j < 44; j++)
		{
			if (i != 0 && j != 0) {
				matrix[i][j] = matrix[i][j] + (matrix[i][j - 1] + matrix[i - 1][j]) / 2;
			}
			else if (i == 0 && j != 0) {//1번째 행의 블록 수행
				matrix[i][j] = matrix[i][j] + (matrix[i][j - 1] + 1024) / 2;
			}
			else if (j == 0 && i != 0) {//1번째 열의 블록 수행
				matrix[i][j] = matrix[i][j] + (matrix[i - 1][j] + 1024) / 2;
			}
			else {
				matrix[i][j] = matrix[i][j] + 1024;
			}
		}
	}
	for (int i = 0; i < 36; i++)
	{
		for (int j = 0; j < 44; j++)
		{
			MODE1_matrix[i][j] = matrix[i][j];
		}
	}
	return MODE1_matrix;
}
int* Reverse_DC_DPCM(int* buffer) {
	int* new_buffer = new int[YSize];
	int matrix[36][44] = { 0, };

	int** MODE1_matrix = new int* [36];
	for (int i = 0; i < 36; i++)
	{
		MODE1_matrix[i] = new int[44];
	}

	int n = -1;
	int m = 0;
	for (int k = 0; k < 1584; k++)
	{
		if (k % 44 == 0)
		{
			n++;
			m = 0;
		}
		else // k가 1일때 부터 m값 증가
			m++;
		//8X8 블록화
		matrix[n][m] = buffer[Width * (8 * n) + 8 * m];
	}
	MODE1_matrix = Reverse_DPCM_Mode1(matrix);
	new_buffer = buffer;
	for (int n = 0; n < 36; n++)
	{
		for (int m = 0; m < 44; m++)
		{
			new_buffer[Width * (8 * n) + 8 * m] = MODE1_matrix[n][m];
			//printf("%d", MODE1_matrix[n][m]);
		}
	}
	for (int i = 0; i < 36; i++)
		delete[] MODE1_matrix[i];
	delete[] MODE1_matrix;
	return new_buffer;
}
int* Reordering(int* buffer) {
	int* new_buffer = new int[YSize];
	int* encoding_buffer = new int[YSize];
	int matrix[8][8] = { 0, };
	int** reorder_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		reorder_matrix[i] = new int[8];
	}
	int n = -1;
	int m = 0;
	for (int k = 0; k < 1584; k++)
	{
		if (k % 44 == 0)
		{
			n++;
			m = 0;
		}
		else // k가 1일때 부터 m값 증가
			m++;
		//8X8 블록화
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				matrix[i][j] = buffer[Width * (i + 8 * n) + j + 8 * m];
			}
		}
		reorder_matrix = ZigZag_Scan(matrix);
		encoding_buffer = entropy_encoding(reorder_matrix);
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				new_buffer[Width * (i + 8 * n) + j + 8 * m] = reorder_matrix[i][j];
				printf("%X ", reorder_matrix[i][j]);
			}
			printf("\n");
		}
		for (int i = 0; i < 64; i++)
		{
			printf("%X ", encoding_buffer[i]);
		}
		for (int i = 0; i < 8; i++)
			delete[] reorder_matrix[i];
		delete[] reorder_matrix;
	}

	return new_buffer;
}
int** ZigZag_Scan(int matrix[8][8]) {
	int* buffer = new int[64];
	int* new_buffer = new int[64];
	int** new_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		new_matrix[i] = new int[8];
	}
	int ZigZag_block[] = {
	0,1,5,6,14,15,27,28,
	2,4,7,13,16,26,29,42,
	3,8,12,17,25,30,41,43,
	9,11,18,24,31,40,44,53,
	10,19,23,32,39,45,52,54,
	20,22,33,38,46,51,55,60,
	21,34,37,47,50,56,59,61,
	35,36,48,49,57,58,62,63
	};
	int k = 0;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			buffer[k] = matrix[i][j];
			k++;
		}
	}
	for (int k = 0; k < 64; k++)
	{
		new_buffer[k] = buffer[ZigZag_block[k]];
	}
	k = 0;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			new_matrix[i][j] = new_buffer[k];
			k++;
		}
	}
	return new_matrix;
}
int** Reverse_ZigZag_Scan(int matrix[8][8]) {
	int* buffer = new int[64];
	int* new_buffer = new int[64];
	int** new_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		new_matrix[i] = new int[8];
	}
	int ZigZag_block[] = {
	0,1,5,6,14,15,27,28,
	2,4,7,13,16,26,29,42,
	3,8,12,17,25,30,41,43,
	9,11,18,24,31,40,44,53,
	10,19,23,32,39,45,52,54,
	20,22,33,38,46,51,55,60,
	21,34,37,47,50,56,59,61,
	35,36,48,49,57,58,62,63
	};
	int k = 0;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			buffer[k] = matrix[i][j];
			k++;
		}
	}
	for (int k = 0; k < 64; k++)
	{
		new_buffer[ZigZag_block[k]] = buffer[k];
	}
	k = 0;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			new_matrix[i][j] = new_buffer[k];
			k++;
		}
	}
	return new_matrix;
}
int* Reverse_Reordering(int* buffer) {
	int* new_buffer = new int[YSize];
	int matrix[8][8] = { 0, };
	int** reorder_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		reorder_matrix[i] = new int[8];
	}
	int n = -1;
	int m = 0;
	for (int k = 0; k < 1584; k++)
	{
		if (k % 44 == 0)
		{
			n++;
			m = 0;
		}
		else // k가 1일때 부터 m값 증가
			m++;
		//8X8 블록화
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				matrix[i][j] = buffer[Width * (i + 8 * n) + j + 8 * m];
			}
		}
		reorder_matrix = Reverse_ZigZag_Scan(matrix);
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				new_buffer[Width * (i + 8 * n) + j + 8 * m] = reorder_matrix[i][j];
			}
		}
		for (int i = 0; i < 8; i++)
			delete[] reorder_matrix[i];
		delete[] reorder_matrix;
	}

	return new_buffer;
}
int* entropy_encoding(int** buffer) {
	//int** entropy_matrix = new int* [8];
	//for (int i = 0; i < 8; i++)
	//{
	//	entropy_matrix[i] = new int[8];
	//}
	int* new_buffer = new int[64] {0, };
	int m = 0;
	int n = 0;
	int k = 0;
	int total_length;
	int min_val;
	int range_size;
	int range = 0;
	int mask = 0;
	int over_mask = 0;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			int x = buffer[i][j];

			if (x == 0) {
				total_length = 2;
				mask = 0b00000000;
				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if (x == 1 || x == -1) {
				total_length = 4;
				if (x > 0) {
					mask = 0b00000101;
				}
				else {
					mask = 0b00000100;
				}
				//range = abs(x) - 1;
				//mask = mask << 0;
				//mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 2 && x < 4) || (x > -4 && x <= -2)) {
				min_val = 2;
				total_length = 5;
				range_size = 1;
				if (x > 0) {
					mask = 0b00000111;
				}
				else {
					mask = 0b00000110;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 4 && x < 8) || (x > -8 && x <= -4)) {
				min_val = 4;
				total_length = 6;
				range_size = 2;
				if (x > 0) {
					mask = 0b00000111;
				}
				else {
					mask = 0b00000110;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 8 && x < 16) || (x > -16 && x <= -8)) {
				min_val = 8;
				total_length = 7;
				range_size = 3;
				if (x > 0) {
					mask = 0b00001011;
				}
				else {
					mask = 0b00001010;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 16 && x < 32) || (x > -32 && x <= -16)) {
				min_val = 16;
				total_length = 8;
				range_size = 4;
				if (x > 0) {
					mask = 0b00001101;
				}
				else {
					mask = 0b00001100;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 32 && x < 64) || (x > -64 && x <= -32)) {
				min_val = 32;
				total_length = 10;
				range_size = 5;
				if (x > 0) {
					mask = 0b00011101;
				}
				else {
					mask = 0b00011100;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 64 && x < 128) || (x > -128 && x <= -64)) {
				min_val = 64;
				total_length = 12;
				range_size = 6;
				if (x > 0) {
					mask = 0b00111101;
				}
				else {
					mask = 0b00111100;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 128 && x < 256) || (x > -256 && x <= -128)) {
				min_val = 128;
				total_length = 14;
				range_size = 7;
				if (x > 0) {
					mask = 0b01111101;
				}
				else {
					mask = 0b01111100;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 256 && x < 512) || (x > -512 && x <= -256)) {
				min_val = 256;
				total_length = 16;
				range_size = 8;
				if (x > 0) {
					mask = 0b11111101;
				}
				else {
					mask = 0b11111100;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 512 && x < 1024) || (x > -1024 && x <= -512)) {
				min_val = 512;
				total_length = 18;
				range_size = 9;
				if (x > 0) {
					mask = 0b111111101;
				}
				else {
					mask = 0b111111100;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 1024 && x < 2048) || (x > -2048 && x <= -1024)) {
				min_val = 1024;
				total_length = 20;
				range_size = 10;
				if (x > 0) {
					mask = 0b1111111101;
				}
				else {
					mask = 0b1111111100;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 2048 && x < 4096) || (x > -4096 && x <= -2048)) {
				min_val = 2048;
				total_length = 22;
				range_size = 11;
				if (x > 0) {
					mask = 0b11111111101;
				}
				else {
					mask = 0b11111111100;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 4096 && x < 8192) || (x > -8192 && x <= -4096)) {
				min_val = 4096;
				total_length = 24;
				range_size = 12;
				if (x > 0) {
					mask = 0b111111111101;
				}
				else {
					mask = 0b111111111100;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = 0;
				if (m > 32) {
					k = m - 32;
					over_mask = mask >> (32 - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] >> (total_length - k);
				mask = mask << k;
				new_buffer[n] = new_buffer[n] | mask;
			}
		}
	}
	return new_buffer;
}
int main() {
	errno_t err, err2, err3, err4, err5;
	FILE* picture;
	FILE* picture2;
	FILE* picture3;
	FILE* picture4;
	FILE* picture5;

	int Frame = 0;
	err = fopen_s(&picture, "football_cif(352X288)_90f.yuv", "rb");
	if (err == 0) {
		fseek(picture, 0, SEEK_END);
		int fileLength = ftell(picture);
		Frame = fileLength / Size;
	}


	unsigned char* buffer7 = new unsigned char[Size];
	for (int f = 0; f < Frame; f++)
	{
		unsigned char* buffer = new unsigned char[Size];
		int* buffer2 = new int[YSize];
		int* buffer3 = new int[YSize];
		int* buffer4 = new int[YSize];
		int* buffer5 = new int[YSize];
		int* buffer6 = new int[YSize];
		unsigned char* buffer7 = new unsigned char[Size];
		int* encode_buffer = new int[YSize];
		err = fopen_s(&picture, "football_cif(352X288)_90f.yuv", "rb");
		if (err == 0) {

			fseek(picture, Size * f, SEEK_SET); //2번째는 이동할 거리, 3번째는 이동방식 -> 파일의 처음 위치를 기준으로 이동한다.
			fread(buffer, 1, Size, picture); // (*ptr, size ,count ,FILE* stream) 2번째 인자의 단위는 바이트 이다.
			for (int i = 0; i < YSize; i++) {
				buffer2[i] = buffer[i];
			}
			buffer3 = DC_DPCM(buffer2);
			buffer4 = Reordering(buffer3);
			
			buffer5 = Reverse_Reordering(buffer4);
			buffer6 = Reverse_DC_DPCM(buffer5);
			for (int i = 0; i < YSize; i++)
			{
				buffer7[i] = (unsigned char)buffer6[i];
			}
			fclose(picture);
			delete[] buffer;
			buffer = NULL;
		}
		err3 = fopen_s(&picture3, "re_reoredering_Y_football_cif(352X288)_90f.yuv", "a+b"); // wb대신에 a+b를 사용하는 이유는 파일을 이어쓰기 때문이다.
		if (err3 == 0) {
			fseek(picture3, YSize * f, SEEK_SET);

			fwrite(buffer7, 1, YSize, picture3);

			fclose(picture3);
		}

		delete[] buffer3;
		buffer3 = NULL;
		delete[] buffer4;
		buffer4 = NULL;
		delete[] buffer5;
		buffer5 = NULL;

		delete[] buffer7;
		buffer7 = NULL;
	}

}