// Copyright 2015 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.

#include "dsp/part.h"
#include "dsp/strummer.h"
#include "dsp/string_synth_part.h"
#include "cv_scaler.h"
#include "daisy_patch.h"
#include "UiHardware.h"
#include "Drone.hpp"

using namespace daisy;
using namespace torus;

daisy::UI ui;

FullScreenItemMenu mainMenu;
FullScreenItemMenu controlEditMenu;
FullScreenItemMenu boolEditMenu;
FullScreenItemMenu normEditMenu;
FullScreenItemMenu plusEditMenu;
FullScreenItemMenu modEditMenu;
UiEventQueue       eventQueue;
DaisyPatch         hw;

float samplerate;

#define MAX_DELAY ((size_t)(10.0f * 48000.0f))

// 10 second delay line on the external SDRAM
daisysp::DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delay;
Drone drone;

uint16_t reverb_buffer[32768];

CvScaler        cv_scaler;
Part            part;
StringSynthPart string_synth;
Strummer        strummer;

const int                kNumMainMenuItems        = 13;
const int                kNumControlEditMenuItems = 5;
const int                kNumNormEditMenuItems    = 5;
const int                kPlusEditMenuItems       = 7;
const int                kModEditMenuItems        = 4;

AbstractMenu::ItemConfig mainMenuItems[kNumMainMenuItems];
AbstractMenu::ItemConfig controlEditMenuItems[kNumControlEditMenuItems];
AbstractMenu::ItemConfig normEditMenuItems[kNumNormEditMenuItems];
AbstractMenu::ItemConfig plusEditMenuItems[kPlusEditMenuItems];
AbstractMenu::ItemConfig modEditMenuItems[kModEditMenuItems];

// control menu items
const char* controlListValues[]
    = {"Frequency", "Structure", "Damping", "Position", "Brightness"};
MappedStringListValue controlListValueOne(controlListValues, 5, 0);
MappedStringListValue controlListValueTwo(controlListValues, 5, 1);
MappedStringListValue controlListValueThree(controlListValues, 5, 2);
MappedStringListValue controlListValueFour(controlListValues, 5, 3);

// poly/model menu items
const char*           polyListValues[] = {"One", "Two", "Four"};
MappedStringListValue polyListValue(polyListValues, 3, 1);
const char*           modelListValues[] = {"Modal",
                                 "Symp Str",
                                 "Inhrm Str",
                                 "Fm Voice",
                                 "Westn Chrd",
                                 "Str & Verb"};
MappedStringListValue modelListValue(modelListValues, 6, 0);
const char*           eggListValues[]
    = {"Formant", "Chorus", "Reverb", "Formant2", "Ensemble", "Reverb2"};
MappedStringListValue eggListValue(eggListValues, 6, 0);

// norm edit menu items
bool exciterIn = false;
bool strumIn = true;
bool noteQuantize = true;
bool testMode = false;
bool monoOutput = true;

//easter egg toggle
bool easterEggOn;

// delay line
float dly_time = 0.03f;
float dly_feedback = 0.01f;
MappedFloatValue delaytimeFloatValue( 0, 5, 0.750, MappedFloatValue::Mapping::lin, "s", 2 );
MappedFloatValue delayfeedbackFloatValue( 0, 1, 0.25f, MappedFloatValue::Mapping::lin, "", 2 );
MappedFloatValue delaymixFloatValue(0,1,0.3f, MappedFloatValue::Mapping::lin, "", 2);

// plus menu
MappedFloatValue brightnessFloatValue( 0.f, 1.f, 0.5f, MappedFloatValue::Mapping::lin, "", 2  );
MappedFloatValue bmpFloatValue( 2, 200, 120);
MappedFloatValue zoomFloatValue( 0.1, 5, 2.f, MappedFloatValue::Mapping::lin, "", 2  );
MappedIntValue baseIntValue( 0, 12, 0.0f, 1, 1 );
MappedFloatValue randFloatValue( 0, 5, 1.f, MappedFloatValue::Mapping::lin, "", 1 );
const char *scaleList[NUMSCALES] = {
  "Maj", "Min", "Pent", "Phry"
};
MappedStringListValue scaleListValue( scaleList, NUMSCALES, 0 );

MappedFloatValue audiomixFloatValue( 0, 2.0f, 0.25f, MappedFloatValue::Mapping::lin,"", 2 );

// mod menu
enum MODTYPE {
  MODTYPE_LFO,
  MODTYPE_RAND,
  MODTYPE_BASS
};
const char*           modtypeListValues[]
    = {"LFO", "RAND", "Bassline" };

