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
//인트라
int** Intra_Mode0(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
int** Intra_Mode1(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
int** Intra_Mode2(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
unsigned char* Intra_Prediction(unsigned char*, int, unsigned char*);

//역 인트라
unsigned char** Reverse_Intra_Mode0(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
unsigned char** Reverse_Intra_Mode1(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
unsigned char** Reverse_Intra_Mode2(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
unsigned char* Reverse_Intra_Prediction(unsigned char*, int, unsigned char*);

//pixel DPCM
int** DPCM_Mode0(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
int** DPCM_Mode0(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
int** DPCM_Mode0(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
unsigned char* pixel_DPCM(unsigned char*, int);

//역 pixel DPCM
unsigned char** Reverse_DPCM_Mode0(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
unsigned char** Reverse_DPCM_Mode1(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
unsigned char** Reverse_DPCM_Mode2(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
unsigned char* Reverse_pixel_DPCM(unsigned char*, int);

//양자화
int** Quantization(double**);
int** DeQuantization(int matrix[8][8]);
//DCT
double** FDCT(double matrix[8][8]);
int** BDCT(int**);
//DC DPCM
int* DC_DPCM(int*);
int** DPCM_Mode1(int matrix[36][44]);
//zigzag scan
int* Reordering_entorpy(int*, int*);
int** ZigZag_Scan(int matrix[8][8]);

int* DCT_QUANTI(unsigned char*);
unsigned char* IDCT_DEQUANTI(int*);
//엔트로피
int* entropy_encoding(int**, int*, int*);

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

int* Reordering_entorpy(int* buffer,int* buffer_stack) {
	int* new_buffer = new int[YSize];
	int* encoding_buffer = (int*)malloc(sizeof(int) * YSize);
	int matrix[8][8] = { 0, };
	int** reorder_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		reorder_matrix[i] = new int[8];
	}
	int* encoding_buffer1 = (int*)malloc(sizeof(int) * 64);
	int a = 0;
	//int b = 0;
	int* buffer_index;
	//int* buffer_stack;
	buffer_index = &a;
	//buffer_stack = &b;
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
		
		encoding_buffer1 = entropy_encoding(reorder_matrix, buffer_index, buffer_stack);
		memcpy(encoding_buffer + (*buffer_stack - *buffer_index), encoding_buffer1, sizeof(int) * (*buffer_index));
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				new_buffer[Width * (i + 8 * n) + j + 8 * m] = reorder_matrix[i][j];
				//printf("%d ", reorder_matrix[i][j]);
			}
			//printf("\n");
		}
		//for (int i = 0; i < *buffer_stack; i++)
		//{
		//	printf("%X ", encoding_buffer[i]);
		//}
		for (int i = 0; i < 8; i++)
			delete[] reorder_matrix[i];
		delete[] reorder_matrix;
	}
	//free(encoding_buffer1);
	realloc(encoding_buffer, sizeof(int) * (*buffer_stack));
	//return new_buffer;
	return encoding_buffer;
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

int** Quantization(double** matrix) {
	int** Quantizer = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		Quantizer[i] = new int[8];
	}
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++)
		{
			if (i == 0 && j == 0) {
				Quantizer[i][j] = round(matrix[i][j] / QP_DC);
			}
			else {
				Quantizer[i][j] = round(matrix[i][j] / QP_AC);
			}
		}
	}
	//for (i = 0; i < 8; i++) {
	//	for (j = 0; j < 8; j++) {
	//		printf("%f\t", Quantizer[i][j]);

	//	}
	//	printf("\n");
	//}
	return Quantizer;
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
double** FDCT(double matrix[8][8]) {
	double** DCT4 = new double* [8];
	for (int i = 0; i < 8; i++)
	{
		DCT4[i] = new double[8];
	}
	double Cu, Cv = 0;
	double DCT1[8][8] = { 0, };
	double DCT2[8][8] = { 0, };
	double DCT3[8][8] = { 0, };

	Cu, Cv = 0;
	//---------------dct1----------------------
	for (int v = 0; v < 8; v++)
	{
		if (v == 0) {
			Cv = sqrt(1 / 8.0);
		}
		else {
			Cv = sqrt(1 / 4.0);
		}
		for (int u = 0; u < 8; u++)
		{
			for (int y = 0; y < 8; y++)
			{
				DCT1[v][u] += matrix[u][y] * cos((2.0 * y + 1.0) * v * PI / (16.0));
			}
			DCT2[v][u] = Cv * DCT1[v][u];
		}
	}
	//---------------dct2----------------------
	for (int u = 0; u < 8; u++)
	{
		if (u == 0) {
			Cu = sqrt(1 / 8.0);
		}
		else {
			Cu = sqrt(1 / 4.0);
		}
		for (int v = 0; v < 8; v++) {

			for (int x = 0; x < 8; x++)
			{
				DCT3[u][v] += DCT2[v][x] * cos((2.0 * x + 1.0) * u * PI / 16.0);
			}

			DCT4[u][v] = Cu * DCT3[u][v];
		}
	}
	return DCT4;
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
int* DCT_QUANTI(unsigned char* buffer)
{
	int* new_buffer = new int[YSize];
	double matrix[8][8] = { 0, };
	double** DCT = new double* [8];
	for (int i = 0; i < 8; i++)
	{
		DCT[i] = new double[8];
	}
	int** Quanti = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		Quanti[i] = new int[8];
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
				matrix[i][j] = (double)buffer[Width * (i + 8 * n) + j + 8 * m];
			}
		}
		DCT = FDCT(matrix);
		Quanti = Quantization(DCT);
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				new_buffer[Width * (i + 8 * n) + j + 8 * m] = Quanti[i][j];
			}
		}
		for (int i = 0; i < 8; i++)
			delete[] DCT[i];
		delete[] DCT;
		for (int i = 0; i < 8; i++)
			delete[] Quanti[i];
		delete[] Quanti;
	}
	return new_buffer;
}

unsigned char* IDCT_DEQUANTI(int* buffer)
{
	unsigned char* new_buffer = new unsigned char[YSize];
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
			}
		}
		DeQuanti = DeQuantization(matrix);
		IDCT = BDCT(DeQuanti);

		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				new_buffer[Width * (i + 8 * n) + j + 8 * m] = (unsigned char)IDCT[i][j];
			}
		}
		for (int i = 0; i < 8; i++)
			delete[] IDCT[i];
		delete[] IDCT;
		for (int i = 0; i < 8; i++)
			delete[] DeQuanti[i];
		delete[] DeQuanti;
	}
	return new_buffer;
}

