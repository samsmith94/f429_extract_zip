#include <Arduino.h>

//#define COMPILE_ONLY_SD
#ifdef COMPILE_ONLY_SD
/******************************************************************************/

#include "SdFat.h"
#include "sdios.h"

SPIClass SPI_4(PE6, PE5, PE2); // MOSI, MISO. SCLK

const uint8_t SD_CS_PIN = PE4;

#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(4), &SPI_4)

/*
SdFat sd;
File file;
File root;

SdFat32 sd;
File32 file;
File32 root;

SdExFat sd;
ExFile file;
ExFile root;

SdFs sd;
FsFile file;
FsFile root;
*/

// eredetileg ez volt itt:
// SdFat32 sd;
// File32 file;
// File32 root;

// de ezel is teljesen jó:
SdFat sd;
File file;
File root;

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
/******************************************************************************/
/******************************************************************************/

#else

#include <unzipLIB.h>
//#include <SD.h>

#include "SdFat.h"
#include "sdios.h"

SPIClass SPI_4(PE6, PE5, PE2); // MOSI, MISO. SCLK

const uint8_t SD_CS_PIN = PE4;

#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(4), &SPI_4)

UNZIP zip; // statically allocate the UNZIP structure (41K)

SdFat sd;
//File file;
File root;

static File myfile;

static File myfileAnother;

//
// Callback functions needed by the unzipLIB to access a file system
// The library has built-in code for memory-to-memory transfers, but needs
// these callback functions to allow using other storage media
//
void * myOpen(const char *filename, int32_t *size) {
  //TODO: EZEKET KELL KICSERÉLNI!!!
  // myfile = SD.open(filename);
  // *size = myfile.size();
  root.open("/");
  myfileAnother.open(filename);
  *size = (uint32_t)myfileAnother.fileSize();
  Serial.print("File size in myOpen: "); Serial.println(*size);

  return (void *)&myfileAnother;
}

void myClose(void *p) {
  ZIPFILE *pzf = (ZIPFILE *)p;
  File *f = (File *)pzf->fHandle;
  if (f) f->close();
}

int32_t myRead(void *p, uint8_t *buffer, int32_t length) {
  ZIPFILE *pzf = (ZIPFILE *)p;
  File *f = (File *)pzf->fHandle;
  return f->read(buffer, length);
}

int32_t mySeek(void *p, int32_t position, int iType) {
  ZIPFILE *pzf = (ZIPFILE *)p;
  File *f = (File *)pzf->fHandle;
  if (iType == SEEK_SET)
    return f->seek(position);
  else if (iType == SEEK_END) {
    return f->seek(position + pzf->iSize); 
  } else { // SEEK_CUR
    long l = f->position();
    return f->seek(l + position);
  }
}

void setup() {
  Serial.begin(115200);

  while (!Serial && millis() < 3000);
  Serial.println("Search for ZIP files on the SD card");

  // while (!SD.begin(4)) {
  //   Serial.println("Unable to access SD Card");
  //   delay(1000);
  // }
  
  if (!sd.begin(SD_CONFIG))
  {
    sd.initErrorHalt(&Serial);
    Serial.println("Unable to access SD Card");
  }

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
  if (!myfile.open("Folder1/file1.txt", O_WRONLY | O_CREAT))
  {
    Serial.println("Create Folder1/file1.txt failed");
  }
  Serial.println("Created Folder1/file1.txt");
  myfile.println("Hello 1, 2, 3");
  Serial.println("Hello 1, 2, 3 written to Folder1/file1.txt");

  Serial.print("Size of Folder1/file1.txt: ");
  Serial.println(myfile.fileSize());

  myfile.close();
  Serial.println("Folder1/file1.txt closed");
  Serial.println("****************************************");


  /**********************/
  //EZ SAJNOS MÉG NEM MŰKÖDIK...
  int rc;
  char szComment[256], szName[256];
  unz_file_info fi;

  const char *name = "application.zip";
  if (!myfile.open(name, O_RDONLY))
  {
    Serial.println("Opening application.zip failed");
  }
  Serial.println("Openend application.zip");
  rc = rc = zip.openZIP(name, myOpen, myClose, myRead, mySeek);
  if (rc == UNZ_OK) {
    Serial.print("ZIP file found");

    // Display the global comment and all of the filenames within
    rc = zip.getGlobalComment(szComment, sizeof(szComment));
    Serial.print("Global comment: "); Serial.println(szComment); Serial.println("Files in this archive:");
    zip.gotoFirstFile();
    rc = UNZ_OK;
    rc = zip.getFileInfo(&fi, szName, sizeof(szName), NULL, 0, szComment, sizeof(szComment));

    if (rc == UNZ_OK) {
      Serial.print(szName); Serial.print(" - "); Serial.print(fi.compressed_size, DEC); Serial.print("/"); Serial.println(fi.uncompressed_size, DEC);
    }
    zip.closeZIP();
  }
}

// Main loop, scan for all .PNG files on the card and display them
void loop() {
  // int rc, filecount = 0;
  // char szComment[256], szName[256];
  // unz_file_info fi;
  
  // File dir = SD.open("/");
  // while (true) {
  //   File entry = dir.openNextFile();
  //   if (!entry) break;

  //   if (entry.isDirectory() == false) {
  //     const char *name = entry.name();
  //     const int len = strlen(name);

  //     if (len > 3 && strcmp(name + len - 3, "ZIP") == 0) {
  //       Serial.print("File: "); Serial.println(name);
  //       rc = rc = zip.openZIP(name, myOpen, myClose, myRead, mySeek);

  //       if (rc == UNZ_OK) {
  //         Serial.print("found zip file: "); Serial.println(name);

  //         // Display the global comment and all of the filenames within
  //         rc = zip.getGlobalComment(szComment, sizeof(szComment));
  //         Serial.print("Global comment: "); Serial.println(szComment); Serial.println("Files in this archive:");
  //         zip.gotoFirstFile();
  //         rc = UNZ_OK;

  //         while (rc == UNZ_OK) { // Display all files contained in the archive
  //           rc = zip.getFileInfo(&fi, szName, sizeof(szName), NULL, 0, szComment, sizeof(szComment));

  //           if (rc == UNZ_OK) {
  //             Serial.print(szName); Serial.print(" - "); Serial.print(fi.compressed_size, DEC); Serial.print("/"); Serial.println(fi.uncompressed_size, DEC);
  //           }
  //           rc = zip.gotoNextFile();
  //         } // while more files...
  //         zip.closeZIP();
  //       }
  //       filecount = filecount + 1;
  //     }
  //   }
  //   entry.close();
  // }
  // if (filecount == 0) {
  //   Serial.println("No .ZIP files found");
  // }
  // delay(2000);
}

/******************************************************************************/
#endif
