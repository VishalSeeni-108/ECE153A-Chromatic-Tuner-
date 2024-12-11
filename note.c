#include "note.h"

#include "lcd.h"
#include "font_TimesNewRoman.h"

//array to store note names for findNote
static char notes[12][3]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

//finds and prints note of frequency and deviation from note
extern inline void findNote(float f, int a, int* results) {
	//a is user inputed value for A4 - shift middle C to match accordingly
	float c=261.63;
	c += a-420;

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
	}

}