int** DPCM_Mode0(unsigned char matrix[8][8], int k, unsigned char* V_ref, unsigned char* H_ref) {
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
						MODE0_matrix[i][j] = matrix[i][j] - matrix[i][j - 1];
					}
					else {//i==0&&j==0 인 경우
						MODE0_matrix[i][j] = matrix[i][j] - 128;
					}
				}
				else if (j == 0) { //1번째 열 픽셀 수행 i != 0
					MODE0_matrix[i][j] = matrix[i][j] - matrix[i - 1][j];
				}
				else {
					MODE0_matrix[i][j] = matrix[i][j] - (matrix[i - 1][j] + matrix[i][j - 1]) / 2;
				}
			}
			else if (k < 44) {//1번째 행의 블록 수행
				if (i == 0) {//1번째 행 픽셀 수행
					if (j > 0) {
						MODE0_matrix[i][j] = matrix[i][j] - matrix[i][j - 1];
					}
					else { //i==0 && j==0 인 경우
						MODE0_matrix[i][j] = matrix[i][j] - V_ref[8 * ((k % 44) - 1) + i];
					}
				}
				else if (j == 0) {//1번째 행 픽셀 수행
					MODE0_matrix[i][j] = matrix[i][j] - matrix[i - 1][j];
				}
				else {
					MODE0_matrix[i][j] = matrix[i][j] - (matrix[i - 1][j] + matrix[i][j - 1]) / 2;
				}
			}
			else if (k % 44 == 0) {//1번째 열의 블록 수행
				if (j == 0) { // 1번째 열 픽셀 수행
					if (i > 0) {
						MODE0_matrix[i][j] = matrix[i][j] - matrix[i - 1][j];
					}
					else { // i ==0 && j == 0
						MODE0_matrix[i][j] = matrix[i][j] - H_ref[8 * (k % 44) + j]; // 사실상 H_ref[0]
					}
				}
				else if (i == 0) { // 1번째 행 픽셀 수행
					MODE0_matrix[i][j] = matrix[i][j] - (matrix[i][j - 1] + H_ref[8 * (k % 44) + j]) / 2;
				}
				else {
					MODE0_matrix[i][j] = matrix[i][j] - (matrix[i - 1][j] + matrix[i][j - 1]) / 2;
				}
			}
			else {// k > 44 || ((k % 44) != 0) 인 블록
				if (i == 0) { // 1번째 열 픽셀 수행
					if (j > 0) {
						MODE0_matrix[i][j] = matrix[i][j] - (matrix[i][j - 1] + H_ref[8 * (k % 44) + j]) / 2;
					}
					else { // i ==0 && j == 0
						MODE0_matrix[i][j] = matrix[i][j] - (H_ref[8 * ((k % 44) + j)] + V_ref[8 * ((k % 44) - 1) + i]) / 2;//사실상 j = 0
					}
				}
				else if (j == 0) {// 1번째 행 픽셀 수행
					MODE0_matrix[i][j] = matrix[i][j] - (matrix[i - 1][j] + V_ref[8 * ((k % 44) - 1) + i]) / 2;
				}
				else { // i > 0 && j > 0
					MODE0_matrix[i][j] = matrix[i][j] - (matrix[i][j - 1] + matrix[i - 1][j]) / 2;
				}
			}
		}
	}
	return MODE0_matrix;
}

