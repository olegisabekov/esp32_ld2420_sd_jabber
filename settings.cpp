#include <cstring>
#include "settings.h"
#include "debug_str.h"

#define CS_PIN 5

short Settings::initSDCard(void)
{
  if(!SD.begin(CS_PIN))
  {
    debug_str("Card Mount Failed");
    return -1;
  }
  uint8_t cardType = SD.cardType();
 
  if(cardType == CARD_NONE)
  {
    debug_str("No SD card attached");
    return -2;
  }
 
  debug_str("SD Card Type: ");
  switch(cardType)
  {
    case CARD_MMC: 
      debug_str("MMC");
      break;
    case CARD_SD: 
      debug_str("SDSC");
      break;
    case CARD_SDHC: 
      debug_str("SDHC");
      break;
    default:
      debug_str("UNKNOWN");
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  return 0;
}
#ifdef TESTMODE
void Settings::printErrorMessage(uint8_t e)
{
  switch (e) {
  case IniFile::errorNoError:
    debug_str("no error");
    break;
  case IniFile::errorFileNotFound:
    debug_str("file not found");
    break;
  case IniFile::errorFileNotOpen:
    debug_str("file not open");
    break;
  case IniFile::errorBufferTooSmall:
    debug_str("buffer too small");
    break;
  case IniFile::errorSeekError:
    debug_str("seek error");
    break;
  case IniFile::errorSectionNotFound:
    debug_str("section not found");
    break;
  case IniFile::errorKeyNotFound:
    debug_str("key not found");
    break;
  case IniFile::errorEndOfFile:
    debug_str("end of file");
    break;
  case IniFile::errorUnknownError:
    debug_str("unknown error");
    break;
  default:
    debug_str("unknown error value");
    break;
  }
}
#endif
short Settings::Add(const uint8_t &index, const char *name)
{
  if(count >= max_settings)
    return -1;
  listdata[count].index = index;
  listdata[count].name = (char *)malloc(strlen(name) + 1);
  listdata[count].val_type = Value_type::empty;
  strcpy((char *)listdata[count].name, name);
  count++;
  return 0;
}
short Settings::Add(const uint8_t &index, const char *name, const char* defvalue)
{
  if(count >= max_settings)
    return -1;
  listdata[count].index = index;
  listdata[count].name = (char *)malloc(strlen(name) + 1);
  listdata[count].val_type = Value_type::empty;
  strcpy((char *)listdata[count].name, name);
  if(defvalue)
  {
    listdata[count].defvalue = (char *)malloc(strlen(defvalue) + 1);
    strcpy((char *)listdata[count].defvalue, defvalue);
  }
  count++;
  return 0;
}
void Settings::NewValue(uint8_t i, const char* v)
{
  FreeValue(i);
  listdata[i].value = malloc(strlen(v) + 1);
  listdata[i].val_type = Value_type::simbol;
  strcpy((char *)listdata[i].value, v);
}

void Settings::NewValue(uint8_t i, const int v)
{
  FreeValue(i);
  listdata[i].value = malloc(sizeof(v));
  *((int* )listdata[i].value) = v;
  listdata[i].val_type = Value_type::number;
}

void Settings::FreeValue(uint8_t i)
{
  if(listdata[i].value)
  {
    free(listdata[i].value);
    listdata[i].value = nullptr;
    listdata[i].val_type = Value_type::empty;
  }
}

short Settings::Read(const char* ns)
{
  IniFile ini(FileName);
  if (!ini.open()) 
  {
    debug_str("Ini file does not exist", FileName);
    return -1;
  }
  const size_t bufferLen = 150;
  char buffer[bufferLen];
    // Check the file is valid. This can be used to warn if any lines
  // are longer than the buffer.
  if (!ini.validate(buffer, bufferLen)) 
  {
    debug_str("ini file", ini.getFilename());
    debug_str("not valid", ini.getError());
    return -2;
  }
  debug_str("Ini file opened successfully. Reading settings:");
  
  for(uint8_t i = 0; i < count; i++)
  {
    if(!ini.getValue(ns, listdata[i].name, buffer, bufferLen))
    {
      #ifdef TESTMODE
      printErrorMessage(ini.getError());
      #endif
      if(listdata[i].defvalue)
      {
        #ifdef TESTMODE
        debug_str("set default");
        #endif
        strcpy(buffer, listdata[i].defvalue);
      }
      else
        return -2 - i;
    }
    char *endptr;
	  int tmp = strtoul(buffer, &endptr, 10);
	  if(endptr == buffer) //string
    {
      NewValue(i, buffer);
      #ifdef TESTMODE
      debug_str(listdata[i].name, ToChar(listdata[i].index));
      #endif
    }
    else
	    if(*endptr == '\0') //number
      {
		    NewValue(i, tmp);
        #ifdef TESTMODE
        debug_str(listdata[i].name, ToInt(listdata[i].index));
        #endif
	    }
  }
  ini.close();
  return 0;
}
