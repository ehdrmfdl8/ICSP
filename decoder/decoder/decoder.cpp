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

const int QP_DC = 1;//2~32
const int QP_AC = 1;//2~64
const int intra_period = 5; //0~31
const int pixel_dpcm_select = 0;
const int Frame_size = 90;
int getAbit(unsigned int, int);

//역 인트라
int** Reverse_Intra_Mode0(int matrix[8][8], int, int*, int*);
int** Reverse_Intra_Mode1(int matrix[8][8], int, int*, int*);
int** Reverse_Intra_Mode2(int matrix[8][8], int, int*, int*);
unsigned char* Reverse_Intra_Prediction(int*, int, unsigned char*);
//역 pixel DPCM
int** Reverse_DPCM_Mode0(int matrix[8][8], int, int*, int*);
int** Reverse_DPCM_Mode1(int matrix[8][8], int, int*, int*);
int** Reverse_DPCM_Mode2(int matrix[8][8], int, int*, int*);
int* Reverse_pixel_DPCM(int*, int);
//엔트로피 디코딩
int* entropy_decoding(unsigned int* ,int, unsigned char*, unsigned char*, unsigned char*);
//역 인터
unsigned char* Reverse_Motion_Estimation(int*, unsigned char*, unsigned char*, unsigned char*);
//역 양자화
int** DeQuantization(int matrix[8][8]);
//역 DCT
int** BDCT(int**);
int* IDCT_DEQUANTI(int*);
//역 DC_DPCM
int** Reverse_DPCM_Mode1(int matrix[36][44]);
int* Reverse_DC_DPCM(int* buffer);
//역 reordering
int** Reverse_ZigZag_Scan(int matrix[8][8]);
int* Reverse_Reordering(int* buffer);

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

int* IDCT_DEQUANTI(int* buffer)
{
	int* new_buffer = new int[YSize];
	int matrix[8][8] = { 0, };
	int** IDCT = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		IDCT[i] = new int[8];
	}
	int** DeQuanti = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		DeQuanti[i] = new int[8];
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
				//printf("%d ", matrix[i][j]);
			}
			//printf("\n");
		}
		DeQuanti = DeQuantization(matrix);
		//for (int i = 0; i < 8; i++)
		//{
		//	for (int j = 0; j < 8; j++)
		//	{
		//		//printf("%d ", DeQuanti[i][j]);
		//	}
		//	//printf("\n");
		//}
		IDCT = BDCT(DeQuanti);

		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				new_buffer[Width * (i + 8 * n) + j + 8 * m] = IDCT[i][j];
				/*if (k == 1580) {
					printf("%d ", new_buffer[Width * (i + 8 * n) + j + 8 * m]);
				}*/
			}
			//printf("\n");
		}
		//printf("\n");
		for (int i = 0; i < 8; i++)
			delete[] IDCT[i];
		delete[] IDCT;
		for (int i = 0; i < 8; i++)
			delete[] DeQuanti[i];
		delete[] DeQuanti;
	}
	return new_buffer;
}

int getAbit(unsigned int x, int n) { // getbit()
	
	return (x & (1 << (31 - n))) >> (31 - n);
}

