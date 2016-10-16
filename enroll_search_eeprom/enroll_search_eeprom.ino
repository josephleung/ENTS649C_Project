/***************************************************
  This is an example sketch for our optical Fingerprint sensor

  Designed specifically to work with the Adafruit BMP085 Breakout
  ----> http://www.adafruit.com/products/751

  These displays use TTL Serial to communicate, 2 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/
/*
   MOD by JosephLeung
   Oct 16, 2016
*/
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//this is the address we store the id in eeprom
int address = 10;

const int buttonPin = 12;
const int ledPin =  12;

int buttonState = 0;
int id;

SoftwareSerial mySerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup() {
  pinMode(buttonPin, INPUT);

  Serial.begin(115200);
  Serial.println("Fingerprint enroll and identify demo.");

  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1);
  }

  //if the button is pushed during setup, delete all enrolled information
  if (!digitalRead(buttonPin)) {
    Serial.println("System resetting!");
    id = (int)EEPROM.read(address); //read out the id number registered (actually it's id -1)
    for (int ind = 0; ind < id; ind++) {
      while (!deleteFingerprint(id));
      delay(100);
      Serial.print("Deleted fingerprint ID: ");
      Serial.print(ind);
    }
    id = 0; //reset id
    EEPROM.update(address, id); //update the id in eeprom
    Serial.println("");
    Serial.println("System resetted! Please release the button in 3 seconds.");
    delay(3000);
  }
  Serial.println("System running.");
}

void loop() {
  //if the button is pushed, then enter the enroll procedure
  if (!digitalRead(buttonPin)) {
    id = (int)EEPROM.read(address); //read out the id number for this try
    Serial.println("");
    Serial.print("Enrolling fingerprint ID: ");
    Serial.print(id);
    //successfully enrolled
    if (!getFingerprintEnroll()) {
      slowBlink();
      id++;
      //      Serial.println("new ID: ");
      //      Serial.print(id);
      //EEPROM.update(address, id); //update the id in eeprom. THIS DOESN'T WORK!
      EEPROM.write(address, id); //update the id in eeprom. THIS WORKS!
      //      Serial.println("IDDDD updated: ");
      //      Serial.print(id);
      //      Serial.println("IDDDD read: ");
      //      Serial.print((int)EEPROM.read(address));
    }
    //failed
    else {
      fastBlink();
    }
  }

  //if the button is not pushed, then enter the search procedure
  getFingerprintIDez();
}

void fastBlink() {
  pinMode(ledPin, OUTPUT);
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, 1);
    delay(100);
    digitalWrite(ledPin, 0);
    delay(100);
  }
  pinMode(buttonPin, INPUT);
}

void slowBlink() {
  pinMode(ledPin, OUTPUT);
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, 1);
    delay(300);
    digitalWrite(ledPin, 0);
    delay(300);
  }
  pinMode(buttonPin, INPUT);
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.println("");
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
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

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.print(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
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
  Serial.print("Creating model for #");  Serial.print(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.print(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    return p; //added by JosephLeung
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }
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
  Serial.println("");
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.print(finger.confidence);
  slowBlink();
  return finger.fingerID;
}

uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }
}
