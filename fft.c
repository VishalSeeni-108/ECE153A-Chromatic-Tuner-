#include "fft.h"
#include "complex.h"
#include "trig.h"
#include <math.h>
#include <string.h>
#include "xil_printf.h"
#include <stdio.h>


static float new_[512];
static float new_im[512];
static float sinLUT[512][9];
static float cosLUT[512][9];


extern inline initializeLUT()
{
	   for(int i = 0; i < 512; i++)
	   {
		   for(int j = 0; j < 9; j++)
		   {
			   sinLUT[i][j] = sin(-PI*i/(1<<j));
			   cosLUT[i][j] = cos(-PI*i/(1<<j));
		   }
	   }
}

#include <stdint.h>

uint16_t get_mirror(uint16_t orig, int numBits) {
    // Reverse the entire 16 bits first
    orig = ((orig & 0xAAAA) >> 1) | ((orig & 0x5555) << 1);
    orig = ((orig & 0xCCCC) >> 2) | ((orig & 0x3333) << 2);
    orig = ((orig & 0xF0F0) >> 4) | ((orig & 0x0F0F) << 4);
    orig = (orig >> 8) | (orig << 8);

    // Mask to retain only numBits
    return orig >> (16 - numBits);
}


float fft(float* q, float* w, int n, int m, float sample_f) {
	int a,b,r,d,e,c;
	int k,place;
	a=n>>1;
	b=1;
	int i,j;
	float real=0,imagine=0;
	float max,frequency;

	// ORdering algorithm
//	for(i=0; i<(m-1); i++){
//		d=0;
//		for (j=0; j<b; j++){
//			for (c=0; c<a; c++){
//				e=c+d;
//				new_[e]=q[(c<<1)+d];
//				new_im[e]=w[(c<<1)+d];
//				new_[e+a]=q[(c<<1)+1+d];
//				new_im[e+a]=w[(c<<1)+1+d];
//			}
//			d+=(n/b);
//		}
//		for (r=0; r<n;r++){
//			q[r]=new_[r];
//			w[r]=new_im[r];
//		}
//		b = b<<1;
//		a=n/(b<<1);
//	}

	uint16_t mirror = 0;
	float temp;

	for(uint16_t r = 0; r < n; r++)
	{
		if(mirror > r)
		{
			temp = q[mirror];
			q[mirror] = q[r];
			q[r] = temp;

			temp = w[mirror];
			w[mirror] = w[r];
			w[r] = temp;
		}

		mirror = get_mirror(r+1, m);
	}
	//end ordering algorithm

	b=1;
	k=0;
	for (j=0; j<m; j++){
	//MATH
//		for(i=0; i<n; i+=2){
//			if (i%(n/b)==0 && i!=0)
//				k++;
//			real=mult_real(q[i+1], w[i+1], cos(-PI*k/b), sin(-PI*k/b));
//			imagine=mult_im(q[i+1], w[i+1], cos(-PI*k/b), sin(-PI*k/b));
//			new_[i]=q[i]+real;
//			new_im[i]=w[i]+imagine;
//			new_[i+1]=q[i]-real;
//			new_im[i+1]=w[i]-imagine;
//
//		}
//		for (i=0; i<n; i++){
//			q[i]=new_[i];
//			w[i]=new_im[i];
//		}

		for(i=0; i<n; i+=2){
			if (i%(n/b)==0 && i!=0)
				k++;
			//xil_printf("sinValue = %f, cosValue = %f\n", sinValue, cosValue);
			real=mult_real(q[i+1], w[i+1], cosLUT[k][j], sinLUT[k][j]);
			imagine=mult_im(q[i+1], w[i+1], cosLUT[k][j], sinLUT[k][j]);
			new_[i]=q[i]+real;
			new_im[i]=w[i]+imagine;
			new_[i+1]=q[i]-real;
			new_im[i+1]=w[i]-imagine;

		}
//		for (i=0; i<n; i++){
//			q[i]=new_[i];
//			w[i]=new_im[i];
//		}
		memcpy(q, new_, sizeof(float)*512);
		memcpy(w, new_im, sizeof(float)*512);

	//END MATH

	//REORDER
		for (i=0; i<(n>>1); i++){
			new_[i]=q[(i<<1)];
			new_[i+(n>>1)]=q[(i<<1)+1];
			new_im[i]=w[(i<<1)];
			new_im[i+(n>>1)]=w[(i<<1)+1];
		}
		for (i=0; i<n; i++){
			q[i]=new_[i];
			w[i]=new_im[i];
		}
	//END REORDER
		b = b << 1;
		k=0;

	}

	//find magnitudes
	max=0;
	place=1;
	for(i=1;i<(n>>1);i++) {
		new_[i]=q[i]*q[i]+w[i]*w[i];
		if(max < new_[i]) {
			max=new_[i];
			place=i;
		}
	}

	float s=sample_f/n; //spacing of bins

	frequency = (sample_f/n)*place;

	//curve fitting for more accuarcy
	//assumes parabolic shape and uses three point to find the shift in the parabola
	//using the equation y=A(x-x0)^2+C
	float y1=new_[place-1],y2=new_[place],y3=new_[place+1];
	float x0=s+(2*s*(y2-y1))/(2*y2-y1-y3);
	x0=x0/s-1;

	if(x0 <0 || x0 > 2) { //error
		return 0;
	}
	if(x0 <= 1)  {
		frequency=frequency-(1-x0)*s;
	}
	else {
		frequency=frequency+(x0-1)*s;
	}

	return frequency;
}

