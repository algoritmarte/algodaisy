#ifndef EUCLIDE_HPP
#define EUCLIDE_HPP

#include "sandrack.hpp"

#define EUC_NUMSEQ 4
#define EUC_MAXSEQLEN 32

struct EuclideSeq {
    int m_seqlen = 16;
    int m_seqsteps = 4;
    int m_seqshift = 0;
    int m_seq[EUC_MAXSEQLEN] = {};
    int currstep = 0;
    float m_prob = 1;

    int Next() {
        int res = m_seq[ (currstep + m_seqshift ) % m_seqlen ];
        currstep = (currstep + 1 ) % m_seqlen;
        if (m_prob <= 0) {
            res = 0;
        } else {
            if (m_prob < 1) {
                if ( sandrack::frand() > m_prob ) res = 0;
            }        
        }
        return res;
    }

    void Randomize() {
        int len = 16;
        //int steps = irand( 3, 12 );
        int steps = irand( 2, 9 );
        int shift = irand( 0, len-1);
        float prob = frand( 0.75, 1 );
        Configure( len, steps, shift, prob );
    }

    void Configure( int seqlen, int seqsteps, int seqshift, float prob ) {
        if (seqlen > 0) m_seqlen = seqlen;
        if ( m_seqsteps > m_seqlen) m_seqsteps = m_seqlen;
        m_seqsteps = seqsteps;
        m_seqshift = seqshift;
        m_prob = prob;
        RebuildSeq();
    }

    void RebuildSeq()  {
        float v = 0;
        for (int i = 0; i < EUC_MAXSEQLEN; i++) m_seq[i] = 0;
        if ( m_seqsteps <= 0) return;
        float delta = 1.0f * m_seqlen / m_seqsteps;
        float s = sandrack::fround(v); 
        while ( s < m_seqlen ) {
            m_seq[ (int)( s ) ] = 1;
            v += delta;
            s = sandrack::fround(v);
        }
    }

};

struct EuclideSet {

    EuclideSeq seqs[EUC_NUMSEQ] = {};
    int m_values[EUC_NUMSEQ] = {};

    void Init() {

    }

    void Randomize() {
        for (int i = 0; i < EUC_NUMSEQ; i++ ) {
            seqs[i].Randomize();
        }
    }

    void Step() {
        for (int i = 0; i < EUC_NUMSEQ; i++) {
            m_values[i] = seqs[i].Next();
        }
    }

    void Reset() {
        for (int i = 0; i < EUC_NUMSEQ; i++) {
            m_values[i] = seqs[i].currstep = 0;
        }
    }

};

#endif