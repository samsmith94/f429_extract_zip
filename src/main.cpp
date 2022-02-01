#include <Arduino.h>
#include <STM32RTC.h>

#include "SdFat.h"
#include "sdios.h"

#include <Servo.h>
Servo servo1;
Servo servo2;

#define RED_LED PB14
#define GREEN_LED PB0
#define BLUE_LED PB7

#define RELAY_1 PE1
#define RELAY_2 PG12

#define ACTIVE_LED BLUE_LED

/* Get the rtc object */
STM32RTC &rtc = STM32RTC::getInstance();

/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 0;
const byte hours = 16;

/* Change these values to set the current initial date */
/* Monday 15th June 2015 */
const byte weekDay = 1;
const byte day = 15;
const byte month = 6;
const byte year = 15;




//            MOSI  MISO  SCLK
SPIClass SPI_4(PE6, PE5, PE2);
//PE4
#define SDCARD_SS_PIN PE4

/*
  Set DISABLE_CS_PIN to disable a second SPI device.
  For example, with the Ethernet shield, set DISABLE_CS_PIN
  to 10 to disable the Ethernet controller.
*/
const int8_t DISABLE_CS_PIN = -1;
/*
  Change the value of SD_CS_PIN if you are using SPI
  and your hardware does not use the default value, SS.
  Common values are:
  Arduino Ethernet shield: pin 4
  Sparkfun SD shield: pin 8
  Adafruit SD shields and modules: pin 10
*/
// SDCARD_SS_PIN is defined for the built-in SD on some boards.
#ifndef SDCARD_SS_PIN
const uint8_t SD_CS_PIN = SS;
#else  // SDCARD_SS_PIN
const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif  // SDCARD_SS_PIN

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(4), &SPI_4)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(16))
#endif  // HAS_SDIO_CLASS