int** DPCM_Mode1(unsigned char matrix[8][8], int k, unsigned char* V_ref, unsigned char* H_ref) {
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
					MODE1_matrix[i][j] = matrix[i][j] - 128;
				}
				else if (i == 0) {//1번째 행 픽셀 수행
					if (j > 0) {
						MODE1_matrix[i][j] = matrix[i][j] - matrix[i][j - 1];
					}
					else {//i==0&&j==0 인경우
						MODE1_matrix[i][j] = matrix[i][j] - V_ref[8 * ((k % 44) - 1) + i];
					}
				}
				else {
					MODE1_matrix[i][j] = matrix[i][j] - matrix[i - 1][j];
				}
			}
			else {// k>44인 블록 수행
				if (i == 0) {//1번째 행 블록 수행
					MODE1_matrix[i][j] = matrix[i][j] - H_ref[8 * (k % 44) + j]; // H_ref는 352길이의 행
				}
				else {
					MODE1_matrix[i][j] = matrix[i][j] - matrix[i - 1][j];
				}
			}
		}
	}
	return MODE1_matrix;
}

int** DPCM_Mode2(unsigned char matrix[8][8], int k, unsigned char* V_ref, unsigned char* H_ref) {
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
					MODE2_matrix[i][j] = matrix[i][j] - 128;
				}
				else if (j == 0) {//1번째 열 픽셀 수행
					if (i > 0) {
						MODE2_matrix[i][j] = matrix[i][j] - matrix[i - 1][j];
					}
					else {//i==0&&j==0 인경우
						MODE2_matrix[i][j] = matrix[i][j] - H_ref[8 * (k % 44) + j];
					}
				}
				else {
					MODE2_matrix[i][j] = matrix[i][j] - matrix[i][j - 1];
				}
			}
			else {// k%44 != 0 인 블록 수행
				if (j == 0) {//1번째 열 블록 수행
					MODE2_matrix[i][j] = matrix[i][j] - V_ref[8 * ((k % 44) - 1) + i];
				}
				else {
					MODE2_matrix[i][j] = matrix[i][j] - matrix[i][j - 1];
				}
			}
		}
	}
	return MODE2_matrix;
}

