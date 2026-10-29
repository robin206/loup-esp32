#pragma once
#include "Arduino.h"
typedef uint8_t fract8;
inline uint8_t scale8(uint8_t i,uint8_t s){ return ((uint16_t)i*(1+(uint16_t)s))>>8; }
inline uint8_t lerp8by8(uint8_t a,uint8_t b,fract8 f){ return b>a ? a+scale8(b-a,f) : a-scale8(a-b,f); }
inline uint8_t ease8InOutQuad(uint8_t i){ uint8_t j=i; if(j&0x80) j=255-j; uint8_t jj=scale8(j,j); uint8_t jj2=jj<<1; if(i&0x80) jj2=255-jj2; return jj2; }
struct CRGB {
  uint8_t r,g,b;
  CRGB():r(0),g(0),b(0){}
  CRGB(uint8_t R,uint8_t G,uint8_t B):r(R),g(G),b(B){}
  bool operator==(const CRGB& o) const { return r==o.r&&g==o.g&&b==o.b; }
  static const CRGB White, Black;
};
inline const CRGB CRGB::White(255,255,255);
inline const CRGB CRGB::Black(0,0,0);
inline CRGB blend(const CRGB& a,const CRGB& b,fract8 f){ return CRGB(lerp8by8(a.r,b.r,f),lerp8by8(a.g,b.g,f),lerp8by8(a.b,b.b,f)); }
inline void fill_solid(CRGB* l,int n,const CRGB& c){ for(int i=0;i<n;i++) l[i]=c; }
#define WS2812B 0
#define GRB 0
struct FastLEDMock {
  CRGB* buf=nullptr; int n=0; int bright=255;
  template<int T,int P,int O> void addLeds(CRGB* l,int count){ buf=l; n=count; }
  void setMaxPowerInVoltsAndMilliamps(int,int){}
  void setBrightness(int b){ bright=b; }
  void show();
};
extern FastLEDMock FastLED;
template<int T,int P,int O> struct _dummy{};
