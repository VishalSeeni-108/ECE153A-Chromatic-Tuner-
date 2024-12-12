#include "note.h"

#include "lcd.h"
#include "font_TimesNewRoman.h"
#include "stdlib.h"
#include <math.h>

//array to store note names for findNote
static char notes[12][3]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

//finds and prints note of frequency and deviation from note
extern inline void findNote(float f, int a, int* results, float* centErrorOut) {
	//a is user inputed value for A4 - shift middle C to match accordingly
	//find value by which A4 is pitch shifted and shift c accordingly
	float shift = (log((double)a/440)/log(2));
	float c=pow(2, (shift))*261.63;
//	int cWhole = c;
//	int cThousandths = (c-cWhole) * 1000000;
	//xil_printf("Value for c is: %d.%d \r\n", cWhole, cThousandths);

	float r;
	int oct=4;
	int note=0;
	//determine which octave frequency is in
	if(f >= c) {
		while(f > c*2) {
			c=c*2;
			oct++;
		}
	}
	else { //f < C4
		while(f < c) {
			c=c/2;
			oct--;
		}
	}

	results[1] = oct;

	//find note below frequency
	//c=middle C
	r=c*root2;
	while(f > r) {
		c=c*root2;
		r=r*root2;
		note++;
	}

	//determine which note frequency is closest to
	if((f-c) <= (r-f)) { //closer to left note
//		WriteString("N:");
//		WriteString(notes[note]);
//		WriteInt(oct);
//		WriteString(" D:+");
//		WriteInt((int)(f-c+.5));
//		WriteString("Hz");
		results[0] = note;
	   int freqWhole = f;
	   int freqThousandths = (f-freqWhole) * 1000000;
	   int targetWhole = c;
	   int targetThousandths = (c-targetWhole) * 1000000;
		//Calculate cent error
		float cents = 1200 * (logf(c/f)/logf(2));
		   int centsWhole = cents;
		   int centsThousandths = (cents-centsWhole) * 1000000;
	    //xil_printf("Picked left, for a measurement of %d.%d we have a target of %d.%d and cent error of %d.%d\r\n", freqWhole, freqThousandths, targetWhole, targetThousandths, centsWhole, centsThousandths);
		if(cents != cents)
		{
			*centErrorOut = 0.0;
		}
		else
		{
		    *centErrorOut = cents;
		}
	}
	else { //f closer to right note
		note++;
		if(note >=12) note=0;
//		WriteString("N:");
//		WriteString(notes[note]);
//		WriteInt(oct);
//		WriteString(" D:-");
//		WriteInt((int)(r-f+.5));
//		WriteString("Hz");
		results[0] = note;
		   int freqWhole = f;
		   int freqThousandths = (f-freqWhole) * 1000000;
		   int targetWhole = r;
		   int targetThousandths = (r-targetWhole) * 1000000;

			//Calculate cent error
				float cents = 1200 * (logf(r/f)/logf(2));
				   int centsWhole = cents;
				   int centsThousandths = (cents-centsWhole) * 1000000;
			    //xil_printf("Picked right, for a measurement of %d.%d we have a target of %d.%d and cent error of %d.%d\r\n", freqWhole, freqThousandths, targetWhole, targetThousandths, centsWhole, centsThousandths);
			if(cents != cents)
			{
				*centErrorOut = 0.0;
			}
			else
			{
			    *centErrorOut = cents;
			}
	}

	//determine error in cents
	//first, find target frequency
//	int numSemiTones = 0;
//	if(oct > 4)
//	{
//		numSemiTones = (note+1) + (((oct-1) - 4)*12) + 2;
//	}
//	else if(oct < 4)
//	{
//		numSemiTones = -(((4 - (oct))*12) + 9 - (note));
//	}
//	else
//	{
//		numSemiTones = note-9;
//	}
//	double tempDiv = (double)numSemiTones/12;
//	double targetFreq = pow(2, tempDiv) * a;
//
//	//next, calculate cent error - return with sign
//	double cents = 1200 * (log(targetFreq/f)/log(2));
//	   int centWhole = cents;
//	   int centThousandths = (cents-centWhole) * 1000;
//	   int freqWhole = f;
//	   int freqThousandths = (f-freqWhole) * 1000000;
//	   int targetWhole = targetFreq;
//	   int targetThousandths = (targetFreq-targetWhole) * 1000000;
//
//	   xil_printf("For a measurement of %d.%d with a target of %d.%d, we are %d.%d cents off\r\n", freqWhole, freqThousandths, targetWhole, targetThousandths, centWhole, centThousandths);


}
