/***************************************************
  Adafruit MQTT Library Ethernet Example

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Alec Moore
  Derived from the code written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
//#include <EtherCard.h>

#include <UIPEthernet.h>

//#include <SPI.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>
#include <Adafruit_SleepyDog.h>

//#include <Ethernet.h>
//#include <EthernetClient.h>
//#include <Dns.h>
//#include <Dhcp.h>

/************************* Ethernet Client Setup *****************************/
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

//Uncomment the following, and set to a valid ip if you don't have dhcp available.
//IPAddress iotIP (192, 168, 0, 42);
//Uncomment the following, and set to your preference if you don't have automatic dns.
//IPAddress dnsIP (8, 8, 8, 8);
//If you uncommented either of the above lines, make sure to change "Ethernet.begin(mac)" to "Ethernet.begin(mac, iotIP)" or "Ethernet.begin(mac, iotIP, dnsIP)"


/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "zhaoqiliang"
#define AIO_KEY         "c88f0843b821462ab16736f48d366407"


/************ Global State (you don't need to change this!) ******************/

//Set up the ethernet client
EthernetClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// You don't need to change anything below this line!
#define halt(s) { Serial.println(F( s )); while(1);  }


/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
// Fingerprint sensor feed
//const char FINGERPRINT_FEED[] PROGMEM = AIO_USERNAME "/feeds/fingerprint";
Adafruit_MQTT_Publish fingerprint = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/fingerprint");

// Setup a feed called 'onoff' for subscribing to changes.
//Adafruit_MQTT_Subscribe photocell = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/photocell");

/*************************** Sketch Code ************************************/
// Function to get fingerprint
int getFingerprintIDez();

// Init Software Serial
SoftwareSerial mySerial(3, 4);

// Fingerprint sensor instance
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Lock state
bool lockState = false;

uint32_t state;

// Your stored finger ID
int fingerID = 0;

// Counters
int activationCounter = 0;
int lastActivation = 0;
int activationTime = 10 * 1000;

void setup() {
  Serial.begin(115200);
  finger.begin(57600);
  Serial.println(F("Role: Fingureprint part."));

  // Check if sensor is present
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1);
  }
  Serial.println("Waiting for valid finger...");

  // Initialise the Client
  Serial.print(F("\nInit the Client..."));
  Ethernet.begin(mac);
  delay(1000); //give the ethernet a second to initialize


  //mqtt.subscribe(&photocell);
}

void loop() {
  // Make sure to reset watchdog every loop iteration!
  Watchdog.reset();

  MQTT_connect();

  // Get fingerprint # ID
  int fingerprintID = getFingerprintIDez();

  // Activation ?
  if (fingerprintID == fingerID && lockState == false) {
    Serial.println(F("Access granted!"));
    lockState = true;
    state = 1;
    if (! fingerprint.publish(state)) {
      Serial.println(F("Activate state send failed"));
    } else {
      Serial.println(F("Activate state send OK!"));
    }
    lastActivation = millis();
  }

  // Set back to 0
  if ((activationCounter - lastActivation > activationTime) && lockState == true) {
    lockState = false;
    state = 0;
    if (! fingerprint.publish(state)) {
      Serial.println(F("Deactivate state send Failed"));
    } else {
      Serial.println(F("Deactivate state send OK!"));
    }
  }

  activationCounter = millis();
  delay(50);

  // ping the server to keep the mqtt connection alive
  if (! mqtt.ping()) {
    mqtt.disconnect();
  }

}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}
