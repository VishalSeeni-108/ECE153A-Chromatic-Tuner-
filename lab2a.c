/*****************************************************************************
* lab2a.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#define AO_LAB2A

#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "xspi.h"
#include "xspi_l.h"
#include "lcd.h"
#include <stdbool.h>
#include "stream_grabber.h"
#include "font_TimesNewRoman.h"
#include <stdio.h>
#include "fft.h"
#include "note.h"
#include "math.h"
#include "stdlib.h"

int volume = 63;
int mode = 1;
bool mute = false;

//FFT Stuff
#define SAMPLES 2048
#define M 9 //2^m=samples
#define CLOCK 100000000.0 //clock speed

//Display variables
const int noteX = 75;
const int noteY = 50;
const int octX = 15;
const int octY = 200;
const int freqX = octX;
const int freqY = octY + 30;
const int centX = freqX;
const int centY = freqY + 30;
const int baseX = centX;
const int baseY = centY + 30;
static int baseNote = 440;
char baseDisplay[100];
static int currNote = 0;
static float frequency;
char frequencyDisplay[100];
static int currOct = 4;
char octDisplay[100];
static int spectX = 0;
static float histData[512];
static int results[2];
static float centError = 0.0;
char centDisplay[100];


static char notes[12][3]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};


int int_buffer[SAMPLES];
static float q[SAMPLES];
static float w[SAMPLES];

static float dec_q[SAMPLES/4];
static float dec_w[SAMPLES/4];
static float sec_q[SAMPLES*2];
static float sec_w[SAMPLES*2];
static float sec_dec_q[SAMPLES/8];
static float sec_dec_w[SAMPLES/8];

static inline void read_fsl_values(float* q, int n) {
   int i;
   unsigned int x;
   stream_grabber_start();
   stream_grabber_wait_enough_samples(1);

   for(i = 0; i < n; i++) {
      int_buffer[i] = stream_grabber_read_sample(i);
      // xil_printf("%d\n",int_buffer[i]);
      x = int_buffer[i];
      q[i] = 3.3*x/67108864.0; // 3.3V and 2^26 bit precision.

   }
}

typedef struct Lab2ATag  {               //Lab2A State machine
	QActive super;
}  Lab2A;

/* Setup state machines */
/**********************************************************************/
static QState Lab2A_initial (Lab2A *me);
static QState Lab2A_on      (Lab2A *me);
static QState Lab2A_stateA  (Lab2A *me);
static QState Lab2A_stateB  (Lab2A *me);


/**********************************************************************/


Lab2A AO_Lab2A;


void Lab2A_ctor(void)  {
	Lab2A *me = &AO_Lab2A;
	QActive_ctor(&me->super, (QStateHandler)&Lab2A_initial);
}


QState Lab2A_initial(Lab2A *me) {
	xil_printf("\n\rInitialization");
    return Q_TRAN(&Lab2A_on);
}

QState Lab2A_on(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("\n\rOn");
			}
			
		case Q_INIT_SIG: {
			return Q_TRAN(&Lab2A_stateA);
			}
	}

	return Q_SUPER(&QHsm_top);
}


/* Create Lab2A_on state and do any initialization code if needed */
/******************************************************************/

