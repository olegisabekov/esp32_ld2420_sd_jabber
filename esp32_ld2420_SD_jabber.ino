#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "SD.h"
#include "SPI.h"
#include <IniFile.h>
#include <WiFiClientSecure.h>
#include "driver/rtc_io.h" // power up in sleep
#include "Xmpp.h"
#include "LD2420.h"

//blink led
#define INFO_LED GPIO_NUM_32
#define ERROR_LED GPIO_NUM_25
#define PWR_PIN GPIO_NUM_36
// Define RX and TX pins for SoftwareSerial
#define RX_PIN GPIO_NUM_16
#define TX_PIN GPIO_NUM_17
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex
#define WAKEUP_GPIO GPIO_NUM_4  // Only RTC IO are allowed - ESP32 Pin example
#define CS_PIN 5
#define MYTZ "SAMT-4"

static const char root_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

char Ssid[40];
char Password[40];
char NtpServerName[50];
char Server[30];
uint16_t Port = 5222;
char Tz_zone_info[40];
char Recipient[30];
char NameSensor[40];
char PassXmpp[40];
char Resource[40];

static const char *NameSection = "general";
const char* Filename = "/settings.ini";

WiFiClientSecure client;
XMPP xmpp;
// Create instances
HardwareSerial sensorSerial(2);
LD2420 radar;  // Use default constructor

const uint8_t Flg_debug = 0;

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
// Detection zones
struct DetectionZone {
  unsigned int index;
  int minDistance;
  int maxDistance;
  String name;
  bool isActive;
  unsigned long lastDetection;
};

DetectionZone zones[] = {
  {1, 0, 50, "Очень близко", false, 0},
  {2, 51, 150, "Близко", false, 0},
  {3, 151, 200, "Рядом", false, 0},
  {4, 201, 300, "Далеко", false, 0},
  {5, 301, 601, "Очень далеко", false, 0}
};

const int NUM_ZONES = sizeof(zones) / sizeof(zones[0]);

// Data filtering
const int FILTER_SIZE = 8;
int distanceFilter[FILTER_SIZE];
int filterIndex;
bool filterFull;

volatile long filteredDistance;
volatile unsigned long CurrentIndexZone;
long oldIndexZone;
unsigned long WaitNotActive = 60000; // 2 min

void setup_wifi() {
  debug_str("\nConnecting to ");
  WiFi.mode(WIFI_STA); // Set ESP32 to Station mode (to connect to an AP) [1, 6]
  WiFi.begin(Ssid, Password);
  while (WiFi.status() != WL_CONNECTED) 
  { // Wait for connection
    delay(500);
    Serial.print(".");
  }
  debug_str("WiFi connected");
  debug_str("IP address", WiFi.localIP());
}

void wifi_off()
{
  debug_str("wifi off");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(10);
}

void ResetAllZones(void)
{
  CurrentIndexZone = 0;
  for( int i = 0; i < NUM_ZONES; i++) 
  {
    zones[i].isActive = false;
  }
  debug_str("All zones cleared");
}

short initSDCard()
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

void printErrorMessage(uint8_t e, bool eol = true)
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

void ReadSettings(void)
{
  IniFile ini(Filename);
  if (!ini.open()) 
  {
    debug_str("Ini file does not exist", Filename);
    digitalWrite( ERROR_LED, HIGH );
    // Cannot do anything else
    while (1);
  }
  const size_t bufferLen = 100;
  char buffer[bufferLen];
    // Check the file is valid. This can be used to warn if any lines
  // are longer than the buffer.
  if (!ini.validate(buffer, bufferLen)) 
  {
    debug_str("ini file", ini.getFilename());
    debug_str("not valid", ini.getError());
    digitalWrite( ERROR_LED, HIGH );
    // Cannot do anything else
    while (1);
  }
  debug_str("Ini file opened successfully. Reading settings:");
  
  if(!ini.getValue(NameSection, "tz_zone_info", buffer, sizeof(buffer)))
    printErrorMessage(ini.getError());
  strcpy(Tz_zone_info, buffer);
  debug_str("Tz_zone_info: %s\n", Tz_zone_info );
  
  if( strlen(Tz_zone_info) == 0)
    strcpy(Tz_zone_info, MYTZ);

  if(!ini.getValue(NameSection, "ssid", buffer, sizeof(buffer)))
  {
    printErrorMessage(ini.getError());
    digitalWrite( ERROR_LED, HIGH );
  }
  strcpy(Ssid, buffer);
  debug_str("Ssid", Ssid );
  
  if(!ini.getValue(NameSection, "password", buffer, sizeof(buffer)))
    printErrorMessage(ini.getError());
  strcpy(Password, buffer);
  debug_str("Password", Password );
  
  if(!ini.getValue(NameSection, "server", buffer, sizeof(buffer)))
  {
    printErrorMessage(ini.getError());
    digitalWrite( ERROR_LED, HIGH );
  }
  strcpy(Server, buffer);
  debug_str("Server", Server );
  
  if(!ini.getValue(NameSection, "name_sensor", buffer, sizeof(buffer)))
  {
    printErrorMessage(ini.getError());
    digitalWrite( ERROR_LED, HIGH );
  }
  debug_str("NameSensor", NameSensor);
  
  if(!ini.getValue(NameSection, "ntp_server", buffer, sizeof(buffer)))
  {
    printErrorMessage(ini.getError());
    digitalWrite( ERROR_LED, HIGH );
  }
  strcpy(NtpServerName, buffer);
  debug_str("NtpServerName", NtpServerName);

  if(!ini.getValue(NameSection, "recipient", buffer, sizeof(buffer)))
  {
    printErrorMessage(ini.getError());
    digitalWrite( ERROR_LED, HIGH );
  }
  strcpy(Recipient, buffer);
  debug_str("Recipient", Recipient);

  if(!ini.getValue(NameSection, "passxmpp", buffer, sizeof(buffer)))
  {
    printErrorMessage(ini.getError());
    digitalWrite( ERROR_LED, HIGH );
  }
  strcpy(PassXmpp, buffer);
  debug_str("PassXmpp", PassXmpp);
  
  if(!ini.getValue(NameSection, "port", buffer, sizeof(buffer), Port))
  {
    printErrorMessage(ini.getError());
    digitalWrite( ERROR_LED, HIGH );
  }
  debug_str("Port", Port);

  unsigned long smi;
  if(!ini.getValue(NameSection, "sleep", buffer, sizeof(buffer), smi))
    printErrorMessage(ini.getError());
  else
  {
    WaitNotActive = smi * 10000;
    debug_str("WaitNotActive", WaitNotActive);
  }
  // Close the file when done reading
  ini.close();
}