int* entropy_decoding(unsigned int* decode_buffer ,int fileLength, unsigned char* MPM_buffer_, unsigned char* x_buf, unsigned char* y_buf) {
	int* new_buffer = new int[YSize * 90]; // 프레임 정하기
	int num;
	num = (int)(Frame_size / intra_period);
	unsigned char* MPM_buffer = new unsigned char[1584 * num];//* round(Frame_size / intra_period)
	unsigned char* X_buffer = new unsigned char[1584 * (Frame_size - num)];
	unsigned char* Y_buffer = new unsigned char[1584 * (Frame_size - num)];//1584 * (Frame_size - round(Frame_size / intra_period))
	int matrix[8][8] = { 0, };
	
	//inter
	int X_bufword = 0;
	int X_bufword_len = 0;
	int Y_bufword = 0;
	int Y_bufword_len = 0;
	//intra
	int MPMword = 0;
	int MPMword_len = 0;
	//entropy
	int codeword_len = 0;
	int codeword = 0;
	int data_len = 0;
	int dataword = 0;
	int sign = 0;
	int bit = 0;
	int sign_bit = 0;
	int data_bit = 0;
	int min_val = 0;
	int category = 0;

	// matrix count
	int y = 0;
	int x = 0;
	int n; // size of int = 32bit
	// flag type
	int MPM_flag = 1;
	int code_flag = 0;
	int sign_flag = 0;
	int data_flag = 0;
	int X_flag = 0;
	int Y_flag = 0;

	int block_X = 0;
	int block_Y = 0;
	int f = 0; // frame count
	int z = 0; // MPM_buffer index
	int p = 0; //inter_buffer index
	MPM_buffer = MPM_buffer_;
	X_buffer = x_buf;
	Y_buffer = y_buf;
	for (int m = 0; m < (fileLength / 4) ; m++)
	{
		n = 0;
		while (n < 32) {
			
			if (y == 7 && x == 8) {
				y = 0;
				x = 0;
				if (f % intra_period == 0) {
					if (MPMword > 2 ||  MPMword < 0) {
						printf("error");
					}
					MPM_buffer[z] = (unsigned char)MPMword;
					z++;
					MPMword = 0;
					MPM_flag = 1;
				}
				else {
					if (X_bufword > 15 || Y_bufword > 15 || Y_bufword < 0 || X_bufword < 0) {
						printf("error");
					}
					X_buffer[p] = (unsigned char)X_bufword;
					Y_buffer[p] = (unsigned char)Y_bufword;
					p++;
					X_bufword = 0;
					Y_bufword = 0;
					X_flag = 1;
				}
				
				for (int i = 0; i < 8; i++)
				{
					for (int j = 0; j < 8; j++)
					{
						new_buffer[YSize * f +Width * (i + 8 * block_Y) + j + 8 * block_X] = matrix[i][j];
						//printf("%d ", matrix[i][j]);
					}
					//printf("\n");
				}
				if (n != 0) {
					m++;
				}
				n = 0;
				block_X++;
				
				code_flag = 0;
				data_flag = 0;
				if (block_Y == 35 && block_X == 44) {
					block_X = 0;
					block_Y = 0;
					f += 1;
					if (f % intra_period == 0) {
						X_flag = 0;
						MPM_flag = 1;
					}
					else {
						MPM_flag = 0;
						X_flag = 1;
					}
					//Frame[0]++;
				}
				if (block_X == 44) {
					block_Y++;
					block_X = 0;
				}
			}
			if (x == 8) {
				y++;
				x = 0;
			}
			if (MPM_flag == 1) {
				bit = getAbit(decode_buffer[m], n);
				n++;
				MPMword = (MPMword << 1) + bit;
				MPMword_len += 1;
				if (MPMword_len == 2) {

					MPMword_len = 0;
					MPM_flag = 0;
					code_flag = 1;
				}
			}
			else if (X_flag == 1) {
				bit = getAbit(decode_buffer[m], n);
				n++;
				X_bufword = (X_bufword << 1) + bit;
				X_bufword_len += 1;
				if (X_bufword_len == 4) {

					X_bufword_len = 0;
					X_flag = 0;
					Y_flag = 1;
				}
			}
			else if (Y_flag == 1) {
				bit = getAbit(decode_buffer[m], n);
				n++;
				Y_bufword = (Y_bufword << 1) + bit;
				Y_bufword_len += 1;
				if (Y_bufword_len == 4) {

					Y_bufword_len = 0;
					Y_flag = 0;
					code_flag = 1;
				}
			}
			else if (code_flag == 1) {
				bit = getAbit(decode_buffer[m], n);
				n++;
				codeword = (codeword << 1) + bit;
				codeword_len++;
				if (codeword == 0 && codeword_len == 2) { // category 0
					category = 0;
					codeword_len = 0;
					codeword = 0;
					data_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 2 && codeword_len == 3) { // category 1
					category = 1;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 3 && codeword_len == 3) {
					category = 2;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 4 && codeword_len == 3) {
					category = 3;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 5 && codeword_len == 3) {
					category = 4;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 6 && codeword_len == 3) {
					category = 5;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 14 && codeword_len == 4) {
					category = 6;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 30 && codeword_len == 5) {
					category = 7;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 62 && codeword_len == 6) {
					category = 8;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 126 && codeword_len == 7) {
					category = 9;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 254 && codeword_len == 8) {
					category = 10;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 510 && codeword_len == 9) {
					category = 11;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 1022 && codeword_len == 10) {
					category = 12;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword == 2046 && codeword_len == 11) {
					category = 13;
					codeword_len = 0;
					codeword = 0;
					sign_flag = 1;
					code_flag = 0;
				}
				else if (codeword > 4094 || codeword_len > 12) {
					printf("error");
				}
			}
			else if (sign_flag == 1) {
				sign_bit = getAbit(decode_buffer[m], n);
				n++;
				if (sign_bit == 0) {
					sign = -1;
				}
				else {
					sign = 1;
				}
				sign_flag = 0;
				data_flag = 1;
			}
			else if (data_flag == 1) {
				switch (category)
				{
				case 0:
					matrix[y][x] = 0;
					x++;
					data_flag = 0;
					code_flag = 1;
					break;
				case 1:
					matrix[y][x] = sign;
					x++;
					data_flag = 0;
					code_flag = 1;
					break;
				case 2:
					data_bit = getAbit(decode_buffer[m], n);
					n++;
					dataword = (dataword << 1) + data_bit;
					data_len++;
					min_val = 2;
					if (data_len == 1) {
						dataword += min_val;
						matrix[y][x] = sign * dataword;
						x++;
						dataword = 0;
						data_len = 0;
						data_flag = 0;
						code_flag = 1;
					}
					break;
				case 3:
					data_bit = getAbit(decode_buffer[m], n);
					n++;
					dataword = (dataword << 1) + data_bit;
					data_len++;
					min_val = 4;
					if (data_len == 2) {
						dataword += min_val;
						matrix[y][x] = sign * dataword;
						x++;
						dataword = 0;
						data_len = 0;
						data_flag = 0;
						code_flag = 1;
					}
					break;
				case 4:
					data_bit = getAbit(decode_buffer[m], n);
					n++;
					dataword = (dataword << 1) + data_bit;
					data_len++;
					min_val = 8;
					if (data_len == 3) {
						dataword += min_val;
						matrix[y][x] = sign * dataword;
						x++;
						dataword = 0;
						data_len = 0;
						data_flag = 0;
						code_flag = 1;
					}
					break;
				case 5:
					data_bit = getAbit(decode_buffer[m], n);
					n++;
					dataword = (dataword << 1) + data_bit;
					data_len++;
					min_val = 16;
					if (data_len == 4) {
						dataword += min_val;
						matrix[y][x] = sign * dataword;
						x++;
						dataword = 0;
						data_len = 0;
						data_flag = 0;
						code_flag = 1;
					}
					break;
				case 6:
					data_bit = getAbit(decode_buffer[m], n);
					n++;
					dataword = (dataword << 1) + data_bit;
					data_len++;
					min_val = 32;
					if (data_len == 5) {
						dataword += min_val;
						matrix[y][x] = sign * dataword;
						x++;
						dataword = 0;
						data_len = 0;
						data_flag = 0;
						code_flag = 1;
					}
					break;
				case 7:
					data_bit = getAbit(decode_buffer[m], n);
					n++;
					dataword = (dataword << 1) + data_bit;
					data_len++;
					min_val = 64;
					if (data_len == 6) {
						dataword += min_val;
						matrix[y][x] = sign * dataword;
						x++;
						dataword = 0;
						data_len = 0;
						data_flag = 0;
						code_flag = 1;
					}
					break;
				case 8:
					data_bit = getAbit(decode_buffer[m], n);
					n++;
					dataword = (dataword << 1) + data_bit;
					data_len++;
					min_val = 128;
					if (data_len == 7) {
						dataword += min_val;
						matrix[y][x] = sign * dataword;
						x++;
						dataword = 0;
						data_len = 0;
						data_flag = 0;
						code_flag = 1;
					}
					break;
				case 9:
					data_bit = getAbit(decode_buffer[m], n);
					n++;
					dataword = (dataword << 1) + data_bit;
					data_len++;
					min_val = 256;
					if (data_len == 8) {
						dataword += min_val;
						matrix[y][x] = sign * dataword;
						x++;
						dataword = 0;
						data_len = 0;
						data_flag = 0;
						code_flag = 1;
					}
					break;
				case 10:
					data_bit = getAbit(decode_buffer[m], n);
					n++;
					dataword = (dataword << 1) + data_bit;
					data_len++;
					min_val = 512;
					if (data_len == 9) {
						dataword += min_val;
						matrix[y][x] = sign * dataword;
						x++;
						dataword = 0;
						data_len = 0;
						data_flag = 0;
						code_flag = 1;
					}
					break;
				case 11:
					data_bit = getAbit(decode_buffer[m], n);
					n++;
					dataword = (dataword << 1) + data_bit;
					data_len++;
					min_val = 1024;
					if (data_len == 10) {
						dataword += min_val;
						matrix[y][x] = sign * dataword;
						x++;
						dataword = 0;
						data_len = 0;
						data_flag = 0;
						code_flag = 1;
					}
					break;
				case 12:
					data_bit = getAbit(decode_buffer[m], n);
					n++;
					dataword = (dataword << 1) + data_bit;
					data_len++;
					min_val = 2048;
					if (data_len == 11) {
						dataword += min_val;
						matrix[y][x] = sign * dataword;
						x++;
						dataword = 0;
						data_len = 0;
						data_flag = 0;
						code_flag = 1;
					}
					break;
				case 13:
					data_bit = getAbit(decode_buffer[m], n);
					n++;
					dataword = (dataword << 1) + data_bit;
					data_len++;
					min_val = 4096;
					if (data_len == 12) {
						dataword += min_val;
						matrix[y][x] = sign * dataword;
						x++;
						dataword = 0;
						data_len = 0;
						data_flag = 0;
						code_flag = 1;
					}
					break;
				default:
					printf("error");
					break;
				}
			}

		}
		
	}
	MPM_buffer_ = MPM_buffer;
	x_buf = X_buffer;
	y_buf = Y_buffer;

	return new_buffer;
}

