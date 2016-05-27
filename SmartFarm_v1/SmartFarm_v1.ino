#include "DHT.h"
#include <ESP8266WiFi.h>

#define DHTPIN 12     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define DEBUG
#define DEBUG_PRINTER Serial

#ifdef  DEBUG
#define DEBUG_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
#define DEBUG_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#else
#define DEBUG_PRINT(...) {}
#define DEBUG_PRINTLN(...) {}
#endif

int SMSMOUT;
int LDROUT;

//////////////////////////////////////////
const char* ssid     = "";
const char* password = "";
//////////////////////////////////////////
int LDR1     = A0;
int LDROUT1  = 0;
//////////////////////////////////////////
int SMSM1 = 5;
int SMSMOUT1 = 0;
//////////////////////////////////////////
unsigned long Time1 = 0;
unsigned long Time2 = 0;
unsigned long Time3 = 0;
unsigned long Time4 = 0;
unsigned long previous_val = 0;
bool TimeState = false;
//////////////////////////////////////////
DHT *dht;
//////////////////////////////////////////
void connectWifi();
void reconnectWifiIfLinkDown();
void initDht(DHT **dht, uint8_t pin, uint8_t dht_type);
void readDht(DHT *dht, float *temp, float *humid);
void uploadThingsSpeak(float t, float h, int LDROUT, int SMSMOUT);
void mainloop();
//////////////////////////////////////////
void setup() 
{
    Serial.begin(115200);
    delay(10);
    pinMode(13, OUTPUT);
    pinMode(14,OUTPUT);
    pinMode(15, OUTPUT);
    digitalWrite(13, LOW);
    digitalWrite(14,LOW);
    digitalWrite(15, LOW);
    
    connectWifi();

    initDht(&dht, DHTPIN, DHTTYPE);
}

void loop() {
    static float t_ds;
    static float t_dht;
    static float h_dht;
    
    mainloop(); 
     
    readDht(dht, &t_dht, &h_dht);

    uploadThingsSpeak(t_dht, h_dht , LDROUT , SMSMOUT);

    reconnectWifiIfLinkDown();
}

void reconnectWifiIfLinkDown() 
{
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("WIFI DISCONNECTED");
        connectWifi();
    }
}

void connectWifi() 
{
    DEBUG_PRINTLN();
    DEBUG_PRINTLN();
    DEBUG_PRINT("Connecting to ");
    DEBUG_PRINTLN(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        DEBUG_PRINT(".");
    }
    
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("WiFi connected");
    digitalWrite(14,HIGH);
    delay(500);
    digitalWrite(14,LOW);
    delay(500);
    digitalWrite(14,HIGH);
    delay(500);
    digitalWrite(14,LOW);
    DEBUG_PRINTLN("IP address: ");
    DEBUG_PRINTLN(WiFi.localIP());
}

void initDht(DHT **dht, uint8_t pin, uint8_t dht_type) {
    // Connect pin 1 (on the left) of the sensor to +5V
    // NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
    // to 3.3V instead of 5V!
    // Connect pin 2 of the sensor to whatever your DHTPIN is
    // Connect pin 4 (on the right) of the sensor to GROUND
    // Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

    // Initialize DHT sensor for normal 16mhz Arduino
    // NOTE: For working with a faster chip, like an Arduino Due or Teensy, you
    // might need to increase the threshold for cycle counts considered a 1 or 0.
    // You can do this by passing a 3rd parameter for this threshold.  It's a bit
    // of fiddling to find the right value, but in general the faster the CPU the
    // higher the value.  The default for a 16mhz AVR is a value of 6.  For an
    // Arduino Due that runs at 84mhz a value of 30 works.
    // Example to initialize DHT sensor for Arduino Due:
    //DHT dht(DHTPIN, DHTTYPE, 30);

    *dht = new DHT(pin, dht_type, 30);
    (*dht)->begin();
    DEBUG_PRINTLN(F("DHTxx test!"))  ;
}

