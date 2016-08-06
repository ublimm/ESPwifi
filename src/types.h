#ifndef types_h
#define types_h

typedef struct {
  int valid;              // 0=no configuration, 1=valid configuration
  char SSID[32];          //
  char password[32];      //
  char MQTTHost[16];      //192.168.002.127
  char MQTTChannel[62];
} eepConfigData_t;

#endif