unsigned char* pixel_DPCM(unsigned char* buffer, int f) {
	unsigned char* new_buffer = new unsigned char[YSize];
	unsigned char* V_reference_buffer = new unsigned char[Width];
	unsigned char* H_reference_buffer = new unsigned char[Width];
	unsigned char matrix[8][8] = { 0, };
	//int** MODE0_matrix = new int* [8];
	//for (int i = 0; i < 8; i++)
	//{
	//	MODE0_matrix[i] = new int[8];
	//}
	int** MODE1_matrix = new int* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE1_matrix[i] = new int[8];
	}
	//int** MODE2_matrix = new int* [8];
	//for (int i = 0; i < 8; i++)
	//{
	//	MODE2_matrix[i] = new int[8];
	//}
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

		//MODE0_matrix = DPCM_Mode0(matrix, k, V_reference_buffer, H_reference_buffer);
		MODE1_matrix = DPCM_Mode1(matrix, k, V_reference_buffer, H_reference_buffer);
		//MODE2_matrix = DPCM_Mode2(matrix, k, V_reference_buffer, H_reference_buffer);
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				//new_buffer[Width * (i + 8 * n) + j + 8 * m] = (unsigned char)MODE0_matrix[i][j];
				new_buffer[Width * (i + 8 * n) + j + 8 * m] = (unsigned char)MODE1_matrix[i][j];
				//new_buffer[Width * (i + 8 * n) + j + 8 * m] = (unsigned char)MODE2_matrix[i][j];
			}
		}

		//-----------------reference buffer-------------------------------
		for (int r = 0; r < 8; r++)
		{
			V_reference_buffer[r + 8 * m] = matrix[r][7];
			H_reference_buffer[r + 8 * m] = matrix[7][r];
		}
		/*for (int i = 0; i < 8; i++)
			delete[] MODE0_matrix[i];
		delete[] MODE0_matrix;*/
		for (int i = 0; i < 8; i++)
			delete[] MODE1_matrix[i];
		delete[] MODE1_matrix;
		/*for (int i = 0; i < 8; i++)
			delete[] MODE2_matrix[i];
		delete[] MODE2_matrix;*/
	}
	return new_buffer;
}

int** Intra_Mode0(unsigned char matrix[8][8], int k, unsigned char* V_ref, unsigned char* H_ref) {
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
				MODE0_matrix[i][j] = matrix[i][j] - 128;
			}
		}
	}
	else if (k < 44) {//1번째 행의 블록 수행
		mean = 0;
		for (int m = 0; m < 8; m++)
		{
			mean = V_ref[8 * ((k % 44) - 1) + m] + 128;
		}
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE0_matrix[i][j] = matrix[i][j] - mean / 16.0;
			}
		}
	}
	else if (k % 44 == 0) {//1번째 열의 블록 수행
		mean = 0;
		for (int m = 0; m < 8; m++) {
			mean = H_ref[8 * (k % 44) + m] + 128;
		}
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE0_matrix[i][j] = matrix[i][j] - mean / 16.0;
			}
		}
	}
	else {// k > 44 || ((k % 44) != 0) 인 블록
		mean = 0;
		for (int m = 0; m < 8; m++)
		{
			mean = H_ref[8 * (k % 44) + m] + V_ref[8 * ((k % 44) - 1) + m];
		}
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE0_matrix[i][j] = matrix[i][j] - mean / 16.0;
			}
		}
	}
	return MODE0_matrix;
}

int** Intra_Mode1(unsigned char matrix[8][8], int k, unsigned char* V_ref, unsigned char* H_ref) {
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
				MODE1_matrix[i][j] = matrix[i][j] - 128;
			}
		}
	}
	else {// k>44인 블록 수행
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE1_matrix[i][j] = matrix[i][j] - H_ref[8 * (k % 44) + j];
			}
		}
	}
	return MODE1_matrix;
}

