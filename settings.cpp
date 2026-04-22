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

void Settings::printErrorMessage(uint8_t e, bool eol = true)
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
  if (eol)
    Serial.println();
}
short Settings::Add(const uint16_t &index, const char *name)
{
  return 0;
}
void Settings::NewValue(int i, const char* v)
{
  Free(i);
  listdata[i].value = malloc(strlen(v) + 1);
  listdata[i].val_type = Value_type::simbol;
  strcpy((char *)listdata[i].value, v);
}

void Settings::NewValue(int i, const int v)
{
  Free(i);
  listdata[i].value = malloc(sizeof(v));
  *((int* )listdata[i].value) = v;
  listdata[i].val_type = Value_type::number;
}

void Settings::Free(int i)
{
  if(listdata[i].value)
  {
    free(listdata[i].value);
    listdata[i].value = nullptr;
    listdata[i].val_type = Value_type::none;
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
  const size_t bufferLen = 100;
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
  
  for(uint16_t i = 0; i < count_settings; i++)
  {
    if(!ini.getValue(ns, listdata[i].name, buffer, bufferLen))
    {
      printErrorMessage(ini.getError());
      return -2 - i;
    }
    char *endptr;
	  int tmp = strtoul(buffer, &endptr, 10);
	  if(endptr == buffer) //string
    {
      NewValue(i, buffer);
      debug_str(listdata[i].name, ToChar(listdata[i].index));
    }
    else
	    if(*endptr == '\0') //number
      {
		    NewValue(i, tmp);
        debug_str(listdata[i].name, ToInt(listdata[i].index));
	    }
    
  }
  ini.close();
  return 0;
}