int** BDCT(int** matrix) {
	int** IDCT3 = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		IDCT3[i] = new int[8];
	}
	double Cu, Cv = 0;
	double IDCT1[8][8] = { 0, };
	double IDCT2[8][8] = { 0, };
	//---------------idct1----------------------
	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			for (int v = 0; v < 8; v++)
			{
				if (v == 0) {
					Cv = sqrt(1.0 / 8.0);
				}
				else {
					Cv = sqrt(1.0 / 4.0);
				}
				IDCT1[y][x] += Cv * matrix[x][v] * cos((2.0 * y + 1.0) * v * PI / 16.0);
			}
		}
	}

	//---------------idct2----------------------
	for (int x = 0; x < 8; x++)
	{
		for (int y = 0; y < 8; y++)
		{
			for (int u = 0; u < 8; u++)
			{
				if (u == 0) {
					Cu = sqrt(1.0 / 8.0);
				}
				else {
					Cu = sqrt(1.0 / 4.0);
				}
				IDCT2[x][y] += Cu * IDCT1[y][u] * cos((2.0 * x + 1.0) * u * PI / 16.0);
			}
			IDCT3[x][y] = IDCT2[x][y];
		}
	}
	//=========================== end ====================================================
	return IDCT3;
}

int** DeQuantization(int matrix[8][8]) {
	int** DeQuantizer = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		DeQuantizer[i] = new int[8];
	}
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++)
		{
			if (i == 0 && j == 0) {
				DeQuantizer[i][j] = matrix[i][j] * QP_DC;
			}
			else {
				DeQuantizer[i][j] = matrix[i][j] * QP_AC;
			}
		}
	}
	/*for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			printf("%f\t", DeQuantizer[i][j]);

		}
		printf("\n");
	}*/
	return DeQuantizer;
}