void hist_fft(float* q, float* w, int n, int m, float sample_f, float* histData) {
	int a,b,r,d,e,c;
	int k,place;
	a=n>>1;
	b=1;
	int i,j;
	float real=0,imagine=0;
	float max,frequency;

	// ORdering algorithm
//	for(i=0; i<(m-1); i++){
//		d=0;
//		for (j=0; j<b; j++){
//			for (c=0; c<a; c++){
//				e=c+d;
//				new_[e]=q[(c<<1)+d];
//				new_im[e]=w[(c<<1)+d];
//				new_[e+a]=q[(c<<1)+1+d];
//				new_im[e+a]=w[(c<<1)+1+d];
//			}
//			d+=(n/b);
//		}
//		for (r=0; r<n;r++){
//			q[r]=new_[r];
//			w[r]=new_im[r];
//		}
//		b = b<<1;
//		a=n/(b<<1);
//	}

	uint16_t mirror = 0;
	float temp;

	for(uint16_t r = 0; r < n; r++)
	{
		if(mirror > r)
		{
			temp = q[mirror];
			q[mirror] = q[r];
			q[r] = temp;

			temp = w[mirror];
			w[mirror] = w[r];
			w[r] = temp;
		}

		mirror = get_mirror(r+1, m);
	}
	//end ordering algorithm

	b=1;
	k=0;
	for (j=0; j<m; j++){
	//MATH
//		for(i=0; i<n; i+=2){
//			if (i%(n/b)==0 && i!=0)
//				k++;
//			real=mult_real(q[i+1], w[i+1], cos(-PI*k/b), sin(-PI*k/b));
//			imagine=mult_im(q[i+1], w[i+1], cos(-PI*k/b), sin(-PI*k/b));
//			new_[i]=q[i]+real;
//			new_im[i]=w[i]+imagine;
//			new_[i+1]=q[i]-real;
//			new_im[i+1]=w[i]-imagine;
//
//		}
//		for (i=0; i<n; i++){
//			q[i]=new_[i];
//			w[i]=new_im[i];
//		}

		for(i=0; i<n; i+=2){
			if (i%(n/b)==0 && i!=0)
				k++;
			//xil_printf("sinValue = %f, cosValue = %f\n", sinValue, cosValue);
			real=mult_real(q[i+1], w[i+1], cosLUT[k][j], sinLUT[k][j]);
			imagine=mult_im(q[i+1], w[i+1], cosLUT[k][j], sinLUT[k][j]);
			new_[i]=q[i]+real;
			new_im[i]=w[i]+imagine;
			new_[i+1]=q[i]-real;
			new_im[i+1]=w[i]-imagine;

		}
//		for (i=0; i<n; i++){
//			q[i]=new_[i];
//			w[i]=new_im[i];
//		}
		memcpy(q, new_, sizeof(float)*512);
		memcpy(w, new_im, sizeof(float)*512);

	//END MATH

	//REORDER
		for (i=0; i<(n>>1); i++){
			new_[i]=q[(i<<1)];
			new_[i+(n>>1)]=q[(i<<1)+1];
			new_im[i]=w[(i<<1)];
			new_im[i+(n>>1)]=w[(i<<1)+1];
		}
		for (i=0; i<n; i++){
			q[i]=new_[i];
			w[i]=new_im[i];
		}
	//END REORDER
		b = b << 1;
		k=0;

	}

	//find magnitudes
	max=0;
	place=1;
	for(i=1;i<512;i++) {
		new_[i]=q[i]*q[i]+w[i]*w[i];
		if(max < new_[i]) {
			max=new_[i];
			place=i;
		}
	}

//   xil_printf("FFT Data START \r\n");
//   for(int i = 0; i < 512; i++)
//   {
//	   int whole, thousandths;
//	   whole = new_[i];
//	   thousandths = (new_[i]-whole) * 1000;
//	   xil_printf("Bin %d: %d.%d \r\n", i, whole, thousandths);
//	   //printf("%f\r\n", new_[i]);
//   }
//   xil_printf("FFT Data END \r\n");

	float s=sample_f/n; //spacing of bins

	frequency = (sample_f/n)*place;

	//curve fitting for more accuarcy
	//assumes parabolic shape and uses three point to find the shift in the parabola
	//using the equation y=A(x-x0)^2+C
	float y1=new_[place-1],y2=new_[place],y3=new_[place+1];
	float x0=s+(2*s*(y2-y1))/(2*y2-y1-y3);
	x0=x0/s-1;

	if(x0 <0 || x0 > 2) { //error
		return 0;
	}
	if(x0 <= 1)  {
		frequency=frequency-(1-x0)*s;
	}
	else {
		frequency=frequency+(x0-1)*s;
	}

	for(int i = 0; i < 512; i++)
	{
		histData[i] = new_[i];
	}
}

