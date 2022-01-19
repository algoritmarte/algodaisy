/*
Daisy Patch skeleton program
https://github.com/algoritmarte/algodaisy
*/
#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

namespace patchutil {
#include "printf.h"
#include "printf.c"
}

#define OLEDCOLS 18

DaisyPatch hw;
bool gateOut1 = false;
float cv1_volt = 0;
float cv2_volt = 0;
bool encoder_held;
bool encoder_press;    
int encoder_inc;
float last_cv1_volt = -1;
float last_cv2_volt = -1;

float knobValues[4] = {};

float audioSampleRate;
float cvSampleRate;

const int notes[8] = { 0,2,4,5,7,9,11,12};
int inote = 0;
float freq = 440.0;
int notetick = 0;
int encoder_track = 0;

WhiteNoise noise = {};
Oscillator osc = {};

void UpdateControls();
void UpdateScreen();

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	UpdateControls();

	// prepare freq for AUDIO1
	float midinote = 60 + notes[ inote ];
	float freq = 440.0f * pow( 2, (midinote - 69) / 12.0f );
	osc.SetFreq( freq );
	osc.SetAmp( 0.2 ); // keep it on LINE level

	for (size_t i = 0; i < size; i++)
	{
		// sine wave on AUDIO1
		out[0][i] = osc.Process();

		// white noise on AUDIO2
		out[1][i] = noise.Process() * 0.2f; // keep it on LINE levels

		out[2][i] = (in[0][i] + in[1][i]) * 0.5f; // mix IN1 and IN2
		out[3][i] = (in[2][i] + in[3][i]) * 0.5f; // mix IN3 and IN4
	}
}

int main(void)
{
	hw.Init();
	//hw.SetAudioBlockSize(32); // if needed (default 48 )

	audioSampleRate = hw.AudioSampleRate();
	cvSampleRate = audioSampleRate / hw.AudioBlockSize();

	noise.Init();
	osc.Init(audioSampleRate);

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	while(1) {
		UpdateScreen();
		hw.DelayMs(50); // 20 Hz refresh
	}
}

void UpdateControls() {
	// needed for properly reading hardware
	hw.ProcessAllControls(); // shortcut for hw.ProcessAnalogControls();  hw.ProcessDigitalControls();

	encoder_held = hw.encoder.Pressed();
 	encoder_press = hw.encoder.RisingEdge();    
    encoder_inc = hw.encoder.Increment();
	encoder_track += encoder_inc;

    // on encoder button press or GATE_IN_1 trig change the state of the GATE_OUTPUT and LED 
	if( encoder_press || hw.gate_input[DaisyPatch::GATE_IN_1].Trig()) {
		gateOut1 = !gateOut1;
		dsy_gpio_write(&hw.gate_output, gateOut1 );
		hw.seed.SetLed( gateOut1 );
	}

	knobValues[0] = hw.GetKnobValue( DaisyPatch::CTRL_1 );
	knobValues[1] = hw.GetKnobValue( DaisyPatch::CTRL_2 );
	knobValues[2] = hw.GetKnobValue( DaisyPatch::CTRL_3 );
	knobValues[3] = hw.GetKnobValue( DaisyPatch::CTRL_4 );

	// every sec change the note on OUT1
	float elapsed = notetick++ / cvSampleRate; // note that UpdateControls is called at cvSampleRate
	if (elapsed > 1)
	{	
		notetick = 0;
		inote = (inote + 1 ) % 8;
	}

	float note = notes[ inote ];
	cv1_volt = note / 12.0f; // standard 1 Volt / Octave pitch 

	if ( last_cv1_volt != cv1_volt ) {
		last_cv1_volt = cv1_volt;
		hw.seed.dac.WriteValue(DacHandle::Channel::ONE,
                              (uint16_t)round(cv1_volt * 819.2f));
	}
	if ( last_cv2_volt != cv2_volt ) {
		last_cv2_volt = cv2_volt;
		hw.seed.dac.WriteValue(DacHandle::Channel::TWO,
                              (uint16_t)round(cv1_volt * 819.2f));
	}						  
}	

void UpdateScreen() {
		char s[OLEDCOLS] = {};
        hw.display.Fill(false);
        hw.display.DrawLine(0, 10, 128, 10, true);

		int cursorpos = 0;
        hw.display.SetCursor(0, cursorpos++ * 13 );
		patchutil::snprintf( s, OLEDCOLS, "K1 %.3f K2 %.3f", knobValues[0], knobValues[1] );
        hw.display.WriteString( s, Font_7x10, true);
        
        hw.display.SetCursor(0, cursorpos++ * 13 );
		patchutil::snprintf( s, OLEDCOLS, "K3 %.3f K4 %.3f", knobValues[2], knobValues[3] );
        hw.display.WriteString( s, Font_7x10, true);

		hw.display.SetCursor(0, cursorpos++ * 13);
		patchutil::snprintf( s, OLEDCOLS, "Enc.track: %d", encoder_track );
        hw.display.WriteString(s, Font_7x10, true);

		hw.display.SetCursor(0, cursorpos++ * 13);
        hw.display.WriteString("0123456789ABCDEFGH", Font_7x10, true);
		hw.display.SetCursor(0, cursorpos++ * 13);
        hw.display.WriteString(" AlgoritmArte.com ", Font_7x10, true);

        hw.display.Update();
}