unsigned char* Reverse_Motion_Estimation(int* cur_buffer, unsigned char* rec_buffer, unsigned char* X_buf, unsigned char* Y_buf) {
	unsigned char* new_buffer = new unsigned char[YSize];
	unsigned char cur_matrix[8][8] = { 0, };
	unsigned char rec_matrix[8][8] = { 0, };
	int n = -1;
	int m = 0;
	for (int k = 0; k < MPM_BUF_Size; k++)
	{
		if (k % 44 == 0)
		{
			n++;
			m = 0;
		}
		else // k가 1일때 부터 m값 증가
			m++;
		//8X8 블록화
		//현재 매크로 블록
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{

				cur_matrix[i][j] = cur_buffer[Width * (i + 8 * n) + j + 8 * m];
			}
		}
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				rec_matrix[i][j] = rec_buffer[Width * ((Y_buf[k] - 7) + i + 8 * n) + j + 8 * m + (X_buf[k] - 7)];
				new_buffer[Width * (i + 8 * n) + j + 8 * m] = cur_matrix[i][j] + rec_matrix[i][j];
			}
		}

	}
	return new_buffer;
}

int** Reverse_Intra_Mode0(int matrix[8][8], int k, int* V_ref, int* H_ref) {
	int** MODE0_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE0_matrix[i] = new int[8];
	}
	int mean = 0;

	if (k < 44 && ((k % 44) == 0)) {//1번째 행과 열 블록 수행
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE0_matrix[i][j] = matrix[i][j] + 128;
			}
		}
	}
	else if (k < 44) {//1번째 행의 블록 수행
		mean = 0;
		for (int m = 0; m < 8; m++)
		{
			mean += V_ref[8 * ((k % 44) - 1) + m] + 128;
		}
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE0_matrix[i][j] = matrix[i][j] + mean / 16.0;
			}
		}
	}
	else if (k % 44 == 0) {//1번째 열의 블록 수행
		mean = 0;
		for (int m = 0; m < 8; m++) {
			mean += H_ref[8 * (k % 44) + m] + 128;
		}
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE0_matrix[i][j] = matrix[i][j] + mean / 16.0;
			}
		}
	}
	else {// k > 44 || ((k % 44) != 0) 인 블록
		mean = 0;
		for (int m = 0; m < 8; m++)
		{
			mean += H_ref[8 * (k % 44) + m] + V_ref[8 * ((k % 44) - 1) + m];
		}
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE0_matrix[i][j] = matrix[i][j] + mean / 16.0;
			}
		}
	}
	return MODE0_matrix;
}

int** Reverse_Intra_Mode1(int matrix[8][8], int k, int* V_ref, int* H_ref) {
	int** MODE1_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE1_matrix[i] = new int[8];
	}
	int vertical = 0;
	if (k < 44) {//1번째 행 블록 수행
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE1_matrix[i][j] = matrix[i][j] + 128;
			}
		}
	}
	else {// k>44인 블록 수행
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE1_matrix[i][j] = matrix[i][j] + H_ref[8 * (k % 44) + j];
			}
		}
	}
	return MODE1_matrix;
}