//------------------------------------------------------------------------------
SdFs sd;
cid_t m_cid;
csd_t m_csd;
uint32_t m_eraseSize;
uint32_t m_ocr;
static ArduinoOutStream cout(Serial);
//------------------------------------------------------------------------------
bool cidDmp() {
  cout << F("\nManufacturer ID: ");
  cout << uppercase << showbase << hex << int(m_cid.mid) << dec << endl;
  cout << F("OEM ID: ") << m_cid.oid[0] << m_cid.oid[1] << endl;
  cout << F("Product: ");
  for (uint8_t i = 0; i < 5; i++) {
    cout << m_cid.pnm[i];
  }
  cout << F("\nVersion: ");
  cout << int(m_cid.prv_n) << '.' << int(m_cid.prv_m) << endl;
  cout << F("Serial number: ") << hex << m_cid.psn << dec << endl;
  cout << F("Manufacturing date: ");
  cout << int(m_cid.mdt_month) << '/';
  cout << (2000 + 16*m_cid.mdt_year_high + m_cid.mdt_year_low) << endl;
  cout << endl;
  return true;
}
//------------------------------------------------------------------------------
void clearSerialInput() {
  uint32_t m = micros();
  do {
    if (Serial.read() >= 0) {
      m = micros();
    }
  } while (micros() - m < 10000);
}
//------------------------------------------------------------------------------
bool csdDmp() {
  bool eraseSingleBlock;
  if (m_csd.v1.csd_ver == 0) {
    eraseSingleBlock = m_csd.v1.erase_blk_en;
    m_eraseSize = (m_csd.v1.sector_size_high << 1) | m_csd.v1.sector_size_low;
  } else if (m_csd.v2.csd_ver == 1) {
    eraseSingleBlock = m_csd.v2.erase_blk_en;
    m_eraseSize = (m_csd.v2.sector_size_high << 1) | m_csd.v2.sector_size_low;
  } else {
    cout << F("m_csd version error\n");
    return false;
  }
  m_eraseSize++;
  cout << F("cardSize: ") << 0.000512 * sdCardCapacity(&m_csd);
  cout << F(" MB (MB = 1,000,000 bytes)\n");

  cout << F("flashEraseSize: ") << int(m_eraseSize) << F(" blocks\n");
  cout << F("eraseSingleBlock: ");
  if (eraseSingleBlock) {
    cout << F("true\n");
  } else {
    cout << F("false\n");
  }
  return true;
}
//------------------------------------------------------------------------------
void errorPrint() {
  if (sd.sdErrorCode()) {
    cout << F("SD errorCode: ") << hex << showbase;
    printSdErrorSymbol(&Serial, sd.sdErrorCode());
    cout << F(" = ") << int(sd.sdErrorCode()) << endl;
    cout << F("SD errorData = ") << int(sd.sdErrorData()) << endl;
  }
}
//------------------------------------------------------------------------------
bool mbrDmp() {
  MbrSector_t mbr;
  bool valid = true;
  if (!sd.card()->readSector(0, (uint8_t*)&mbr)) {
    cout << F("\nread MBR failed.\n");
    errorPrint();
    return false;
  }
  cout << F("\nSD Partition Table\n");
  cout << F("part,boot,bgnCHS[3],type,endCHS[3],start,length\n");
  for (uint8_t ip = 1; ip < 5; ip++) {
    MbrPart_t *pt = &mbr.part[ip - 1];
    if ((pt->boot != 0 && pt->boot != 0X80) ||
        getLe32(pt->relativeSectors) > sdCardCapacity(&m_csd)) {
      valid = false;
    }
    cout << int(ip) << ',' << uppercase << showbase << hex;
    cout << int(pt->boot) << ',';
    for (int i = 0; i < 3; i++ ) {
      cout << int(pt->beginCHS[i]) << ',';
    }
    cout << int(pt->type) << ',';
    for (int i = 0; i < 3; i++ ) {
      cout << int(pt->endCHS[i]) << ',';
    }
    cout << dec << getLe32(pt->relativeSectors) << ',';
    cout << getLe32(pt->totalSectors) << endl;
  }
  if (!valid) {
    cout << F("\nMBR not valid, assuming Super Floppy format.\n");
  }
  return true;
}
//------------------------------------------------------------------------------
void dmpVol() {
  cout << F("\nScanning FAT, please wait.\n");
  uint32_t freeClusterCount = sd.freeClusterCount();
  if (sd.fatType() <= 32) {
    cout << F("\nVolume is FAT") << int(sd.fatType()) << endl;
  } else {
    cout << F("\nVolume is exFAT\n");
  }
  cout << F("sectorsPerCluster: ") << sd.sectorsPerCluster() << endl;
  cout << F("clusterCount:      ") << sd.clusterCount() << endl;
  cout << F("freeClusterCount:  ") << freeClusterCount << endl;
  cout << F("fatStartSector:    ") << sd.fatStartSector() << endl;
  cout << F("dataStartSector:   ") << sd.dataStartSector() << endl;
  if (sd.dataStartSector() % m_eraseSize) {
    cout << F("Data area is not aligned on flash erase boundary!\n");
    cout << F("Download and use formatter from www.sdcard.org!\n");
  }
}
//------------------------------------------------------------------------------
void printCardType() {

  cout << F("\nCard type: ");

  switch (sd.card()->type()) {
    case SD_CARD_TYPE_SD1:
      cout << F("SD1\n");
      break;

    case SD_CARD_TYPE_SD2:
      cout << F("SD2\n");
      break;

    case SD_CARD_TYPE_SDHC:
      if (sdCardCapacity(&m_csd) < 70000000) {
        cout << F("SDHC\n");
      } else {
        cout << F("SDXC\n");
      }
      break;

    default:
      cout << F("Unknown\n");
  }
}
//------------------------------------------------------------------------------
void printConfig(SdSpiConfig config) {
  if (DISABLE_CS_PIN < 0) {
    cout << F(
           "\nAssuming the SD is the only SPI device.\n"
           "Edit DISABLE_CS_PIN to disable an SPI device.\n");
  } else {
    cout << F("\nDisabling SPI device on pin ");
    cout << int(DISABLE_CS_PIN) << endl;
    pinMode(DISABLE_CS_PIN, OUTPUT);
    digitalWrite(DISABLE_CS_PIN, HIGH);
  }
  cout << F("\nAssuming the SD chip select pin is: ") << int(config.csPin);
  cout << F("\nEdit SD_CS_PIN to change the SD chip select pin.\n");
}
//------------------------------------------------------------------------------
void printConfig(SdioConfig config) {
  (void)config;
  cout << F("Assuming an SDIO interface.\n");
}
//-----------------------------------------------------------------------------

//HardwareSerial Serial1(PG9, PG14);

void button_ISR()
{
  Serial.println("Reset");
  HAL_NVIC_SystemReset();
}

int buttonPin = PC13;


//SdFat32 sd;
File32 file;
File32 root;


