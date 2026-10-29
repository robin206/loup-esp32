#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <string>
#include <algorithm>
#define TWO_PI 6.283185307179586476925286766559f
#define constrain(a,lo,hi) ((a)<(lo)?(lo):((a)>(hi)?(hi):(a)))
extern uint64_t g_us;
inline uint32_t millis(){ return (uint32_t)(g_us/1000); }
inline void delay(uint32_t ms){ g_us += (uint64_t)ms*1000; }
inline long random(long m){ return rand()%m; }
struct String {
  std::string s;
  String(){}
  String(const char* c):s(c?c:""){}
  String(const std::string& x):s(x){}
  String(long n):s(std::to_string(n)){}
  String(int n):s(std::to_string(n)){}
  const char* c_str() const { return s.c_str(); }
  size_t length() const { return s.size(); }
  void trim(){ size_t a=s.find_first_not_of(" \t\r\n"); if(a==std::string::npos){s="";return;} size_t b=s.find_last_not_of(" \t\r\n"); s=s.substr(a,b-a+1);}
  void toLowerCase(){ for(auto&c:s) c=tolower(c);}
  bool startsWith(const char* p) const { return s.rfind(p,0)==0; }
  String substring(size_t i) const { return String(s.substr(i)); }
  long toInt() const { return atol(s.c_str()); }
  bool operator==(const char* o) const { return s==o; }
  String operator+(const String& o) const { return String(s+o.s); }
  friend String operator+(const char* a,const String& b){ return String(std::string(a)+b.s); }
};
struct SerialMock {
  void begin(int){}
  void print(const char* c){ if(verbose) printf("%s",c);} void print(const String& c){ if(verbose) printf("%s",c.c_str());}
  void println(const char* c){ if(verbose) printf("%s\n",c);} void println(const String& c){ if(verbose) printf("%s\n",c.c_str());}
  bool verbose=true;
};
extern SerialMock Serial;
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x)
#define portEXIT_CRITICAL(x)