MappedStringListValue modtypeListValue( modtypeListValues, 3, 0 );
static Oscillator lfo;
MappedFloatValue modgainFloatValue( 0, 1, 0.5f, MappedFloatValue::Mapping::lin,"", 2 );
MappedFloatValue modfreqFloatValue( 0.01, 2, 0.2f, MappedFloatValue::Mapping::lin,"", 2 );

bool randSequence = false;

int oldModel = 0;

void InitUi()
{
    UI::SpecialControlIds specialControlIds;
    specialControlIds.okBttnId
        = bttnEncoder; // Encoder button is our okay button
    specialControlIds.menuEncoderId
        = encoderMain; // Encoder is used as the main menu navigation encoder

    // This is the canvas for the OLED display.
    UiCanvasDescriptor oledDisplayDescriptor;
    oledDisplayDescriptor.id_     = canvasOledDisplay; // the unique ID
    oledDisplayDescriptor.handle_ = &hw.display;   // a pointer to the display
    oledDisplayDescriptor.updateRateMs_      = 50; // 50ms == 20Hz
    oledDisplayDescriptor.screenSaverTimeOut = 0;  // display always on
    oledDisplayDescriptor.clearFunction_     = &ClearCanvas;
    oledDisplayDescriptor.flushFunction_     = &FlushCanvas;

    ui.Init(eventQueue,
            specialControlIds,
            {oledDisplayDescriptor},
            canvasOledDisplay);
}

