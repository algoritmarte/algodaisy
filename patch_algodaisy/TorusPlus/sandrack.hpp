#ifndef SANDRACK_HPP
#define SANDRACK_HPP

#include <stdlib.h>
#include <string.h>

namespace sandrack {

const float F_PI = 2 * acos(0.0f);

inline int irand(int min, int max) {
	return rand()%(max-min + 1) + min;
}

inline float frand() {
 return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
}

inline float frand( float min, float max ) {
	return min + frand() * (max-min);
}

inline float frands() {
	return 2.0f*( frand() - 0.5f);
}

inline float frands(float min, float max) {
	return frand(min, max) * (irand(0,1)==0? 1 : -1);
}

inline float clamp( float v, float vmin, float vmax ) {
	return std::max(vmin, std::min(v, vmax));
}

inline float midi2freq( int midinote ) {
	return powf(2, (midinote - 69.0f) / 12.0f) * 440.0f;;
}

inline int fround( float v ) {
    return floor( v + 0.5 );
}

}


#endif