// Configuration
void setup() 
{
  pinMode( INFO_LED, OUTPUT );
  pinMode( ERROR_LED, OUTPUT );
  pinMode( WAKEUP_GPIO, INPUT_PULLDOWN );
  digitalWrite( INFO_LED, HIGH);
  
  
  // Initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  if(initSDCard() == 0)
    ReadSettings();
  else
    digitalWrite( ERROR_LED, HIGH );
  
  esp_sleep_enable_ext1_wakeup_io(BUTTON_PIN_BITMASK(WAKEUP_GPIO), ESP_EXT1_WAKEUP_ANY_HIGH);
  rtc_gpio_pulldown_en(WAKEUP_GPIO);  // GPIO4 is tie to GND in order to wake up in HIGH
  rtc_gpio_pullup_dis(WAKEUP_GPIO);   // Disable PULL_UP in order to allow it to wakeup on HIGH
    
  setup_wifi();

  debug_str("=== LD2420 Advanced Example ===");
  // Initialize SoftwareSerial for LD2420
  oldIndexZone = -1;
  filteredDistance = 0;
  CurrentIndexZone = 0;
  sensorSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  debug_str("SoftwareSerial initialized on RX:%d, TX:%d\n", RX_PIN, TX_PIN);
  
  // Initialize the radar sensor
  if (radar.begin(sensorSerial)) 
    debug_str("LD2420 initialized successfully!");
  else 
  {
    debug_str("Failed to initialize LD2420!");
    digitalWrite( ERROR_LED, HIGH );
    while (1) delay(1000);
  }
  // Configure sensor
  radar.setDistanceRange(0, 500);
  radar.setUpdateInterval(50);
  
  // Set up callbacks
  radar.onDetection(onDetectionEvent);
  radar.onDataUpdate(onDataUpdateEvent);
  
  // Initialize filter
  initializeFilter();
  
  debug_str("Setup complete. Monitoring detection zones...");
  printZoneInfo();
  debug_str("Configure ntp time...");
  configTzTime(Tz_zone_info, NtpServerName, "time.google.com", "pool.ntp.org");
  // Wait for time to be set
  while( time(nullptr) < 1510592825 ) 
  { // A timestamp after the function was introduced
    delay(100);
    Serial.print("*");
  }
  debug_str("\nTime synchronized");
  client.setCACert(root_cert);
  // Inform the library that we want to start in plain text mode first
  client.setInsecure(); 
  client.setPlainStart();
  if(Flg_debug)
    xmpp.setSerial(&Serial);
  xmpp.setClient(&client);
  xmpp.setConnectionData(NameSensor, PassXmpp, Resource, Server, Recipient);
  if (client.connect(Server, Port)) 
  {
    debug_str("Connected to server in plain mode");
    // Send the STARTTLS command
    if(!xmpp.startTls())
    {
      debug_str("No startTLS mode");
      return;
    }
    client.startTLS();
    if(xmpp.connect())
      debug_str("XMPP connected");
    else
      debug_str("XMPP connection failed");
  } 
  else 
    debug_str("Connection failed");
}

void SendMessage( const char *message )
{
  debug_str("SendMessage");
  const uint16_t len_massage = 400;
  char tempbuffer[200];
  char msg[len_massage];
  digitalWrite( INFO_LED, HIGH);
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
    debug_str("Failed to obtain time");
  strftime(tempbuffer, sizeof(tempbuffer), "%d.%m.%Y %H:%M:%S", &timeinfo);
  //unsigned long v = analogRead( PWR_PIN );
 
  snprintf(msg, len_massage, "%s. Время: %s. %s", NameSensor, tempbuffer, message);
  xmpp.sendMessage(Recipient, msg, "chat");
}

