#include <string.h>
#include "SD.h"
#include "SPI.h"
#include <IniFile.h>

const int count_settings = 11;

class Settings
{
private:
  uint16_t count = 0;
  const char* empty = "";
  const int zerro = 0;
  char* FileName;
  char* NameSection;
  enum Value_type { none, simbol, number };
  // list setting
  struct Data
  {
    uint16_t index;
    const char* name;
    void* value = nullptr;
    Value_type val_type = Value_type::none;
  };
  Data listdata[count_settings] = {
    {1, "tz_zone_info"},
    {2, "ssid"},
    {3, "password"},
    {4, "server"},
    {5, "name_sensor"},
    {6, "ntp_server"},
    {7, "recipient"},
    {8, "passxmpp"},
    {9, "port"},
    {10, "sleep"},
    {11, "resource"}
  };
  void NewValue(int, const char*);
  void NewValue(int, const int);
  void Free(int);
  void printErrorMessage(uint8_t , bool );
public:
  Settings() {;};
  Settings(char* fn) { FileName = fn; };
  void SetFileName(char* fn)
  {
    FileName = fn;
  }
  const char* GetFileName(void)
  {
    return FileName;
  }
  short initSDCard(void);
  short Add(const uint16_t &, const char *);
  short Read(const char *);
  const char* ToChar(const uint16_t& index)
  {
    if(index > count_settings)
      return empty;
    for(uint16_t i = 0; i < count_settings; i++)
    {
      if(listdata[i].index == index)
        if(listdata[i].val_type == Value_type::simbol)
          if(listdata[i].value)
            return (char *)listdata[index - 1].value;
    }
    return empty;
  }
  const int& ToInt(const uint16_t& index)
  {
    if(index > count_settings)
      return zerro;
    for(uint16_t i = 0; i < count_settings; i++)
    {
      if(listdata[i].index == index)
        if(listdata[i].val_type == Value_type::number)
          if(listdata[i].value)
            return *((int* )listdata[i].value);
    }
    return zerro;
  }
};