int** Reverse_Intra_Mode2(int matrix[8][8], int k, int* V_ref, int* H_ref) {
	int** MODE2_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE2_matrix[i] = new int[8];
	}

	if (k % 44 == 0) {//1번째 열 블록 수행
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE2_matrix[i][j] = matrix[i][j] + 128;
			}
		}
	}
	else {// k%44 != 0 인 블록 수행
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE2_matrix[i][j] = matrix[i][j] + V_ref[8 * ((k % 44) - 1) + i];
			}
		}
	}
	return MODE2_matrix;
}

unsigned char* Reverse_Intra_Prediction(int* buffer, int f, unsigned char* MPM_buffer) {
	unsigned char* new_buffer = new unsigned char[YSize];

	int* V_reference_buffer = new int[Width];
	int* H_reference_buffer = new int[Width];
	int matrix[8][8] = { 0, };

	int** MODE0_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE0_matrix[i] = new int[8];
	}

	int** MODE1_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE1_matrix[i] = new int[8];
	}

	int** MODE2_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE2_matrix[i] = new int[8];
	}

	int n = -1;
	int m = 0;
	for (int k = 0; k < MPM_BUF_Size; k++)
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
				//printf("%d ", buffer[Width * (i + 8 * n) + j + 8 * m]);
				matrix[i][j] = buffer[Width * (i + 8 * n) + j + 8 * m];
			}
			//printf("\n");
		}
		//printf("\n");

		/*for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				printf("%d ", MODE0_matrix[i][j]);
			}
			printf("\n");
		}*/


		//-----------------DPCM select------------------------------------
		int select = 0;
		if (MPM_buffer[k] == 0) {
			MODE0_matrix = Reverse_Intra_Mode0(matrix, k, V_reference_buffer, H_reference_buffer);
			select = 1;
			for (int r = 0; r < 8; r++)
			{
				V_reference_buffer[r + 8 * m] = MODE0_matrix[r][7];
				H_reference_buffer[r + 8 * m] = MODE0_matrix[7][r];
			}
		}
		else if (MPM_buffer[k] == 1) {
			MODE1_matrix = Reverse_Intra_Mode1(matrix, k, V_reference_buffer, H_reference_buffer);
			select = 2;
			for (int r = 0; r < 8; r++)
			{
				V_reference_buffer[r + 8 * m] = MODE1_matrix[r][7];
				H_reference_buffer[r + 8 * m] = MODE1_matrix[7][r];
			}
		}
		else if (MPM_buffer[k] == 2) {
			MODE2_matrix = Reverse_Intra_Mode2(matrix, k, V_reference_buffer, H_reference_buffer);
			select = 3;
			for (int r = 0; r < 8; r++)
			{
				V_reference_buffer[r + 8 * m] = MODE2_matrix[r][7];
				H_reference_buffer[r + 8 * m] = MODE2_matrix[7][r];
			}
		}
		else {
			printf("error");
			return 0;
		}

		if (select == 1) {
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					if (abs(MODE0_matrix[i][j]) > 255) {
						MODE0_matrix[i][j] = 255;
					}
					new_buffer[Width * (i + 8 * n) + j + 8 * m] = (unsigned char)abs(MODE0_matrix[i][j]);
				}
			}
		}
		else if (select == 2) {
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					if (abs(MODE1_matrix[i][j]) > 255) {
						MODE1_matrix[i][j] = 255;
					}
					new_buffer[Width * (i + 8 * n) + j + 8 * m] = (unsigned char)abs(MODE1_matrix[i][j]);
				}
			}
		}
		else if (select == 3) {
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					if (abs(MODE2_matrix[i][j]) > 255) {
						MODE2_matrix[i][j] = 255;
					}
					new_buffer[Width * (i + 8 * n) + j + 8 * m] = (unsigned char)abs(MODE2_matrix[i][j]);
				}
			}
		}

	}
	for (int i = 0; i < 8; i++)
		delete[] MODE0_matrix[i];
	delete[] MODE0_matrix;
	for (int i = 0; i < 8; i++)
		delete[] MODE1_matrix[i];
	delete[] MODE1_matrix;
	for (int i = 0; i < 8; i++)
		delete[] MODE2_matrix[i];
	delete[] MODE2_matrix;

	return new_buffer;
}

