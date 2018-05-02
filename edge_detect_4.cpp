#include <ap_fixed.h>
#include <ap_int.h>
#include <stdint.h>
#include <assert.h>

typedef ap_uint<8> pixel_type;
typedef ap_int<8> pixel_type_s;
typedef ap_uint<96> u96b;
typedef ap_uint<32> word_32;
typedef ap_ufixed<8,0, AP_RND, AP_SAT> comp_type;
typedef ap_fixed<10,2, AP_RND, AP_SAT> coeff_type;


typedef unsigned int uint;
typedef unsigned char uchar;

struct pixel_data {
	pixel_type blue;
	pixel_type green;
	pixel_type red;
};

static uchar row[1920][4];

unsigned char gray (uint current);

void gray_filter(volatile uint32_t* in_data, volatile uint32_t* out_data, int w, int h, int parameter_1){
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE s_axilite port=parameter_1
#pragma HLS INTERFACE s_axilite port=w
#pragma HLS INTERFACE s_axilite port=h

#pragma HLS INTERFACE m_axi depth=2073600 port=in_data offset=slave // This will NOT work for resolutions higher than 1080p
#pragma HLS INTERFACE m_axi depth=2073600 port=out_data offset=slave

	unsigned char out;
	for(int j = 0; j < 3; j++){
		for(int i = 0; i < w; i++){
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_FLATTEN off
			row[i][j] = gray(*in_data++);
		}
	}

	uchar LU;
	uchar U;
	uchar RU;
	uchar L;
	uchar M;
	uchar R;
	uchar LD;
	uchar D;
	uchar RD;

	uchar cur_row_in = 3;   // points to buffer to write
	uchar cur_row_out_0 = 0;	//points to buffers to read
	uchar cur_row_out_1 = 1;
	uchar cur_row_out_2 = 2;

	for (int j = 0; j < h-3; j++) {
		LU = row[0][cur_row_out_0];
		U  = row[1][cur_row_out_0];
		RU = row[2][cur_row_out_0];
		L  = row[0][cur_row_out_1];
		M  = row[1][cur_row_out_1];
		R  = row[2][cur_row_out_1];
		LD = row[0][cur_row_out_2];
		D  = row[1][cur_row_out_2];
		RD = row[2][cur_row_out_2];

		for (int i = 0; i < w; i++) {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_FLATTEN off
			//buffer
			row[i][cur_row_in]=*in_data;
			in_data++;
			if((j == 0) || (j == h-1) || (i == 0) || (i == w-1)){ //jak cos to zmienic
				out = 0;
			}
			else{

				LU = U;
				U  = RU;
				RU = row[i+1][cur_row_out_0];
				L  = M;
				M  = R;
				R  = row[i+1][cur_row_out_1];
				LD = D;
				D  = RD;
				RD = row[i+1][cur_row_out_2];

				int X = (RU)+(R)*2+(RD)+(LU)*(-1)+(L)*(-2)+(LD)*(-1);
				int Y = (LD)+(D)*2+(RD)+(LU)*(-1)+(U)*(-2)+(RU)*(-1);

				float tmp = sqrtf((X*X)+(Y*Y));
				out = int(tmp);
			}
			unsigned int output = out | (out << 8) | (out << 16);
			*out_data++ = output;
			//in_data++;
		}
		cur_row_in = (cur_row_in + 1) % 4;   // points to buffer to write
		cur_row_out_0 = (cur_row_out_0 + 1) % 4;	//points to buffers to read
		cur_row_out_1 = (cur_row_out_1 + 1) % 4;
		cur_row_out_2 = (cur_row_out_2 + 1) % 4;
	}
}


unsigned char gray (uint current){

	unsigned char in_r = current & 0xFF;
	unsigned char in_b = (current >> 8) & 0xFF;
	unsigned char in_g = (current >> 16) & 0xFF;

	float Y = 0.2126f*in_r  + 0.7152f*in_g  + 0.0722f*in_b ;

	unsigned char output = int(Y);

	return output;
}
