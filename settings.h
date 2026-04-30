#include <string.h>
#include "SD.h"
#include "SPI.h"
#include <IniFile.h>

const uint8_t max_settings = 12;

class Settings
{
private:
  uint8_t count = 0;
  const char* str_empty = "";
  const int zerro = 0;
  char* FileName;
  char* NameSection;
  enum Value_type { none, empty, simbol, number };
  // list setting
  struct Data
  {
    uint8_t index;
    const char* name;
    const char* defvalue = nullptr;
    void* value = nullptr;
    Value_type val_type = Value_type::none;
  };
  Data listdata[max_settings];
  void NewValue(uint8_t, const char*);
  void NewValue(uint8_t, const int);
  void FreeValue(uint8_t);
#ifdef TESTMODE
  void printErrorMessage(uint8_t);
#endif
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
  short Add(const uint8_t &, const char *);
  short Add(const uint8_t &, const char *, const char *);
  short Read(const char *);
  const char* ToChar(const uint8_t& index)
  {
    if(index > count)
      return str_empty;
    for(uint8_t i = 0; i < count; i++)
    {
      if(listdata[i].index == index)
        if(listdata[i].val_type == Value_type::simbol)
          if(listdata[i].value)
            return (char *)listdata[index - 1].value;
    }
    return str_empty;
  }
  const int& ToInt(const uint8_t& index)
  {
    if(index > count)
      return zerro;
    for(uint8_t i = 0; i < count; i++)
    {
      if(listdata[i].index == index)
        if(listdata[i].val_type == Value_type::number)
          if(listdata[i].value)
            return *((int* )listdata[i].value);
    }
    return zerro;
  }
};