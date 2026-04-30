#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <WiFiClientSecure.h>
#include "driver/rtc_io.h" // power up in sleep
#include "LD2420.h"
#include "Xmpp.h"
#include "settings.h"
#include "debug_str.h"

//blink led
#define INFO_LED GPIO_NUM_32
#define ERROR_LED GPIO_NUM_25
#define PWR_PIN GPIO_NUM_36
// Define RX and TX pins for SoftwareSerial
#define RX_PIN GPIO_NUM_16
#define TX_PIN GPIO_NUM_17
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex
#define WAKEUP_GPIO GPIO_NUM_4  // Only RTC IO are allowed - ESP32 Pin example
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

static const char *NameSection = "general";
const char* Filename = "/settings.ini";

WiFiClientSecure client;
XMPP xmpp;
// Create instances
HardwareSerial sensorSerial(2);
LD2420 radar;  // Use default constructor
Settings settings;

const uint8_t Flg_debug = 1;
const uint8_t Xmpp_debug = 0;


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
  {4, 201, 250, "Недалеко", false, 0},
  {5, 251, 300, "Далековато", false, 0},
  {6, 301, 400, "Далеко", false, 0},
  {7, 401, 801, "Очень далеко", false, 0}
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

void setup_wifi() {
  debug_str("Connecting to ", settings.ToChar(2));
  debug_str("Password", settings.ToChar(3));
  WiFi.mode(WIFI_STA); // Set ESP32 to Station mode (to connect to an AP) [1, 6]
  WiFi.begin(settings.ToChar(2), settings.ToChar(3));
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
void ReadSettings(void)
{
  debug_str("init settings...");
  settings.SetFileName((char *)Filename);
  if(settings.initSDCard() == 0)
  {
    settings.Add(1, "tz_zone_info", "SAMT-4");
    settings.Add(2, "ssid");
    settings.Add(3, "password");
    settings.Add(4, "server");
    settings.Add(5, "name_sensor");
    settings.Add(6, "ntp_server", "0.ru.pool.ntp.org");
    settings.Add(7, "recipient");
    settings.Add(8, "passxmpp");
    settings.Add(9, "port", "5222");
    settings.Add(10, "sleep", "4");
    settings.Add(11, "resource", "esp32-aaa");
    settings.Add(12, "alarm_zone", "5");
    if(settings.Read(NameSection) < 0 )
    {
      digitalWrite( ERROR_LED, HIGH );
      while (1) delay(1000);
    }
  }
  else
  {
    digitalWrite( ERROR_LED, HIGH );
    while (1) delay(1000);
  }
  if( settings.ToInt(12) > NUM_ZONES || settings.ToInt(12) < 1 )
  {
    debug_str("alarm zone number goes beyond!");  
    digitalWrite( ERROR_LED, HIGH );
    while (1) delay(1000);
  }
}
// Configuration
void setup() 
{
  pinMode( INFO_LED, OUTPUT );
  pinMode( ERROR_LED, OUTPUT );
  pinMode( WAKEUP_GPIO, INPUT_PULLDOWN );
  digitalWrite( INFO_LED, HIGH);
  #ifdef TESTMODE
  // pause usb redirect network
  uint32_t pause_net_usb = 100;  
  while(--pause_net_usb)
    delay(10);
  #endif
  // Initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  
  ReadSettings();

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
  debug_str("SoftwareSerial initialized on RX, TX", RX_PIN, TX_PIN);
  
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
    
  // Initialize filter
  initializeFilter();
  #ifdef TESTMODE
  debug_str("Setup complete. Monitoring detection zones...");
  printZoneInfo();
  #endif
  debug_str("Configure ntp time...");
  configTzTime(settings.ToChar(1), settings.ToChar(6), "time.google.com", "pool.ntp.org");
  // Wait for time to be set
  while( time(nullptr) < 1510592825 ) 
  { // A timestamp after the function was introduced
    delay(100);
    Serial.print("*");
  }
  debug_str("\nTime synchronized");

  //while (1) { delay(1000); Serial.print("|"); }

  client.setCACert(root_cert);
  // Inform the library that we want to start in plain text mode first
  client.setInsecure(); 
  client.setPlainStart();
  if(Xmpp_debug)
    xmpp.setSerial(&Serial);
  xmpp.setClient(&client);
  #ifdef TESTMODE
  debug_str(settings.ToChar(5));
  debug_str(settings.ToChar(8));
  debug_str(settings.ToChar(11));
  debug_str(settings.ToChar(4));
  debug_str(settings.ToChar(7));
  #endif
  xmpp.setConnectionData(settings.ToChar(5), settings.ToChar(8), settings.ToChar(11), settings.ToChar(4), settings.ToChar(7));
  if(client.connect(settings.ToChar(4), settings.ToInt(9)))
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
  if(client.connected())
  {
    const uint16_t len_massage = 400;
    char tempbuffer[200];
    char msg[len_massage];
    digitalWrite( INFO_LED, HIGH);
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo))
      debug_str("Failed to obtain time");
    strftime(tempbuffer, sizeof(tempbuffer), "%d.%m.%Y %H:%M:%S", &timeinfo);
    //unsigned long v = analogRead( PWR_PIN );
    snprintf(msg, len_massage, "%s. Время: %s. %s", settings.ToChar(5), tempbuffer, message);
    xmpp.sendMessage(settings.ToChar(7), msg, "chat");
  }
  else
  {
    digitalWrite( ERROR_LED, HIGH );
    debug_str("Not connected server, restart.");
    ESP.restart();
  }
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
    long isz = settings.ToInt(12);
    debug_str("oldIndexZone", oldIndexZone);
    debug_str("CurrentIndexZone", CurrentIndexZone);
    if(oldIndexZone == -1)
      SendMessage("Разбудили, тут кто-то есть." );
    if(oldIndexZone == isz && CurrentIndexZone == isz - 1)
    {
      String str = "Нарушение периметра охраны. Расстояние: ";
      str += GetNameZone(oldIndexZone);
      SendMessage(str.c_str());
    }
    oldIndexZone = CurrentIndexZone;
  }
  if(RadarActive == LOW)
  {
    if((mil - updateActive) > (settings.ToInt(10) * 60000))
    {
      digitalWrite( INFO_LED, LOW );
      SendMessage("Я спать." );
      debug_str("Sleeping ......");
      uint32_t pause = 100;  
      while(--pause)
        delay(20);
      wifi_off();
      esp_deep_sleep_start();
    }
  }
  else
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
  
  //Serial.print("Raw: ");
  //Serial.print(distance);
  //Serial.print(" cm, Filtered: ");
  //Serial.print(filteredDistance);
  //Serial.println(" cm");
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
#ifdef TESTMODE
// Print zone information
void printZoneInfo() {
  debug_str("=== Detection Zones ===");
  for (int i = 0; i < NUM_ZONES; i++)
    debug_str(zones[i].name.c_str(), zones[i].minDistance, zones[i].maxDistance);
  debug_str("=====================");
}
#endif