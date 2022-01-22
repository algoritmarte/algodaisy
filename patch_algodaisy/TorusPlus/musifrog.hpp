#ifndef MUSIFROG_HPP
#define MUSIFROG_HPP


#define SCALELEN 20
#define NUMSTEPS 32
#define JUMPTABLEN 5

#include "sandrack.hpp"

struct Musifrog {

    int scale[SCALELEN] = { 0,2,4,5,7,9,11,12,-1,14,16,14,12,11,9,7,5,4,2,-1 };
    int jumpindex[SCALELEN] = {};
    int frogpos = 0;
    int jumptab[JUMPTABLEN] = {};

    void Init() {
        Randomize();
    }

    void Randomize() {
        for (int i = 0; i < JUMPTABLEN; i++) {
            jumptab[i] = (i == 0)? 1 : irand(1,SCALELEN/2);
        }

        for (int i = 0; i < SCALELEN; i++) {
            jumpindex[i] = irand(0,JUMPTABLEN-1);
        }

        frogpos = irand(0,SCALELEN-1);
    }

    int NextNote() {
        int res = scale[ frogpos ];
        int jump = jumptab[ jumpindex[ frogpos ] ];
        jumpindex[ frogpos ] = ( jumpindex[ frogpos ] + 1) % JUMPTABLEN;
        frogpos = ( frogpos + jump ) % SCALELEN;
        //frogpos = (frogpos + 1) % SCALELEN;
        return res;
    }

};




#endif