int** Intra_Mode2(unsigned char matrix[8][8], int k, unsigned char* V_ref, unsigned char* H_ref) {
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
				MODE2_matrix[i][j] = matrix[i][j] - 128;
			}
		}
	}
	else {// k%44 != 0 인 블록 수행
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				MODE2_matrix[i][j] = matrix[i][j] - V_ref[8 * ((k % 44) - 1) + i];
			}
		}
	}
	return MODE2_matrix;
}
unsigned char* Intra_Prediction(unsigned char* buffer, int f, unsigned char* MPM_buffer) {
	unsigned char* new_buffer = new unsigned char[YSize];
	//unsigned char* IncludeMPM_buffer = new unsigned char[YSize + MPM_BUF_Size];
	unsigned char* V_reference_buffer = new unsigned char[Width];
	unsigned char* H_reference_buffer = new unsigned char[Width];
	unsigned char matrix[8][8] = { 0, };
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


		MODE0_matrix = Intra_Mode0(matrix, k, V_reference_buffer, H_reference_buffer);
		MODE1_matrix = Intra_Mode1(matrix, k, V_reference_buffer, H_reference_buffer);
		MODE2_matrix = Intra_Mode2(matrix, k, V_reference_buffer, H_reference_buffer);


		//------------------DPCM 선택 하는 코드------------------------------
		int sum0 = 0;
		int sum1 = 0;
		int sum2 = 0;
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				sum0 += abs(MODE0_matrix[i][j]);
				sum1 += abs(MODE1_matrix[i][j]);
				sum2 += abs(MODE2_matrix[i][j]);
			}
		}

		//----------------- select Intra MODE-------------------------------
		if (min({ sum0, sum1, sum2 }) == sum0) {
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					MPM_buffer[k] = 0;
					new_buffer[Width * (i + 8 * n) + j + 8 * m] = (unsigned char)MODE0_matrix[i][j];

				}
			}
		}
		else if (min({ sum0, sum1, sum2 }) == sum1) {
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					MPM_buffer[k] = 1;
					new_buffer[Width * (i + 8 * n) + j + 8 * m] = (unsigned char)MODE1_matrix[i][j];

				}
			}
		}
		else {
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					MPM_buffer[k] = 2;
					new_buffer[Width * (i + 8 * n) + j + 8 * m] = (unsigned char)MODE2_matrix[i][j];

				}
			}
		}

		//-----------------reference buffer-------------------------------
		for (int r = 0; r < 8; r++)
		{
			V_reference_buffer[r + 8 * m] = matrix[r][7];
			H_reference_buffer[r + 8 * m] = matrix[7][r];
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
	}
	return new_buffer;
}