void InitUiPages()
{
    // ====================================================================
    // The main menu
    // ====================================================================
    
    mainMenuItems[0].type = daisy::AbstractMenu::ItemType::valueItem;
    mainMenuItems[0].text = "Brightness";
    mainMenuItems[0].asMappedValueItem.valueToModify = &brightnessFloatValue;

    mainMenuItems[1].type = daisy::AbstractMenu::ItemType::valueItem;
    mainMenuItems[1].text = "Polyphony";
    mainMenuItems[1].asMappedValueItem.valueToModify = &polyListValue;

    mainMenuItems[2].type = daisy::AbstractMenu::ItemType::valueItem;
    mainMenuItems[2].text = "Model";
    mainMenuItems[2].asMappedValueItem.valueToModify = &modelListValue;

    mainMenuItems[3].type = daisy::AbstractMenu::ItemType::valueItem;
    mainMenuItems[3].text = "Dly time";
    mainMenuItems[3].asMappedValueItem.valueToModify = &delaytimeFloatValue;

    mainMenuItems[4].type = daisy::AbstractMenu::ItemType::valueItem;
    mainMenuItems[4].text = "Dly fdback";
    mainMenuItems[4].asMappedValueItem.valueToModify = &delayfeedbackFloatValue;

    mainMenuItems[5].type = daisy::AbstractMenu::ItemType::valueItem;
    mainMenuItems[5].text = "Dly mix";
    mainMenuItems[5].asMappedValueItem.valueToModify = &delaymixFloatValue;

    mainMenuItems[6].type = daisy::AbstractMenu::ItemType::openUiPageItem;
    mainMenuItems[6].text = "Plus";
    mainMenuItems[6].asOpenUiPageItem.pageToOpen = &plusEditMenu;

    mainMenuItems[7].type = daisy::AbstractMenu::ItemType::openUiPageItem;
    mainMenuItems[7].text = "Mod";
    mainMenuItems[7].asOpenUiPageItem.pageToOpen = &modEditMenu;

    mainMenuItems[8].type = daisy::AbstractMenu::ItemType::valueItem;
    mainMenuItems[8].text = "Audio Mix";
    mainMenuItems[8].asMappedValueItem.valueToModify = &audiomixFloatValue;

    mainMenuItems[9].type = daisy::AbstractMenu::ItemType::openUiPageItem;
    mainMenuItems[9].text = "Normalize";
    mainMenuItems[9].asOpenUiPageItem.pageToOpen = &normEditMenu;

    mainMenuItems[10].type = daisy::AbstractMenu::ItemType::valueItem;
    mainMenuItems[10].text = "Easter FX";
    mainMenuItems[10].asMappedValueItem.valueToModify = &eggListValue;

    mainMenuItems[11].type = daisy::AbstractMenu::ItemType::checkboxItem;
    mainMenuItems[11].text = "Mono out";
    mainMenuItems[11].asCheckboxItem.valueToModify = &monoOutput;

    mainMenuItems[12].type = daisy::AbstractMenu::ItemType::openUiPageItem;
    mainMenuItems[12].text = "Controls";
    mainMenuItems[12].asOpenUiPageItem.pageToOpen = &controlEditMenu;

    mainMenu.Init(mainMenuItems, kNumMainMenuItems);

    // ====================================================================
    // The "control edit" menu
    // ====================================================================

    controlEditMenuItems[0].type = daisy::AbstractMenu::ItemType::valueItem;
    controlEditMenuItems[0].text = "Ctrl One";
    controlEditMenuItems[0].asMappedValueItem.valueToModify
        = &controlListValueOne;

    controlEditMenuItems[1].type = daisy::AbstractMenu::ItemType::valueItem;
    controlEditMenuItems[1].text = "Ctrl Two";
    controlEditMenuItems[1].asMappedValueItem.valueToModify
        = &controlListValueTwo;

    controlEditMenuItems[2].type = daisy::AbstractMenu::ItemType::valueItem;
    controlEditMenuItems[2].text = "Ctrl Three";
    controlEditMenuItems[2].asMappedValueItem.valueToModify
        = &controlListValueThree;

    controlEditMenuItems[3].type = daisy::AbstractMenu::ItemType::valueItem;
    controlEditMenuItems[3].text = "Ctrl Four";
    controlEditMenuItems[3].asMappedValueItem.valueToModify
        = &controlListValueFour;

    controlEditMenuItems[4].type = daisy::AbstractMenu::ItemType::closeMenuItem;
    controlEditMenuItems[4].text = "Back";

    controlEditMenu.Init(controlEditMenuItems, kNumControlEditMenuItems);

    // ====================================================================
    // The "normalization edit" menu
    // ====================================================================
    normEditMenuItems[0].type = daisy::AbstractMenu::ItemType::checkboxItem;
    normEditMenuItems[0].text = "Exciter";
    normEditMenuItems[0].asCheckboxItem.valueToModify = &exciterIn;

    normEditMenuItems[1].type = daisy::AbstractMenu::ItemType::checkboxItem;
    normEditMenuItems[1].text = "Strum";
    normEditMenuItems[1].asCheckboxItem.valueToModify = &strumIn;

    normEditMenuItems[2].type = daisy::AbstractMenu::ItemType::checkboxItem;
    normEditMenuItems[2].text = "Note";
    normEditMenuItems[2].asCheckboxItem.valueToModify = &noteQuantize;

    normEditMenuItems[3].type = daisy::AbstractMenu::ItemType::checkboxItem;
    normEditMenuItems[3].text = "Easter Egg";
    normEditMenuItems[3].asCheckboxItem.valueToModify = &easterEggOn;

    normEditMenuItems[4].type = daisy::AbstractMenu::ItemType::closeMenuItem;
    normEditMenuItems[4].text = "Back";

    normEditMenu.Init(normEditMenuItems, kNumNormEditMenuItems);

    // ====================================================================
    // The "Plus edit" menu
    // ====================================================================
    plusEditMenuItems[0].type = daisy::AbstractMenu::ItemType::valueItem;
    plusEditMenuItems[0].text = "BPM";
    plusEditMenuItems[0].asMappedValueItem.valueToModify = &bmpFloatValue;

    plusEditMenuItems[1].type = daisy::AbstractMenu::ItemType::valueItem;
    plusEditMenuItems[1].text = "Zoom";
    plusEditMenuItems[1].asMappedValueItem.valueToModify = &zoomFloatValue;

    plusEditMenuItems[2].type = daisy::AbstractMenu::ItemType::valueItem;
    plusEditMenuItems[2].text = "Note offs.";
    plusEditMenuItems[2].asMappedValueItem.valueToModify = &baseIntValue;

    plusEditMenuItems[3].type = daisy::AbstractMenu::ItemType::valueItem;
    plusEditMenuItems[3].text = "Scale";
    plusEditMenuItems[3].asMappedValueItem.valueToModify = &scaleListValue;

    plusEditMenuItems[4].type = daisy::AbstractMenu::ItemType::checkboxItem;
    plusEditMenuItems[4].text = "Randomize";
    plusEditMenuItems[4].asCheckboxItem.valueToModify = &randSequence;

    plusEditMenuItems[5].type = daisy::AbstractMenu::ItemType::checkboxItem;
    plusEditMenuItems[5].text = "Test Mode";
    plusEditMenuItems[5].asCheckboxItem.valueToModify = &testMode;

    plusEditMenuItems[6].type = daisy::AbstractMenu::ItemType::closeMenuItem;
    plusEditMenuItems[6].text = "Back";

    plusEditMenu.Init( plusEditMenuItems, kPlusEditMenuItems );

    // ====================================================================
    // The "mod edit" menu
    // ====================================================================
    modEditMenuItems[0].type = daisy::AbstractMenu::ItemType::valueItem;
    modEditMenuItems[0].text = "CV2 mode";
    modEditMenuItems[0].asMappedValueItem.valueToModify = &modtypeListValue;

    modEditMenuItems[1].type = daisy::AbstractMenu::ItemType::valueItem;
    modEditMenuItems[1].text = "Mod. Freq";
    modEditMenuItems[1].asMappedValueItem.valueToModify = &modfreqFloatValue;

    modEditMenuItems[2].type = daisy::AbstractMenu::ItemType::valueItem;
    modEditMenuItems[2].text = "Mod. Gain";
    modEditMenuItems[2].asMappedValueItem.valueToModify = &modgainFloatValue;

    modEditMenuItems[3].type = daisy::AbstractMenu::ItemType::closeMenuItem;
    modEditMenuItems[3].text = "Back";

    modEditMenu.Init( modEditMenuItems, kModEditMenuItems );
}

