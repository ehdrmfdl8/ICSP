#include <stdio.h>
#include <cstdio>
#include <stdlib.h>
#include <algorithm>

#define Width 352
#define Height 288
#define PI 3.142857

using namespace std;
const long Size = Width * Height * 3 / 2;
const long YSize = Width * Height;
const long MPM_BUF_Size = 1584;
const long USize = Width * Height * 1 / 4;
const long VSize = Width * Height * 1 / 4;
unsigned char** Reverse_DPCM_Mode0(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
unsigned char** Reverse_DPCM_Mode1(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
unsigned char** Reverse_DPCM_Mode2(unsigned char matrix[8][8], int, unsigned char*, unsigned char*);
unsigned char* Reverse_pixel_DPCM(unsigned char*,int);
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
				MODE1_matrix[i][j] = matrix[i][j] + H_ref[8 * ((k % 44) - 1) + j];
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


int main() {
	errno_t err, err2, err3, err4;
	FILE* picture;
	FILE* picture2;
	FILE* picture3;
	FILE* picture4;

	int Frame = 0;
	err = fopen_s(&picture, "football_cif(352X288)_90f.yuv", "rb");
	if (err == 0) {
		fseek(picture, 0, SEEK_END);
		int fileLength = ftell(picture);
		Frame = fileLength / Size;
	}

	for (int f = 0; f < Frame; f++)
	{
		unsigned char* buffer = new unsigned char[Size];
		unsigned char* buffer2 = new unsigned char[YSize];
		unsigned char* buffer3 = new unsigned char[YSize];
		unsigned char* buffer4 = new unsigned char[YSize];
		unsigned char* MPM_buffer = new unsigned char[MPM_BUF_Size];

		err = fopen_s(&picture, "IncludeMPM_buffer_DPCM1_Y_football_cif(352X288)_90f.yuv", "rb");
		if (err == 0) {

			fseek(picture, (YSize + MPM_BUF_Size) * f, SEEK_SET); //2번째는 이동할 거리, 3번째는 이동방식 -> 파일의 처음 위치를 기준으로 이동한다.
			fread(buffer, 1, YSize + MPM_BUF_Size, picture); // (*ptr, size ,count ,FILE* stream) 2번째 인자의 단위는 바이트 이다.
			//for (int i = 0; i < YSize + MPM_BUF_Size; i++)
			//{
			//	if (i % 44 == 0) {
			//		printf("\n");
			//	}
			//	printf("%d ", buffer[i]);
			//}
			for (int i = 0; i < MPM_BUF_Size; i++)
			{
				MPM_buffer[i] = buffer[i];
			}
			for (int i = 0; i < YSize; i++)
			{
				buffer2[i] = buffer[i + MPM_BUF_Size];
			}
			
			buffer3 = Reverse_pixel_DPCM(buffer2, f);
			buffer4 = Reverse_Intra_Prediction(buffer3, f, MPM_buffer);
			//buffer4 = dctTransform(buffer3);
			fclose(picture);
			delete[] buffer;
			buffer = NULL;
		}

		err2 = fopen_s(&picture2, "decode_pixel_DPCM_Mode_Y_football_cif(352X288)_90f.yuv", "a+b"); // wb대신에 a+b를 사용하는 이유는 파일을 이어쓰기 때문이다.
		if (err2 == 0) {
			fseek(picture2, YSize * f, SEEK_SET);

			fwrite(buffer3, 1, YSize, picture2);

			fclose(picture2);
			delete[] buffer3;
			buffer3 = NULL;
		}
		
		err3 = fopen_s(&picture3, "decode_intra_Y_football_cif(352X288)_90f.yuv", "a+b"); // wb대신에 a+b를 사용하는 이유는 파일을 이어쓰기 때문이다.
		if (err3 == 0) {
			fseek(picture3, YSize * f, SEEK_SET);

			fwrite(buffer4, 1, YSize, picture3);

			fclose(picture3);
			delete[] buffer4;
			buffer4 = NULL;
		}

		delete[] buffer2;
		buffer2 = NULL;

	}

	return 0;
}