QState Lab2A_stateA(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("Startup State A\r\n");

			drawBackground();
	        setColor(0, 255, 0);
			setFont(&TimesNewRoman_16);
	        lcdPrint("Octave: ", octX, octY);
	        lcdPrint("Frequency: ", freqX, freqY);
	        lcdPrint("Base Note: ", baseX, baseY);
	        sprintf(baseDisplay, "%d Hz", baseNote);
	        lcdPrint(baseDisplay, baseX + 100, baseY);
	        lcdPrint("Cent Error: ", centX, centY);

			return Q_HANDLED();
		}
		
		case ENCODER_UP: {
			xil_printf("Encoder Up from State A\r\n");
			if(baseNote < 460)
			{
				baseNote++;
				setColor(0, 0, 0);
				fillRect(baseX + 100, baseY, baseX + 150, baseY + 20);
				setColor(0, 255, 0);
		        sprintf(baseDisplay, "%d Hz", baseNote);
		        lcdPrint(baseDisplay, baseX + 100, baseY);
			}
			return Q_HANDLED();
		}

		case ENCODER_DOWN: {
			xil_printf("Encoder Down from State A\r\n");
			if(baseNote > 420)
			{
				baseNote--;
				setColor(0, 0, 0);
				fillRect(baseX + 100, baseY, baseX + 150, baseY + 20);
				setColor(0, 255, 0);
				sprintf(baseDisplay, "%d Hz", baseNote);
				lcdPrint(baseDisplay, baseX + 100, baseY);
			}
			return Q_HANDLED();
		}

		case ENCODER_CLICK:  {
			xil_printf("Encoder Click from State A\r\n");
			baseNote = 440;
			setColor(0, 0, 0);
			fillRect(baseX + 100, baseY, baseX + 150, baseY + 20);
			setColor(0, 255, 0);
			sprintf(baseDisplay, "%d Hz", baseNote);
			lcdPrint(baseDisplay, baseX + 100, baseY);

			return Q_HANDLED();
		}
		case BUTTON_1: {
//			lcdPrint("Mode: 1", 65, 130);
//			mode = 1;
			return Q_HANDLED();
		}
		case BUTTON_2:{
//			lcdPrint("Mode: 2", 65, 130);
//			mode = 2;
			return Q_HANDLED();
		}
		case BUTTON_3:{
//			lcdPrint("Mode: 3", 65, 130);
//			mode = 3;
			return Q_TRAN(&Lab2A_stateB);
		}
		case BUTTON_4:{
//			lcdPrint("Mode: 4", 65, 130);
//			mode = 4;
			return Q_HANDLED();
		}
		case BUTTON_5:{
//			lcdPrint("Mode: 5", 65, 130);
//			mode = 5;
			return Q_HANDLED();
		}
		case FFT_UPDATE:{
	    	//Call FFT
	    	//FFT Stuff
	       float sample_f;
	       int l;
//	       int ticks; //used for timer
//	       uint32_t Control;
//	       float tot_time; //time to run program

	       //Get new values
	       read_fsl_values(q, SAMPLES);

	       for(int i = 0; i < SAMPLES; i+=4)
	       {
	    	   float sum = q[i] + q[i+1] + q[i+2] + q[i+3];
	    	   dec_q[i/4] = sum/4;
	       }

//	       xil_printf("q_dec START");
//	       for(int i = 0; i < 512; i++)
//	       {
//			   int whole = dec_q[i];
//			   int thousandths = (dec_q[i]-whole) * 1000000;
//	    	   xil_printf("q_dec[%d]: %d.%d\r\n", i, whole, thousandths);
//	       }
//	       xil_printf("q_dec END");

	       sample_f = 100*1000*1000/2048.0;

	       for(l=0;l<SAMPLES;l++)
	    	   w[l]=0;

	       for(l=0;l<(SAMPLES/4);l++)
	    	   dec_w[l]=0;
	       //xil_printf("Before first FFT \r\n");
	       frequency=fft(dec_q,dec_w,(SAMPLES/4),M,sample_f/4);
	       //xil_printf("After first FFT \r\n");

//	       if(frequency < 440)
//	       {
//	    	   read_fsl_values(sec_q, SAMPLES*2);
//		       for(int i = 0; i < SAMPLES*2; i+=8)
//		       {
//		    	   float sum = sec_q[i] + sec_q[i+1] + sec_q[i+2] + sec_q[i+3] + sec_q[i+4] + sec_q[i+5] + sec_q[i+6] + sec_q[i+7];
//		    	   sec_dec_q[i/8] = sum/8;
//		       }
//		       for(l=0;l<(SAMPLES/4);l++)
//		    	   sec_dec_w[l]=0;
//
//		       frequency=fft(sec_dec_q,sec_dec_w,(SAMPLES/4),M,sample_f/8);
//
//	       }

//		   if(frequency < 440)
//		   {
//			   for(int i = 0; i < SAMPLES/4; i+=2)
//			   {
//				   float sum = dec_q[i] + dec_q[i+1];
//				   sec_dec_q[i/2] = sum/2;
//			   }
//
//			   for(l=0;l<(SAMPLES/8);l++)
//				   sec_dec_w[l]=0;
//
//			   xil_printf("Before second FFT \r\n");
//			   frequency=fft(sec_dec_q,sec_dec_w,(SAMPLES/8),8,sample_f/8);
//			   xil_printf("After second FFT \r\n");
//		   }

		   findNote(frequency, baseNote, results, &centError);
		   sprintf(frequencyDisplay, "%.2f Hz", frequency);
//		   int centsWhole = centError;
//		   int centsThousandths = (centError-centsWhole) * 1000000;
//		   xil_printf("Cent error: %d.%d \r\n", centsWhole, centsThousandths);


		   //Rewrite values
		   if(results[0] != currNote)
		   {
		       setFont(&TimesNewRoman_96);
		       setColor(0, 0, 0);
		       lcdPrint(notes[currNote], noteX, noteY);
		       setColor(0, 255, 0);
	           lcdPrint(notes[results[0]], noteX, noteY);
			   currNote = results[0];
		   }

		   if(results[1] != currOct && (results[1] >= 0 && results[1] <= 8))
		   {
			   setFont(&TimesNewRoman_16);
			   setColor(0, 0, 0);
			   lcdPrint(octDisplay, octX+100, octY);
		       sprintf(octDisplay, "%d", results[1]);
			   setColor(0, 255, 0);
			   lcdPrint(octDisplay, octX+100, octY);
			   currOct = results[1];
		   }

	       setFont(&TimesNewRoman_16);
	       setColor(0, 0, 0);
	       fillRect(freqX+100, freqY, 255, freqY + 20);
	       setColor(0, 255, 0);
           lcdPrint(frequencyDisplay, freqX+100, freqY);

	       setFont(&TimesNewRoman_16);
	       setColor(0, 0, 0);
	       fillRect(centX+100, centY, 255, centY + 20);
	       setColor(0, 255, 0);
	       sprintf(centDisplay, "%f", centError);
           lcdPrint(centDisplay, centX+100, centY);

           //Error rectangle
           setColor(0, 0, 0);
           fillRect(0, freqY - 75, 240, freqY - 50);
	       if(fabs(centError) < 25)
	       {
	           setColor(0, 255, 0);
	       }
	       else if(fabs(centError) < 75)
	       {
	    	   setColor(255, 175, 0);
	       }
	       else
	       {
	    	   setColor(255, 0, 0);
	       }
	       fillRect(120, freqY - 75, 120+centError, freqY - 50);


	           //get time to run program
	//           ticks=XTmrCtr_GetValue(&sys_timer, 0);
	//           XTmrCtr_Stop(&timer, 0);
	//           tot_time=ticks/CLOCK;

	           //xil_printf("frequency: %d Hz\r\n program time: %dms \r\n", (int)(frequency+.5), (int)(1000*tot_time));
	//           xil_printf("Idling\r\n");
	//           xil_printf("frequency: %d Hz\r\n", (int)(frequency+.5));
	//           xil_printf("Note: %d \r\n", currNote);
	           //lcdPrint("C", 50, 50);
		}
