#ifndef DRONE_HPP
#define DRONE_HPP

#include <string>

#include "daisysp.h"

#include "sandrack.hpp"
#include "cliff.hpp"
#include "euclide.hpp"

using namespace daisysp;

struct Drone {

    WhiteNoise noise = {};
    Cliff cliff = {};
    EuclideSet eset = {};

    const int test_notes[8] = { 0, 2, 4, 5, 7, 9, 11, 12 };

    float audioRate = 44100;
    float cvRate = 44100;
    float bpm = 60;

    float v_noise = 0;

    int v_beat = 0;
    int v_pulse = 0;
    int v_note = -1;

    float tick = 0;
    float tickoff = 0;
    bool testMode = false;
    int itest = 0;

    void Init(float audioSampleRate, float cvSampleRate = 0 ) {
        audioRate = audioSampleRate;
        cvRate = (cvSampleRate > 0)? cvSampleRate : audioSampleRate;
        noise.Init();
        Randomize();
    }

    void Randomize() {
        cliff.Randomize();
        eset.Randomize();
    }

    void ProcessCV() {
        v_pulse = 0;
        
        float elapsed = tickoff + tick / cvRate; // secs elapsed
        tickoff = 0;
        float beatlen = 60.0f / bpm; // beat duration
        float steplen = beatlen / 2.0f;
        if ( elapsed >= steplen ) {
            tickoff = elapsed - steplen;
            tick = 0;
            v_beat = (v_beat + 1) % 2;

            if ( testMode ) {
                if (v_beat == 0) {
                    v_pulse = 1;
                    v_note = test_notes[ itest++ % 8];
                }
            } else {
                eset.Step();
                if ( eset.m_values[0] || eset.m_values[1] ) {
                    v_pulse = 1;
                    v_note = cliff.NextNote( 0 ); 
                } else {
                    if ( eset.m_values[2] || eset.m_values[3] ) {
                        v_pulse = 1;
                        v_note = cliff.NextNote( 1 ); 
                    }
                }                
            }
        }
        tick++;
    }

    void ProcessAudio()  {
        v_noise = noise.Process();
    }
};

#endif