int** Reverse_DPCM_Mode0(int matrix[8][8], int k, int* V_ref, int* H_ref) {
	int** MODE0_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE0_matrix[i] = new int[8];
	}
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if (k < 44 && ((k % 44) == 0)) {//1번째 행과 열 블록 수행
				if (i == 0) {//1번째 행 픽셀 수행
					if (j > 0) {
						matrix[i][j] = matrix[i][j] + matrix[i][j - 1];
					}
					else {//i==0&&j==0 인 경우
						matrix[i][j] = matrix[i][j] + 128;
					}
				}
				else if (j == 0) { //1번째 열 픽셀 수행 i != 0
					matrix[i][j] = matrix[i][j] + matrix[i - 1][j];
				}
				else {
					matrix[i][j] = matrix[i][j] + (matrix[i - 1][j] + matrix[i][j - 1]) / 2;
				}
			}
			else if (k < 44) {//1번째 행의 블록 수행
				if (i == 0) {//1번째 행 픽셀 수행
					if (j > 0) {
						matrix[i][j] = matrix[i][j] + matrix[i][j - 1];
					}
					else { //i==0 && j==0 인 경우
						matrix[i][j] = matrix[i][j] + V_ref[8 * ((k % 44) - 1) + i];
					}
				}
				else if (j == 0) {//1번째 행 픽셀 수행
					matrix[i][j] = matrix[i][j] + matrix[i - 1][j];
				}
				else {
					matrix[i][j] = matrix[i][j] + (matrix[i - 1][j] + matrix[i][j - 1]) / 2;
				}
			}
			else if (k % 44 == 0) {//1번째 열의 블록 수행
				if (j == 0) { // 1번째 열 픽셀 수행
					if (i > 0) {
						matrix[i][j] = matrix[i][j] + matrix[i - 1][j];
					}
					else { // i ==0 && j == 0
						matrix[i][j] = matrix[i][j] + H_ref[8 * (k % 44) + j]; // 사실상 H_ref[0]
					}
				}
				else if (i == 0) { // 1번째 행 픽셀 수행
					matrix[i][j] = matrix[i][j] + (matrix[i][j - 1] + H_ref[8 * (k % 44) + j]) / 2;
				}
				else {
					matrix[i][j] = matrix[i][j] + (matrix[i - 1][j] + matrix[i][j - 1]) / 2;
				}
			}
			else {// k > 44 || ((k % 44) != 0) 인 블록
				if (i == 0) { // 1번째 열 픽셀 수행
					if (j > 0) {
						matrix[i][j] = matrix[i][j] + (matrix[i][j - 1] + H_ref[8 * (k % 44) + j]) / 2;
					}
					else { // i ==0 && j == 0
						matrix[i][j] = matrix[i][j] + (H_ref[8 * ((k % 44) + j)] + V_ref[8 * ((k % 44) - 1) + i]) / 2;//사실상 j = 0
					}
				}
				else if (j == 0) {// 1번째 행 픽셀 수행
					matrix[i][j] = matrix[i][j] + (matrix[i - 1][j] + V_ref[8 * ((k % 44) - 1) + i]) / 2;
				}
				else { // i > 0 && j > 0
					matrix[i][j] = matrix[i][j] + (matrix[i][j - 1] + matrix[i - 1][j]) / 2;
				}
			}
		}
	}
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			MODE0_matrix[i][j] = matrix[i][j];
		}
	}
	return MODE0_matrix;
}

int** Reverse_DPCM_Mode1(int matrix[8][8], int k, int* V_ref, int* H_ref) {
	int** MODE1_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE1_matrix[i] = new int[8];
	}
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if (k < 44) {//1번째 행 블록 수행
				if (k == 0 && i == 0 && j == 0) {
					matrix[i][j] = matrix[i][j] + 128;
				}
				else if (i == 0) {//1번째 행 픽셀 수행
					if (j > 0) {
						matrix[i][j] = matrix[i][j] + matrix[i][j - 1];
					}
					else {//i==0&&j==0 인경우
						matrix[i][j] = matrix[i][j] + V_ref[8 * ((k % 44) - 1) + i];
					}
				}
				else {
					matrix[i][j] = matrix[i][j] + matrix[i - 1][j];
				}
			}
			else {// k>44인 블록 수행
				if (i == 0) {//1번째 행 블록 수행
					matrix[i][j] = matrix[i][j] + H_ref[8 * (k % 44) + j]; // H_ref는 352길이의 행
				}
				else {
					matrix[i][j] = matrix[i][j] + matrix[i - 1][j];
				}
			}
		}
	}
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			MODE1_matrix[i][j] = matrix[i][j];
		}
	}
	return MODE1_matrix;
}

