#pragma once

#include <stdlib.h>

#include "sandrack.hpp"

using namespace sandrack;

#define CLIFFLEN 10
#define NUMCOEFF 8

struct Cliff
{
    int diatonic[CLIFFLEN]
        = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16 };

//        = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24};
//    int diatonic[CLIFFLEN]
//        = {0, 2, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19, 20, 22, 24};

    float x;
    float y;
    float coeff[NUMCOEFF] = {0, 0, 0, 0, 0, 0, 0, 0};

    float cliffzoom = 3;
    int base = 0;

    void Randomize()
    {
        for(int i = 0; i < NUMCOEFF; i++)
        {
            coeff[i] = frands(0.1, 5);
            //println( coeff[i] );
        }
        coeff[2] = frand(-1.3, 1.3);
        coeff[3] = frand(-1.3, 1.3);
        coeff[6] = frand(-1.3, 1.3);
        coeff[7] = frand(-1.3, 1.3);
        x        = frand(-2, 2);
        y        = frand(-2, 2);
    }

    int NextNote(int ch)
    {
        int k = (irand(0, 3) == 0) ? 4 : 0;

        float x1 = std::sin(coeff[k + 0] * y)
                   + coeff[k + 2] * std::cos(coeff[k + 0] * x);
        float y1 = std::sin(coeff[k + 1] * x)
                   + coeff[k + 3] * std::cos(coeff[k + 1] * y);

        x        = x1;
        y        = y1;
        int note = 0;
        if(ch == 0)
        {
            note = diatonic[( base + fround(std::abs(x * cliffzoom)))
                              % CLIFFLEN];
        }
        else
        {
            note = diatonic[(base + fround(std::abs(y * cliffzoom)))
                              % CLIFFLEN] + 12;
        }
        return note;
    }
};