void setup()
{
  //Serial1.begin(115200);

  Serial.begin(115200);

  /*
  servo1.attach(PC9);
  servo2.attach(PC6);
  while(1) {
    for (int i = 0; i < 180; i++) {
      servo1.write(i);
      servo2.write(i);
      delay(3);
    }
    for (int i = 180; i > 0; i--) {
      servo1.write(i);
      servo2.write(i);
      delay(3);
    }
  }
  */

  // pinMode(ACTIVE_LED, OUTPUT);
  // pinMode(RELAY_1, OUTPUT);
  // pinMode(RELAY_2, OUTPUT);

  // Select RTC clock source: LSI_CLOCK, LSE_CLOCK or HSE_CLOCK.
  // By default the LSI is selected as source.
  //rtc.setClockSource(STM32RTC::LSE_CLOCK);

  rtc.begin(); // initialize RTC 24H format

  // // Set the time
  // rtc.setHours(hours);
  // rtc.setMinutes(minutes);
  // rtc.setSeconds(seconds);

  // // Set the date
  // rtc.setWeekDay(weekDay);
  // rtc.setDay(day);
  // rtc.setMonth(month);
  // rtc.setYear(year);

  // // you can use also
  // rtc.setTime(hours, minutes, seconds);
  // rtc.setDate(weekDay, day, month, year);

  // pinMode(buttonPin, INPUT);
  // attachInterrupt(digitalPinToInterrupt(buttonPin), button_ISR, FALLING);

  
  while (!Serial) {
    yield();
  }
  cout << F("SdFat version: ") << SD_FAT_VERSION_STR << endl;
  printConfig(SD_CONFIG);
  
}


void loop()
{
  //Serial1.println("Hello\r\n");
  //Serial.println("Hello");

  // // Print date...
  // Serial.printf("%02d/%02d/%02d ", rtc.getDay(), rtc.getMonth(), rtc.getYear());
  // // ...and time
  // Serial.printf("%02d:%02d:%02d.%03d\n", rtc.getHours(), rtc.getMinutes(), rtc.getSeconds(), rtc.getSubSeconds());
  // delay(1000);

  // digitalWrite(ACTIVE_LED, HIGH);
  // digitalWrite(RELAY_1, HIGH);
  // digitalWrite(RELAY_2, HIGH);
  // delay(1000);
  // digitalWrite(ACTIVE_LED, LOW);
  // digitalWrite(RELAY_1, LOW);
  // digitalWrite(RELAY_2, LOW);
  // delay(1000);


  
  // Read any existing Serial data.
  clearSerialInput();

  // F stores strings in flash to save RAM
  cout << F("\ntype any character to start\n");
  while (!Serial.available()) {
    yield();
  }
  uint32_t t = millis();
  if (!sd.cardBegin(SD_CONFIG)) {
    cout << F(
           "\nSD initialization failed.\n"
           "Do not reformat the card!\n"
           "Is the card correctly inserted?\n"
           "Is there a wiring/soldering problem?\n");
    if (isSpi(SD_CONFIG)) {
      cout << F(
           "Is SD_CS_PIN set to the correct value?\n"
           "Does another SPI device need to be disabled?\n"
           );
    }
    errorPrint();
    return;
  }
  t = millis() - t;
  cout << F("init time: ") << t << " ms" << endl;

  if (!sd.card()->readCID(&m_cid) ||
      !sd.card()->readCSD(&m_csd) ||
      !sd.card()->readOCR(&m_ocr)) {
    cout << F("readInfo failed\n");
    errorPrint();
    return;
  }
  printCardType();
  cidDmp();
  csdDmp();
  cout << F("\nOCR: ") << uppercase << showbase;
  cout << hex << m_ocr << dec << endl;
  if (!mbrDmp()) {
    return;
  }
  if (!sd.volumeBegin()) {
    cout << F("\nvolumeBegin failed. Is the card formatted?\n");
    errorPrint();
    return;
  }
  dmpVol();
  

  /***************/
  // Initialize the SD card.
  if (!sd.begin(SD_CONFIG)) {
    sd.initErrorHalt(&Serial);
  }

  int rootFileCount = 0;
  if (!root.open("/")) {
    Serial.println("open root");
  }
  // Create a new folder.
  if (!sd.mkdir("Folder1")) {
    Serial.println("Create Folder1 failed");
  }

  // Create a file in Folder1 using a path.
  if (!file.open("Folder1/file1.txt", O_WRONLY | O_CREAT)) {
    Serial.println("create Folder1/file1.txt failed");
  }

  file.println("Hello 1, 2, 3");
  file.close();
  cout << F("Created Folder1/file1.txt\n");


}