unsigned char** Reverse_Intra_Mode0(unsigned char matrix[8][8], int k, unsigned char* V_ref, unsigned char* H_ref) {
	unsigned char** MODE0_matrix = new unsigned char* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE0_matrix[i] = new unsigned char[8];
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
			mean = V_ref[8 * ((k % 44) - 1) + m] + 128;
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
			mean = H_ref[8 * (k % 44) + m] + 128;
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
			mean = H_ref[8 * (k % 44) + m] + V_ref[8 * ((k % 44) - 1) + m];
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

unsigned char** Reverse_Intra_Mode1(unsigned char matrix[8][8], int k, unsigned char* V_ref, unsigned char* H_ref) {
	unsigned char** MODE1_matrix = new unsigned char* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE1_matrix[i] = new unsigned char[8];
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

unsigned char** Reverse_Intra_Mode2(unsigned char matrix[8][8], int k, unsigned char* V_ref, unsigned char* H_ref) {
	unsigned char** MODE2_matrix = new unsigned char* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE2_matrix[i] = new unsigned char[8];
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

unsigned char* Reverse_Intra_Prediction(unsigned char* buffer, int f, unsigned char* MPM_buffer) {
	unsigned char* new_buffer = new unsigned char[YSize];
	unsigned char* IncludeMPM_buffer = new unsigned char[YSize + MPM_BUF_Size];
	unsigned char* V_reference_buffer = new unsigned char[Width];
	unsigned char* H_reference_buffer = new unsigned char[Width];
	unsigned char matrix[8][8] = { 0, };

	unsigned char** MODE0_matrix = new unsigned char* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE0_matrix[i] = new unsigned char[8];
	}

	unsigned char** MODE1_matrix = new unsigned char* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE1_matrix[i] = new unsigned char[8];
	}

	unsigned char** MODE2_matrix = new unsigned char* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE2_matrix[i] = new unsigned char[8];
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
					new_buffer[Width * (i + 8 * n) + j + 8 * m] = MODE0_matrix[i][j];
				}
			}
		}
		else if (select == 2) {
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					new_buffer[Width * (i + 8 * n) + j + 8 * m] = MODE1_matrix[i][j];
				}
			}
		}
		else if (select == 3) {
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					new_buffer[Width * (i + 8 * n) + j + 8 * m] = MODE2_matrix[i][j];
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


unsigned char** Reverse_DPCM_Mode0(unsigned char matrix[8][8], int k, unsigned char* V_ref, unsigned char* H_ref) {
	unsigned char** MODE0_matrix = new unsigned char* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE0_matrix[i] = new unsigned char[8];
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

unsigned char** Reverse_DPCM_Mode1(unsigned char matrix[8][8], int k, unsigned char* V_ref, unsigned char* H_ref) {
	unsigned char** MODE1_matrix = new unsigned char* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE1_matrix[i] = new unsigned char[8];
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

unsigned char** Reverse_DPCM_Mode2(unsigned char matrix[8][8], int k, unsigned char* V_ref, unsigned char* H_ref) {
	unsigned char** MODE2_matrix = new unsigned char* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE2_matrix[i] = new unsigned char[8];
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

unsigned char* Reverse_pixel_DPCM(unsigned char* buffer, int f) {
	unsigned char* new_buffer = new unsigned char[YSize];
	unsigned char* V_reference_buffer = new unsigned char[Width];
	unsigned char* H_reference_buffer = new unsigned char[Width];
	unsigned char matrix[8][8] = { 0, };

	/*unsigned char** MODE0_matrix = new unsigned char* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE0_matrix[i] = new unsigned char[8];
	}*/

	unsigned char** MODE1_matrix = new unsigned char* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE1_matrix[i] = new unsigned char[8];
	}

	/*unsigned char** MODE2_matrix = new unsigned char* [8];
	for (int i = 0; i < 8; i++)
	{
		MODE2_matrix[i] = new unsigned char[8];
	}*/

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

		MODE1_matrix = Reverse_DPCM_Mode1(matrix, k, V_reference_buffer, H_reference_buffer);
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

	}
	/*for (int i = 0; i < 8; i++)
		delete[] MODE0_matrix[i];
	delete[] MODE0_matrix;*/
	for (int i = 0; i < 8; i++)
		delete[] MODE1_matrix[i];
	delete[] MODE1_matrix;
	/*for (int i = 0; i < 8; i++)
		delete[] MODE2_matrix[i];
	delete[] MODE2_matrix;*/

	return new_buffer;
}

unsigned char* Motion_Estimation(unsigned char* cur_buffer, unsigned char* rec_buffer, unsigned char* X_buf, unsigned char* Y_buf) {
	unsigned char* new_buffer = new unsigned char[YSize];
	unsigned char cur_matrix[8][8] = { 0, };
	unsigned char rec_matrix[8][8] = { 0, };
	unsigned char new_matrix[8][8] = { 0, };
	int SAD_matrix[8][8] = { 0, };
	int err_size_matrix[15][15] = { 0, };
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
		int min = 100000;
		for (int y = 0; y < 15; y++)
		{
			for (int x = 0; x < 15; x++)
			{
				err_size_matrix[y][x] = 0;
				//이전 매크로 블록
				for (int i = 0; i < 8; i++)
				{
					for (int j = 0; j < 8; j++)
					{
						if (((y - 7) + i + 8 * n) >= 0 && (j + 8 * m + (x - 7)) >= 0 && ((y - 7) + i + 8 * n) < Height && (j + 8 * m + (x - 7)) < Width) {
							rec_matrix[i][j] = rec_buffer[Width * ((y - 7) + i + 8 * n) + j + 8 * m + (x - 7)];
							SAD_matrix[i][j] = cur_matrix[i][j] - rec_matrix[i][j];

							err_size_matrix[y][x] += abs(SAD_matrix[i][j]);

						}
						else {
							err_size_matrix[y][x] += 255;
						}
					}
				}
				if (min > err_size_matrix[y][x]) {
					min = err_size_matrix[y][x];
					X_buf[k] = x;
					Y_buf[k] = y;
					for (int i = 0; i < 8; i++)
					{
						for (int j = 0; j < 8; j++)
						{
							new_matrix[i][j] = rec_matrix[i][j];
						}
					}

				}
			}
		}
		//printf("\n %d ", min);
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				//printf(" %d %d", cur_matrix[i][j], new_matrix[i][j]);
				//printf("\n");
				new_buffer[Width * (i + 8 * n) + j + 8 * m] = cur_matrix[i][j] - new_matrix[i][j];

			}
		}
	}
	return new_buffer;
}