void uploadThingsSpeak(float t, float h, int LDROUT, int SMSMOUT) {
    static const char* host = "api.thingspeak.com";
    static const char* apiKey = "";

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) 
    {
        DEBUG_PRINTLN("connection failed");
        digitalWrite(14,LOW);
        return;
    }

    // We now create a URI for the request
    String url = "/update/";
    //  url += streamId;
    url += "?key=";
    url += apiKey;
    url += "&field1=";
    url += t;
    url += "&field2=";
    url += h;
    url += "&field3=";
    url += LDROUT;
    url += "&field4=";
    url += SMSMOUT;
    
    
    DEBUG_PRINT("Requesting URL: ");
    DEBUG_PRINTLN(url);

    digitalWrite(14,HIGH);
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
}

//void uploadSparkfun(float t, float h) {
//    const char* host = "";
//    const char* streamId   = "";
//    const char* privateKey = "";
//
//    // Use WiFiClient class to create TCP connections
//    WiFiClient client;
//    const int httpPort = 80;
//    if (!client.connect(host, httpPort)) {
//        DEBUG_PRINTLN("connection failed");
//        return;
//    }
//
//    // We now create a URI for the request
//    String url = "/input/";
//    url += streamId;
//    url += "?private_key=";
//    url += privateKey;
//    url += "&temp=";
//    url += t;
//    url += "&humid=";
//    url += h;
//
//    DEBUG_PRINT("Requesting URL: ");
//    DEBUG_PRINTLN(url);
//
//    // This will send the request to the server
//    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
//                 "Host: " + host + "\r\n" +
//                 "Connection: close\r\n\r\n");
//}


void readDht(DHT *dht, float *temp, float *humid) {

    if (dht == NULL) 
    {
        DEBUG_PRINTLN(F("[dht22] is not initialised. please call initDht() first."));
        return;
    }
    float h = dht->readHumidity();
    float t = dht->readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) 
    {
        DEBUG_PRINTLN("Failed to read from DHT sensor!");
        return;
    }

    // Compute heat index
    // Must send in temp in Fahrenheit!

    DEBUG_PRINT("Humidity: ");
    DEBUG_PRINT(h);
    DEBUG_PRINT(" %\t");
    DEBUG_PRINT("Temperature: ");
    DEBUG_PRINT(t);
    DEBUG_PRINT(" *C ");

    *temp = t;
    *humid = h;

}
//
void mainloop()
{
    /////////////////////////////////////////
    int LDROUT1 = analogRead(LDR1);
    /////////////////////////////////////////
    int SMSMOUT1 = digitalRead(SMSM1);
    /////////////////////////////////////////
    /////////////////////////////////////////
    LDROUT  = (LDROUT1);
    SMSMOUT = (SMSMOUT1);
    /////////////////////////////////////////
    /////////////////////////////////////////
    DEBUG_PRINT("LDR(T)= ");
    DEBUG_PRINT(LDROUT);
    DEBUG_PRINT(" ");
    DEBUG_PRINT("|");
    DEBUG_PRINT(" ");
    /////////////////////////////////////////
    DEBUG_PRINT("SMSM(T)= ");
    DEBUG_PRINT(SMSMOUT);
    DEBUG_PRINT(" ");
    DEBUG_PRINT("||");
    DEBUG_PRINT(" ");
    /////////////////////////////////////////
  if (LDROUT == 1024)
  {
    digitalWrite(13, HIGH);
    /////////////////////////////////////////
    DEBUG_PRINT("Light : ON ");
    DEBUG_PRINT(" ");
    DEBUG_PRINT("|");
    DEBUG_PRINT(" ");
  }
  if (LDROUT <= 800)
  {
    digitalWrite(13, LOW);
    /////////////////////////////////////////
    DEBUG_PRINT("Light : OFF");
    DEBUG_PRINT(" ");
    DEBUG_PRINT("|");
    DEBUG_PRINT(" ");
  }

  if (SMSMOUT == 1)
  {
    digitalWrite(15, HIGH);
    ////////////////////////////////////////
    DEBUG_PRINT("Water : ON ");
    DEBUG_PRINT(" ");
    DEBUG_PRINT("||");
    DEBUG_PRINT(" ");
  }
  else
  {
    digitalWrite(15, LOW);
    /////////////////////////////////////////
    DEBUG_PRINT("Water : OFF");
    DEBUG_PRINT(" ");
    DEBUG_PRINT("||");
    DEBUG_PRINT(" ");
  }
  /////////////////////////////////////////
  /////////////////////////////////////////
  ////////////////////////////////////////
  DEBUG_PRINTLN();
}
