#include <cstdint>
#include <vector>
#include <string>
#include <cstdio>
uint64_t g_us=0;
#include "Arduino.h"
#include "BLEDevice.h"
SerialMock Serial;
std::vector<BLECharacteristic*> g_chars;
#include "FastLED.h"
FastLEDMock FastLED;
// --- enregistrement des frames
struct Frame{ uint64_t t; std::string px; };
std::vector<Frame> frames; std::vector<std::pair<uint64_t,std::string>> events;
void FastLEDMock::show(){
  char hex[8]; std::string s; bool uni=true;
  for(int i=0;i<300;i++){ if(!(buf[i]==buf[0])) {uni=false;break;} }
  int cnt = uni?1:300;
  for(int i=0;i<cnt;i++){ snprintf(hex,8,"%02x%02x%02x",buf[i].r,buf[i].g,buf[i].b); s+=hex; }
  frames.push_back({g_us,(uni?"u":"f")+s});
  g_us += 9000; // ~300 LEDs x 30 us
}
void on_notify(const std::string& v){ events.push_back({g_us,"notify:"+v}); }
#include "loup_garou_v1_7b.ino"

static void send(const char* cmd){
  BLECharacteristic* rx=g_chars[0];
  rx->value=cmd; rx->cb->onWrite(rx);
  for(int i=0;i<1000000 && hasPendingCmd;i++){ loop(); }
}
static std::string lastNotify;
int main(int argc,char**argv){
  Serial.verbose=false; setup(); clientConnected=true;
  const char* amb[]={"off","jour","nuit","vote"};
  const char* anims[]={"loup","clocher","horloge","tonnerre","violon"};
  FILE* f=fopen(argv[1],"w");
  fprintf(f,"{\"runs\":{");
  bool first=true;
  auto dump=[&](const std::string& key,uint64_t t0){
    fprintf(f,"%s\"%s\":{\"f\":[",first?"":",",key.c_str()); first=false;
    std::string prev; bool fst=true;
    for(auto& fr:frames){ if(fr.px==prev) continue; prev=fr.px; std::string p=fr.px.substr(1);
      fprintf(f,"%s[%llu,\"%s\"]",fst?"":",",(unsigned long long)((fr.t-t0)/1000),p.c_str()); fst=false; }
    uint64_t done=0; for(auto&e:events) if(e.second.rfind("notify:",0)==0) done=e.first-t0;
    fprintf(f,"],\"done\":%llu}",(unsigned long long)(done/1000));
  };
  for(int a=0;a<4;a++){
    for(int b=0;b<4;b++){ if(a==b) continue;           // transitions (b>0 uniquement utile)
      send(amb[a]); frames.clear(); events.clear(); g_us+=1000000; uint64_t t0=g_us; send(amb[b]);
      dump(std::string("t:")+amb[a]+">"+amb[b],t0); }
    for(const char* an: anims){
      send(amb[a]); frames.clear(); events.clear(); g_us+=1000000; uint64_t t0=g_us; send(an);
      dump(std::string("a:")+amb[a]+":"+an,t0); }
  }
  fprintf(f,"}}\n"); fclose(f); printf("ok\n");
}