void GenerateUiEvents()
{
    if( hw.encoder.RisingEdge() ) {
        eventQueue.AddButtonPressed(bttnEncoder, 1);
    }

    if(hw.encoder.FallingEdge())
        eventQueue.AddButtonReleased(bttnEncoder);

    const auto increments = hw.encoder.Increment();
    if(increments != 0)
        eventQueue.AddEncoderTurned(encoderMain, increments, 12);
}

int old_poly = 0;

void ProcessControls(Patch* patch, PerformanceState* state)
{
    // control settings
    cv_scaler.channel_map_[0] = controlListValueOne.GetIndex();
    cv_scaler.channel_map_[1] = controlListValueTwo.GetIndex();
    cv_scaler.channel_map_[2] = controlListValueThree.GetIndex();
    cv_scaler.channel_map_[3] = controlListValueFour.GetIndex();

    cv_scaler.brightnessOffset = brightnessFloatValue.Get();

    //polyphony setting
    int poly = polyListValue.GetIndex();
    if(old_poly != poly)
    {
        part.set_polyphony(0x01 << poly);
        string_synth.set_polyphony(0x01 << poly);
    }
    old_poly = poly;

    //model settings
    part.set_model((torus::ResonatorModel)modelListValue.GetIndex());
    string_synth.set_fx((torus::FxType)eggListValue.GetIndex());

    // normalization settings
    state->internal_note    = !noteQuantize;
    state->internal_exciter = !exciterIn;
    state->internal_strum   = !strumIn;

    //strum
    state->strum = hw.gate_input[0].Trig();
}

float input[kMaxBlockSize];
float output[kMaxBlockSize];
float aux[kMaxBlockSize];