//		case TIMER_RESET:{
//			return Q_TRAN(&Lab2A_stateB);
//		}

	}
	return Q_SUPER(&Lab2A_on);

}

QState Lab2A_stateB(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("Startup State B\r\n");

			drawBackground();



			return Q_HANDLED();
		}
		
		case ENCODER_UP:
		case ENCODER_DOWN:
		case ENCODER_CLICK:
		case BUTTON_1:
		case BUTTON_2:
		case BUTTON_4:
		case BUTTON_5:
		{
			return Q_HANDLED();
		}
		case BUTTON_3:
		{
			return Q_TRAN(&Lab2A_stateA);
		}

		case FFT_UPDATE:{
			    	//Call FFT
			    	//FFT Stuff
			       float sample_f;
			       int l;

			       //Get new values
			       read_fsl_values(q, SAMPLES);

			       for(int i = 0; i < SAMPLES; i+=4)
			       {
			    	   float sum = q[i] + q[i+1] + q[i+2] + q[i+3];
			    	   dec_q[i/4] = sum/4;
			       }

			       sample_f = 100*1000*1000/2048.0;

			       	       for(l=0;l<SAMPLES;l++)
			       	    	   w[l]=0;

			       	       for(l=0;l<(SAMPLES/4);l++)
			       	    	   dec_w[l]=0;


			       hist_fft(dec_q,dec_w,(SAMPLES/4),M,sample_f/4, histData);
//			       xil_printf("Histogram Data START \r\n");
//			       for(int i = 0; i < 512; i++)
//			       {
//			    	   	   int whole, thousandths;
//			    	   	   whole = logf(histData[i]);
//			    	   	   thousandths = (logf(histData[i])-whole) * 1000;
//			    	   	   whole = histData[i];
//			    	   	   thousandths = (histData[i] - whole) * 1000;
//			    	   	   xil_printf("Bin %d: %d.%d \r\n", i, whole, thousandths);
//			       }
//			       xil_printf("Histogram Data END \r\n");

			       for(int i = 0; i < 512; i++) //TODO - need to figure out a way to reduce this to 240 bins
			       {
			    	   if(logf(histData[i]) < 0.25)
			    	   {
			    		   setColor(0,0,255);
			    	   }
			    	   else if(logf(histData[i]) < 0.75)
			    	   {
			    		   setColor(0,255,0);
			    	   }
			    	   else
			    	   {
			    		   setColor(255,0,0);
			    	   }

				       fillRect(i, spectX, i + 1, spectX+1);
			       }
			       if(spectX > 320)
			       {
			    	   spectX = 0;
			       }
			       else
			       {
				       spectX += 1;
			       }
				}

	}

	return Q_SUPER(&Lab2A_on);

}