const String GetNameZone( unsigned int const &indx)
{
  for (int i = 0; i < NUM_ZONES; i++) 
  {
    if( zones[i].index == indx )
      return zones[i].name;
  }
  return "";
}

void loop() 
{
  static unsigned long updateRadar = 0;
  static unsigned long updateActive = 0;
  static bool flg_woke_up = false;
  int RadarActive = digitalRead(GPIO_NUM_4);
  unsigned long mil = millis();

  // Update radar readings
  radar.update();

  if(( mil - updateRadar ) > 30 )
  {
    digitalWrite( INFO_LED, LOW );
    updateRadar = mil;
    // Check zone status
    updateZones();
  }
  if( oldIndexZone != CurrentIndexZone )
  {
    debug_str("oldIndexZone", oldIndexZone);
    debug_str("CurrentIndexZone", CurrentIndexZone);
    if( oldIndexZone == -1 )
    {
      flg_woke_up = true;
      SendMessage("Разбудили, нарушение периметра охраны." );
    }
    else
      if( oldIndexZone == 5 && CurrentIndexZone == 4 && flg_woke_up == false)
      {
        String str = "Нарушение периметра охраны. Расстояние: ";
        str += GetNameZone(CurrentIndexZone);
        SendMessage(str.c_str());
      }
      else
        flg_woke_up = false;
    oldIndexZone = CurrentIndexZone;
  }
  if(( RadarActive == LOW ) && CurrentIndexZone >= 3  && (( mil - updateActive ) > WaitNotActive ))
  {
    digitalWrite( INFO_LED, LOW );
    SendMessage("Я спать." );
    debug_str("Sleeping ......");
    delay( 200 );
    wifi_off();
    esp_deep_sleep_start();
  }
  if( RadarActive == HIGH )
    updateActive = mil;
}
// Initialize the distance filter
void initializeFilter() 
{
  for (int i = 0; i < FILTER_SIZE; i++) 
    distanceFilter[i] = 0;
  filterIndex = 0;
  filterFull = false;
}
// Add a value to the filter and return the filtered result
int addToFilter(int newValue) 
{
  distanceFilter[filterIndex] = newValue;
  filterIndex = (filterIndex + 1) % FILTER_SIZE;
  
  if (!filterFull && filterIndex == 0) 
    filterFull = true;
  
  // Calculate average
  int sum = 0;
  int count = filterFull ? FILTER_SIZE : filterIndex;
  
  for (int i = 0; i < count; i++) 
    sum += distanceFilter[i];
  return count > 0 ? sum / count : 0;
}

// Update detection zones
void updateZones() 
{
  bool isDetecting = radar.isDetecting();
  int currentDistance = filteredDistance;
  
  for (int i = 0; i < NUM_ZONES; i++) 
  {
    bool wasActive = zones[i].isActive;
    // Check if object is in this zone
    if (isDetecting && currentDistance >= zones[i].minDistance && currentDistance <= zones[i].maxDistance) 
    {
      zones[i].isActive = true;
      CurrentIndexZone = zones[i].index;
      zones[i].lastDetection = millis();
    }
    else 
    {
      if( wasActive )
        oldIndexZone = zones[i].index;
      zones[i].isActive = false;
    }
  }
  if( CurrentIndexZone == 0 )
    CurrentIndexZone = NUM_ZONES;
}

// Callback functions
void onDetectionEvent(int distance) 
{
  filteredDistance = addToFilter(distance);
  
//  Serial.print("Raw: ");
//  Serial.print(distance);
//  Serial.print(" cm, Filtered: ");
//  Serial.print(filteredDistance);
//  Serial.println(" cm");
}

void onDataUpdateEvent(LD2420_Data data) {
  // This callback gets called for every data update
  // Use it for continuous monitoring or logging
  
  static unsigned long updateCounter = 0;
  updateCounter++;
  
  // Print data rate every 100 updates
  if (updateCounter % 100 == 0) {
    static unsigned long lastRateCheck = 0;
    unsigned long now = millis();
    
    if (lastRateCheck > 0)
      float rate = 100000.0 / (now - lastRateCheck); // Updates per second
    lastRateCheck = now;
  }
}

// Convert detection state to string
String stateToString(LD2420_DetectionState state) {
  switch (state) {
    case LD2420_NO_DETECTION: return "No Detection";
    case LD2420_DETECTION_ACTIVE: return "Active Detection";
    case LD2420_DETECTION_LOST: return "Detection Lost";
    default: return "Unknown";
  }
}

// Print zone information
void printZoneInfo() {
  debug_str("=== Detection Zones ===");
  for (int i = 0; i < NUM_ZONES; i++)
    debug_str(zones[i].name.c_str(), zones[i].minDistance, zones[i].maxDistance);
  debug_str("=====================");
}