const float kNoiseGateThreshold = 0.00003f;
float       in_level            = 0.0f;
bool gatePulse = false;
bool ledStatus = false;
int gate1Count = 0;
int cvTicksPerMsec = 5;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();
    GenerateUiEvents();

    PerformanceState performance_state;
    Patch            patch;
    
    ProcessControls(&patch, &performance_state);
    cv_scaler.Read(&patch, &performance_state);

    if(easterEggOn)
    {
        for(size_t i = 0; i < size; ++i)
        {
            input[i] = in[0][i];
        }
        strummer.Process(NULL, size, &performance_state);
        string_synth.Process(
            performance_state, patch, input, output, aux, size);
    }
    else
    {
        // Apply noise gate.
        for(size_t i = 0; i < size; i++)
        {
            float in_sample = in[0][i];
            float error, gain;
            error = in_sample * in_sample - in_level;
            in_level += error * (error > 0.0f ? 0.1f : 0.0001f);
            gain = in_level <= kNoiseGateThreshold
                       ? (1.0f / kNoiseGateThreshold) * in_level
                       : 1.0f;
            input[i] = gain * in_sample;
        }

        strummer.Process(input, size, &performance_state);
        part.Process(performance_state, patch, input, output, aux, size);
    }

    // delay line
    float new_dly_feedback = delayfeedbackFloatValue.Get();
    //new_dly_feedback *= new_dly_feedback * new_dly_feedback;
    if (new_dly_feedback != dly_feedback) {
        dly_feedback = new_dly_feedback;
    }
    float new_dly_time = delaytimeFloatValue.Get();
    if ( new_dly_time != dly_time  ) {
        dly_time +=  (new_dly_time - dly_time) * 0.001f;
        delay.SetDelay( dly_time * samplerate );
    }
    float dly_mix = delaymixFloatValue.Get();

    // internal random sequencer
    if ( randSequence ) {
        randSequence = false;
        drone.Randomize();
    }
    float newbpm = bmpFloatValue.Get();
    if (newbpm != drone.bpm) {
        drone.bpm = newbpm;
    }
    float newZoom = zoomFloatValue.Get();
    if ( newZoom != drone.cliff.cliffzoom ) {
        drone.cliff.cliffzoom = newZoom;
    }
    drone.cliff.cliffscale = scaleListValue.GetIndex();
    drone.cliff.base = baseIntValue.Get();
    int modtype = modtypeListValue.GetIndex();

    drone.testMode = testMode;
    drone.ProcessCV();

    float cv1 = -1;
    float cv2 = -1;
    bool audiotrig = false;
 
    if( drone.v_pulse2 > 0 ) {
        cv1 = drone.v_note2 / 12.f;
    }
    if( drone.v_pulse1 > 0 ) {
        if ( modtype != MODTYPE_BASS ) {
            cv1 = drone.v_note1 / 12.f; // priority to "bass" note if not CV2=bass
        } else {
            cv2 = drone.v_note1 / 12.f; // cv2 = "bass"            
        }
        audiotrig = true;
    }        

    float modgain = modgainFloatValue.Get();
    float modfreq = modfreqFloatValue.Get();

    if ( modtype == MODTYPE_LFO ) {
        lfo.SetFreq( modfreq );
        cv2 = 2.5f * (lfo.Process() + 1.f) * modgain;
        cv2 = clamp( cv2, 0.f, 5.0f);
    }

    if ( modtype == MODTYPE_RAND ) {
        if ( audiotrig ) {        
            cv2 = frand(0.f,5.f) * modgain;
            cv2 = clamp( cv2, 0.f, 5.0f);
        }
    }

    if ( cv1 >= 0 )   
    {
        hw.seed.dac.WriteValue(DacHandle::Channel::ONE,(uint16_t)round( cv1 * 819.2f));
        gate1Count = 3 * cvTicksPerMsec; // 3 msec gate
    }
    if ( cv2 >= 0 )   
    {
        hw.seed.dac.WriteValue(DacHandle::Channel::TWO,(uint16_t)round( cv2 * 819.2f));
    }

    if ( gate1Count > 0) {
        gate1Count--;
    }
    bool trigOut = (gate1Count > 0);
    if ( trigOut !=  gatePulse ) {
        gatePulse = trigOut;
        dsy_gpio_write(&hw.gate_output, gatePulse );
    }

    ledStatus = drone.v_beat == 0;
    hw.seed.SetLed( ledStatus );

    float audioMix = audiomixFloatValue.Get();
    float attMix = 1;
    if (audioMix > 1) { 
        attMix = 2-audioMix;
        if ( attMix < 0) attMix = 0; 
        audioMix = 1;
    }
    audioMix = audioMix * audioMix * 10.0f;

    for(size_t i = 0; i < size; i++)
    {
        float delsig = 0;

         // Handle Delay
        delsig = delay.Read() * dly_feedback;

        out[0][i] = output[i] + delsig * dly_mix;
        out[1][i] = aux[i] + delsig * dly_mix;

        float mono = (out[0][i] + out[1][i]) * 0.5;

        delay.Write( mono );

        if ( monoOutput ) {
            out[0][i] = mono;
        }
        if (attMix < 1) {
            out[0][i] *= attMix;
            out[1][i] *= attMix;
        }
        out[0][i] += in[2][i] * audioMix;
        out[1][i] += in[3][i] * audioMix; 

        drone.ProcessAudio();
        out[2][i] = drone.v_noise * 0.09f;

        out[3][i] = audiotrig? 0.7 : 0;
    }
}

int main(void)
{
    hw.Init();

    //hw.SetAudioBlockSize( 24 );

    samplerate = hw.AudioSampleRate();
    float blocksize  = hw.AudioBlockSize();

    InitUi();
    InitUiPages();
    InitResources();
    ui.OpenPage(mainMenu);

    strummer.Init(0.01f, samplerate / blocksize);
    part.Init(reverb_buffer);
    string_synth.Init(reverb_buffer);

    cv_scaler.Init();

    UI::SpecialControlIds ids;

    cvTicksPerMsec = (int)( 1 + samplerate / (blocksize * 1000) ); 
    drone.Init( samplerate, samplerate / blocksize );
    delay.Init();       
    lfo.Init( samplerate / blocksize );
    lfo.SetWaveform(lfo.WAVE_SIN);
    lfo.SetFreq(0.1);
    lfo.SetAmp(1);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        ui.Process();
    }
}