int** Reverse_DPCM_Mode2(int matrix[8][8], int k, int* V_ref, int* H_ref) {
	int** MODE2_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE2_matrix[i] = new int[8];
	}
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if (k % 44 == 0) {//1번째 열 블록 수행
				if (k == 0 && i == 0 && j == 0) {
					matrix[i][j] = matrix[i][j] + 128;
				}
				else if (j == 0) {//1번째 열 픽셀 수행
					if (i > 0) {
						matrix[i][j] = matrix[i][j] + matrix[i - 1][j];
					}
					else {//i==0&&j==0 인경우
						matrix[i][j] = matrix[i][j] + H_ref[8 * (k % 44) + j];
					}
				}
				else {
					matrix[i][j] = matrix[i][j] + matrix[i][j - 1];
				}
			}
			else {// k%44 != 0 인 블록 수행
				if (j == 0) {//1번째 열 블록 수행
					matrix[i][j] = matrix[i][j] + V_ref[8 * ((k % 44) - 1) + i];
				}
				else {
					matrix[i][j] = matrix[i][j] + matrix[i][j - 1];
				}
			}
		}
	}
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			MODE2_matrix[i][j] = matrix[i][j];
			//printf("%d", matrix[i][j]);
		}
	}
	return MODE2_matrix;
}

int* Reverse_pixel_DPCM(int* buffer, int f) {
	int* new_buffer = new int[YSize];
	int* V_reference_buffer = new int[Width];
	int* H_reference_buffer = new int[Width];
	int matrix[8][8] = { 0, };

	int** MODE0_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE0_matrix[i] = new int[8];
	}

	int** MODE1_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE1_matrix[i] = new int[8];
	}

	int** MODE2_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE2_matrix[i] = new int[8];
	}

	int n = -1;
	int m = 0;
	for (int k = 0; k < MPM_BUF_Size; k++)
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
				//printf("%d ", buffer[Width * (i + 8 * n) + j + 8 * m]);
				matrix[i][j] = buffer[Width * (i + 8 * n) + j + 8 * m];
			}
			//printf("\n");
		}
		//printf("\n");
		switch (pixel_dpcm_select)
		{
		case 0: MODE0_matrix = Reverse_DPCM_Mode0(matrix, k, V_reference_buffer, H_reference_buffer);
			for (int r = 0; r < 8; r++)
			{
				V_reference_buffer[r + 8 * m] = MODE0_matrix[r][7];
				H_reference_buffer[r + 8 * m] = MODE0_matrix[7][r];
			}
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					new_buffer[Width * (i + 8 * n) + j + 8 * m] = MODE0_matrix[i][j];
				}
			}
			break;
		case 1: MODE1_matrix = Reverse_DPCM_Mode0(matrix, k, V_reference_buffer, H_reference_buffer);
			for (int r = 0; r < 8; r++)
			{
				V_reference_buffer[r + 8 * m] = MODE1_matrix[r][7];
				H_reference_buffer[r + 8 * m] = MODE1_matrix[7][r];
			}
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					new_buffer[Width * (i + 8 * n) + j + 8 * m] = MODE1_matrix[i][j];
				}
			}
			break;
		case 2: MODE2_matrix = Reverse_DPCM_Mode0(matrix, k, V_reference_buffer, H_reference_buffer);
			for (int r = 0; r < 8; r++)
			{
				V_reference_buffer[r + 8 * m] = MODE2_matrix[r][7];
				H_reference_buffer[r + 8 * m] = MODE2_matrix[7][r];
			}
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					new_buffer[Width * (i + 8 * n) + j + 8 * m] = MODE2_matrix[i][j];
				}
			}
			break;
		default:
			break;
		}
	}
	for (int i = 0; i < 8; i++)
		delete[] MODE0_matrix[i];
	delete[] MODE0_matrix;
	for (int i = 0; i < 8; i++)
		delete[] MODE1_matrix[i];
	delete[] MODE1_matrix;
	for (int i = 0; i < 8; i++)
		delete[] MODE2_matrix[i];
	delete[] MODE2_matrix;

	return new_buffer;
}

