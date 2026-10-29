#pragma once
#include "Arduino.h"
#include <vector>
class BLECharacteristic;
class BLECharacteristicCallbacks { public: virtual ~BLECharacteristicCallbacks(){} virtual void onWrite(BLECharacteristic*){} };
class BLEServerCallbacks;
class BLEDescriptor {};
class BLE2902 : public BLEDescriptor {};
class BLECharacteristic {
public:
  static const int PROPERTY_WRITE=1, PROPERTY_NOTIFY=2;
  std::string value; BLECharacteristicCallbacks* cb=nullptr;
  std::string getValue(){ return value; }
  void setValue(uint8_t* d,size_t n){ value.assign((char*)d,n);} 
  void notify(){ extern void on_notify(const std::string&); on_notify(value);} 
  void setCallbacks(BLECharacteristicCallbacks* c){cb=c;}
  void addDescriptor(BLEDescriptor*){}
};
class BLEService { public: BLECharacteristic* createCharacteristic(const char*,int){ auto c=new BLECharacteristic(); extern std::vector<BLECharacteristic*> g_chars; g_chars.push_back(c); return c;} void start(){} };
class BLEServer;
class BLEServerCallbacks { public: virtual ~BLEServerCallbacks(){} virtual void onConnect(BLEServer*){} virtual void onDisconnect(BLEServer*){} };
class BLEServer { public: void setCallbacks(BLEServerCallbacks*){} BLEService* createService(const char*){ return new BLEService(); } };
class BLEAdvertising { public: void addServiceUUID(const char*){} void setScanResponse(bool){} void setMinPreferred(int){} void setMaxPreferred(int){} };
class BLEDevice { public: static void init(const char*){} static BLEServer* createServer(){ return new BLEServer(); } static BLEAdvertising* getAdvertising(){ static BLEAdvertising a; return &a;} static void startAdvertising(){} };
