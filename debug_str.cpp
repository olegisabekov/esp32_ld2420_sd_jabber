#include <Arduino.h>
#include "debug_str.h"

void debug_str( const char* intro, const char* message) 
{
  if(Flg_debug)
  {
    Serial.printf("%s: %s\n", intro, message);
	  Serial.flush();
  }
}

void debug_str( const char* intro, const uint16_t v) 
{
  if(Flg_debug)
  {
    Serial.printf("%s: %d\n", intro, v);
	  Serial.flush();
  }
}

void debug_str( const char* intro, const int v, const int v2) 
{
  if(Flg_debug)
  {
    Serial.printf("%s: %d - %d\n", intro, v, v2);
	  Serial.flush();
  }
}

void debug_str( const char* message) 
{
  if(Flg_debug)
  {
    Serial.println(message);
	  Serial.flush();
  }
}