unsigned char* Reverse_Motion_Estimation(unsigned char* cur_buffer, unsigned char* rec_buffer, unsigned char* X_buf, unsigned char* Y_buf) {
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
int* entropy_encoding(int** buffer,int* buffer_index,int* buffer_stack) {
	//int* new_buffer = new int[64];
	int* new_buffer;
	new_buffer = (int*)malloc(sizeof(int) * 64);
	int k = 0;
	int n = 0;
	int m = 0;
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
				new_buffer[n] = new_buffer[n] | mask;
			}
			else if ((x >= 4 && x < 8) || (x > -8 && x <= -4)) {
				min_val = 4;
				total_length = 6;
				range_size = 2;
				if (x > 0) {
					mask = 0b00001001;
				}
				else {
					mask = 0b00001000;
				}
				range = abs(x) - min_val;
				mask = mask << range_size;
				mask = mask | range;

				m += total_length;
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}

				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
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
				k = total_length;
				if (m > 32) {
					k = m - 32;
					m = k;
					over_mask = mask >> k;
					new_buffer[n] = new_buffer[n] << (total_length - k);
					new_buffer[n] = new_buffer[n] | over_mask;
					n++;
				}
				new_buffer[n] = new_buffer[n] << k;
				if (k != total_length) { mask = mask & ((int)pow(2, k) - 1); }
				new_buffer[n] = new_buffer[n] | mask;
			}
		}
	}
	if (m != 0) {
		k = 32 - m;
		new_buffer[n] = new_buffer[n] << k;
	}
	
	n++;
	realloc(new_buffer, sizeof(int) * n);
	buffer_index[0] = n;
	buffer_stack[0] += n;
	/*printf("\n");
	printf("%d ", buffer_index[0]);
	printf("\n");
	printf("%d ", buffer_stack[0]);
	printf("\n");*/
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

	unsigned char* Reconstructed_buf = new unsigned char[YSize];
	int* encode_buffer2 = new int[YSize];
	for (int f = 0; f < Frame; f++)
	{
		unsigned char* buffer = new unsigned char[Size];
		unsigned char* buffer2 = new unsigned char[YSize];
		unsigned char* buffer3 = new unsigned char[YSize];
		unsigned char* buffer4 = new unsigned char[YSize];
		int* buffer5 = new int[YSize];
		int* encode_buffer1 = new int[YSize];
		
		unsigned char* buffer6 = new unsigned char[YSize];
		unsigned char* buffer7 = new unsigned char[YSize];
		unsigned char* buffer8 = new unsigned char[YSize];
		int* buffer9 = new int[YSize];
		unsigned char* buffer10 = new unsigned char[YSize];
		unsigned char* buffer11 = new unsigned char[YSize];
		//unsigned char* Reconstructed_buf = new unsigned char[YSize];
		unsigned char* MPM_buffer = new unsigned char[36 * 44];
		unsigned char* X_buf = new unsigned char[36 * 44];
		unsigned char* Y_buf = new unsigned char[36 * 44];
		//unsigned char* IncludeMPM_buffer = new unsigned char[YSize + MPM_BUF_Size];
		int b = 0;
		int* buffer_stack;
		buffer_stack = &b;
		err = fopen_s(&picture, "football_cif(352X288)_90f.yuv", "rb");
		if (err == 0) {

			fseek(picture, Size * f, SEEK_SET); //2번째는 이동할 거리, 3번째는 이동방식 -> 파일의 처음 위치를 기준으로 이동한다.
			fread(buffer, 1, Size, picture); // (*ptr, size ,count ,FILE* stream) 2번째 인자의 단위는 바이트 이다.
			for (int i = 0; i < YSize; i++) {
				buffer2[i] = buffer[i];

			}
			if (f % intra_period == 0) { //인트라 인코딩
				buffer3 = Intra_Prediction(buffer2, f, MPM_buffer);
				buffer4 = pixel_DPCM(buffer3, f);
				buffer5 = DCT_QUANTI(buffer4); // dct 된 
				encode_buffer1 = DC_DPCM(buffer5);
				encode_buffer2 = Reordering_entorpy(encode_buffer1,buffer_stack);
				realloc(encode_buffer2, sizeof(int) * (*buffer_stack));

				buffer6 = IDCT_DEQUANTI(buffer5); //idct 된
				buffer7 = Reverse_pixel_DPCM(buffer4, f);
				Reconstructed_buf = Reverse_Intra_Prediction(buffer7, f, MPM_buffer);
			}
			else { //인터 인코딩
				buffer8 = Motion_Estimation(buffer2, Reconstructed_buf, X_buf, Y_buf);
				buffer9 = DCT_QUANTI(buffer8);
				encode_buffer1 = DC_DPCM(buffer9);
				encode_buffer2 = Reordering_entorpy(encode_buffer1,buffer_stack);
				realloc(encode_buffer2, sizeof(int) * (*buffer_stack));

				buffer10 = IDCT_DEQUANTI(buffer9);
				Reconstructed_buf = Reverse_Motion_Estimation(buffer8, Reconstructed_buf, X_buf, Y_buf);
			}

			////check MPM_buffer
			//for (int k = 0; k < 1584; k++){
			//	
			//	if (k % 44 == 0) {
			//		printf("\n");
			//	}
			//	printf("%d ", MPM_buffer[k]);
			//}
			////printf("\n");
			////printf("\n");
			fclose(picture);
			delete[] buffer;
			buffer = NULL;
		}

		//err2 = fopen_s(&picture2, "intra_prediction_Y_football_cif(352X288)_90f.yuv", "a+b"); // wb대신에 a+b를 사용하는 이유는 파일을 이어쓰기 때문이다.
		//if (err2 == 0) {
		//	fseek(picture2, (YSize) * f, SEEK_SET);

		//	fwrite(buffer3, 1, YSize, picture2);

		//	fclose(picture2);
		//}
		//

		err3 = fopen_s(&picture3, "encode_Y_football_cif(352X288)_90f.yuv", "a+b"); // wb대신에 a+b를 사용하는 이유는 파일을 이어쓰기 때문이다.
		if (err3 == 0) {
			fseek(picture3, *buffer_stack * f, SEEK_SET);

			fwrite(encode_buffer2, 4, *buffer_stack, picture3);

			fclose(picture3);
		}


		//err5 = fopen_s(&picture5, "reconstruct_Y_football_cif(352X288)_90f.yuv", "a+b"); // wb대신에 a+b를 사용하는 이유는 파일을 이어쓰기 때문이다.
		//if (err5 == 0) {
		//	fseek(picture5, YSize * f, SEEK_SET);

		//	fwrite(Reconstructed_buf, 1, YSize, picture5);

		//	fclose(picture5);
		//}


		delete[] buffer2;
		buffer2 = NULL;
		delete[] buffer3;
		buffer3 = NULL;
		delete[] buffer4;
		buffer4 = NULL;
		delete[] buffer5;
		buffer5 = NULL;
		delete[] buffer6;
		buffer6 = NULL;
		delete[] buffer7;
		buffer7 = NULL;

		delete[] buffer8;
		buffer8 = NULL;
		delete[] buffer9;
		buffer9 = NULL;
		delete[] buffer10;
		buffer10 = NULL;
		delete[] buffer11;
		buffer11 = NULL;
		/*delete[] encode_buffer1;
		encode_buffer1 = NULL;*/
		/*delete[] IncludeMPM_buffer;
		IncludeMPM_buffer = NULL;*/
	}
	delete[] Reconstructed_buf;
	Reconstructed_buf = NULL;
	
	/*delete[] encode_buffer2;
	encode_buffer2 = NULL;*/
	return 0;
}