int main() {
	errno_t err, err2, err3, err4;
	FILE* picture;
	FILE* picture2;
	FILE* picture3;
	FILE* picture4;

	int* Frame = 0;
	int* buffer0 = new int[YSize * Frame_size];
	unsigned char* MPM_buffer = new unsigned char[1584 * (int)round(Frame_size / intra_period)];
	unsigned char* X_buf = new unsigned char[1584 * (Frame_size - (int)round(Frame_size / intra_period))];
	unsigned char* Y_buf = new unsigned char[1584 * (Frame_size - (int)round(Frame_size / intra_period))];
	unsigned char* Reconstructed_buf = new unsigned char[YSize];
	unsigned char* MPM_buffer1 = new unsigned char[1584];
	
	int inter = 0;
	int intra = 0;
	err = fopen_s(&picture, "encode_Y_football_cif(352X288)_90f.yuv", "rb");
	if (err == 0) {
		fseek(picture, 0, SEEK_END);
		int fileLength = ftell(picture);
		unsigned int* decode_buffer = new unsigned int[fileLength];
		
		fseek(picture, 0, SEEK_SET);
		fread(decode_buffer, 4, fileLength, picture);
		//for (int i = 0; i < fileLength; i++)
		//{
		//	printf("%X ", decode_buffer[i]); //제대로 받음
		//}
		buffer0 = entropy_decoding(decode_buffer, fileLength, MPM_buffer, X_buf, Y_buf);
		delete[] decode_buffer;
	}
	
	for (int f = 0; f < 90; f++)
	{
		int* buffer1 = new int[YSize];
		int* buffer2 = new int[YSize];
		int* buffer3 = new int[YSize];
		int* buffer4 = new int[YSize];
		int* buffer5 = new int[YSize];
		int* buffer6 = new int[YSize];
		int* buffer7 = new int[YSize];
		int* buffer8 = new int[YSize];
		int* buffer9 = new int[YSize];
		unsigned char* X_buf1 = new unsigned char[1584];
		unsigned char* Y_buf1 = new unsigned char[1584];
		
		////check MPM_buffer
		//for (int k = 0; k < 1584 * 18; k++) {
		//	if (k % 44 == 0) {
		//		printf("\n");
		//	}
		//	if (k % 1584 == 0) {
		//		printf("\n");
		//	}
		//	printf("%d ", MPM_buffer[k]);
		//}
		//printf("\n");
		//printf("\n");
		//check MPM_buffer

		//for (int k = 0; k < 1584 * 72; k++) {
		//	if (k % 44 == 0) {
		//		printf("\n");
		//	}
		//	if (k % 1584 == 0) {
		//		printf("\n");
		//	}
		//	printf("%d ", X_buf[k]);
		//}
		//printf("\n");
		//printf("\n");
		for (int i = 0; i < YSize; i++)
		{
			buffer1[i] = buffer0[i + (f * YSize)];
		}
		if (f % intra_period == 0) {
			for (int i = 0; i < 1584; i++)
			{
				MPM_buffer1[i] = MPM_buffer[i + (intra * 1584)];
			}
			intra+= 1;
			buffer2 = Reverse_Reordering(buffer1);
			buffer3 = Reverse_DC_DPCM(buffer2);
			
			buffer4 = IDCT_DEQUANTI(buffer3);
			buffer5 = Reverse_pixel_DPCM(buffer4, f);
			Reconstructed_buf = Reverse_Intra_Prediction(buffer5, f, MPM_buffer1);

		}
		else {
			for (int i = 0; i < 1584; i++)
			{
				X_buf1[i] = X_buf[i + (inter * 1584)];
				Y_buf1[i] = Y_buf[i + (inter * 1584)];
			}
			inter+= 1;
			buffer6 = Reverse_Reordering(buffer1);
			buffer7 = Reverse_DC_DPCM(buffer6);
			
			buffer8 = IDCT_DEQUANTI(buffer7);
			Reconstructed_buf = Reverse_Motion_Estimation(buffer8, Reconstructed_buf, X_buf1, Y_buf1);
		}

		//err2 = fopen_s(&picture2, "test(352X288)_90f.yuv", "a+b"); // wb대신에 a+b를 사용하는 이유는 파일을 이어쓰기 때문이다.
		//if (err2 == 0) {
		//	fseek(picture2, YSize * f, SEEK_SET);

		//	fwrite(Reconstructed_buf, 1, YSize, picture2);

		//	fclose(picture2);
		//}

		err3 = fopen_s(&picture3, "decode_Y_football_cif(352X288)_90f.yuv", "a+b"); // wb대신에 a+b를 사용하는 이유는 파일을 이어쓰기 때문이다.
		if (err3 == 0) {
			fseek(picture3, YSize * f, SEEK_SET);

			fwrite(Reconstructed_buf, 1, YSize, picture3);

			fclose(picture3);
		}
		delete[] buffer2;
		buffer2 = NULL;
		/*delete[] buffer3;
		buffer3 = NULL;*/
		delete[] buffer4;
		buffer4 = NULL;
		delete[] buffer5;
		buffer5 = NULL;
		delete[] buffer6;
		buffer6 = NULL;
		/*delete[] buffer7;
		buffer7 = NULL;*/
		delete[] buffer8;
		buffer8 = NULL;
		delete[] X_buf1;
		X_buf1 = NULL;
		delete[] Y_buf1;
		Y_buf1 = NULL;
		//delete[] Reconstructed_buf;
		//Reconstructed_buf = NULL;
	}
	/*delete[] X_buf;
	delete[] Y_buf;
	delete[] X_buf1;
	delete[] Y_buf1;*/
	Reconstructed_buf = NULL;
	delete[] Reconstructed_buf;
	Reconstructed_buf = NULL;
	delete[] MPM_buffer;
	MPM_buffer = NULL;
	delete[] buffer0;
	buffer0 = NULL;
	return 0;
}