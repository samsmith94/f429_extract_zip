#include <Arduino.h>

#define COMPILE_UNZIP
#ifdef COMPILE_UNZIP
/******************************************************************************/

#include "SdFat.h"
#include "sdios.h"

SPIClass SPI_4(PE6, PE5, PE2); // MOSI, MISO. SCLK

const uint8_t SD_CS_PIN = PE4;

#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(4), &SPI_4)

SdFat32 sd;
File32 file;
File32 root;

void setup()
{
  Serial.begin(115200);

  while (!Serial)
  {
    yield();
  }

  //////////////////////////////////////////////////////////////////////////////
  // Initialize the SD card.
  if (!sd.begin(SD_CONFIG))
  {
    sd.initErrorHalt(&Serial);
  }

  int rootFileCount = 0;
  if (!root.open("/"))
  {
    Serial.println("Open root failed");
  }

  //először el kell távolítani a már létező fájlokat!!!, emiatt nem működött először
  if (sd.exists("Folder1"))
  {
    if (sd.exists("Folder1/file1.txt"))
    {
      Serial.println("Folder1/file1.txt is already existing");

      // Change volume working directory to Folder1.
      if (!sd.chdir("Folder1"))
      {
        Serial.println("chdir failed for Folder1.");
      }
      Serial.println("chdir to Folder1");

      // Remove files from current directory.
      if (!sd.remove("file1.txt"))
      {
        Serial.println("remove failed");
      }
      Serial.println("file1.txt removed.");

      // Change current directory to root.
      if (!sd.chdir())
      {
        Serial.println("chdir to root failed.");
      }

      // Remove Folder1.
      if (!sd.rmdir("Folder1"))
      {
        Serial.println("rmdir for Folder1 failed");
      }
      Serial.println("Folder1 removed.");
    }
  }

  //////////////////////////////////////////////////////////////////////////////

  // Create a new folder.
  if (!sd.mkdir("Folder1"))
  {
    Serial.println("Create Folder1 failed");
  }

  if (!sd.mkdir("Folder2"))
  {
    Serial.println("Create Folder2 failed");
  }

  // Create a file in Folder1 using a path.
  if (!file.open("Folder1/file1.txt", O_WRONLY | O_CREAT))
  {
    Serial.println("Create Folder1/file1.txt failed");
  }
  Serial.println("Created Folder1/file1.txt");
  file.println("Hello 1, 2, 3");
  Serial.println("Hello 1, 2, 3 written to Folder1/file1.txt");

  Serial.print("Size of Folder1/file1.txt: ");
  Serial.println(file.fileSize());

  file.close();
  Serial.println("Folder1/file1.txt closed");
  Serial.println("****************************************");
}

void loop() {}

/******************************************************************************/
#else

void setup() {}
void loop() {}

/******************************************************************************/
#endif
