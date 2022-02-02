#include <Arduino.h>

#include <HexDump.h>
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


static File binFile;
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
  //Serial.print("File size in myOpen: "); Serial.println(*size);

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


#define BUFF_SIZE 512
static uint8_t l_Buff[BUFF_SIZE];

void setup() {
  Serial.begin(115200);

  while (!Serial && millis() < 3000);
  Serial.println("Search for ZIP files on the SD card");
  
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


  //////////////////////////////////////////////////////////////////////////////
  int rc;
  char szComment[256], szName[256];
  unz_file_info fi;

  const char *name = "application.zip";
  if (!myfile.open(name, O_RDONLY))
  {
    Serial.println("Opening application.zip failed");
  }
  Serial.println("Openened application.zip");
  rc = rc = zip.openZIP(name, myOpen, myClose, myRead, mySeek);
  if (rc == UNZ_OK) {
    Serial.println("ZIP file found!");

    // Display the global comment and all of the filenames within
    rc = zip.getGlobalComment(szComment, sizeof(szComment));
    Serial.print("Files in this archive: ");
    zip.gotoFirstFile();
    rc = UNZ_OK;
    rc = zip.getFileInfo(&fi, szName, sizeof(szName), NULL, 0, szComment, sizeof(szComment));

    if (rc == UNZ_OK) {
      Serial.println(szName);
      Serial.print("Compressed size: "); Serial.println(fi.compressed_size, DEC);
      Serial.print("Uncompressed size: "); Serial.println(fi.uncompressed_size, DEC);
      
    }

    //ez fog kelleni majd nekem a chunkwise dologhoz!!!
    //int UNZIP::readCurrentFile(uint8_t *buffer, int iLength)

    //a mySeek-et is tudom hívni :D
    // mySeek(void *p, int32_t position, int iType)
    //         myFile,         offset,   SEEK_SET


    // Create a file in Folder1 using a path.
    if (!binFile.open("application.bin", O_WRONLY | O_CREAT))
    {
      Serial.println("Create application.bin failed");
    }
    else
    {
      Serial.println("application.bin created");
    }
    

    zip.openCurrentFile();

    // //1. blokk
    // //mySeek(void *p, int32_t position, int iType)
    // EZ ELRONT VALAMIT...: mySeek(&myfile, 0, SEEK_SET);
    //mySeek(&myfile, 0, SEEK_SET);
    //myfile.seekSet(0);
    int readSize = zip.readCurrentFile(l_Buff, BUFF_SIZE);
    Serial.print("Bytes read in buffer: "); Serial.println(readSize);
    binFile.write(l_Buff, BUFF_SIZE);

    HexDump(Serial, l_Buff, sizeof(l_Buff));

    //Serial.print("First chunk (0): "); Serial.print((char*)l_Buff[0]);

    // //2. blokk
    // mySeek(&myfile, 1024, SEEK_SET);
    // myfile.seekSet(1024);
    // readSize = zip.readCurrentFile(l_Buff, BUFF_SIZE);
    // Serial.print("Bytes read in buffer: "); Serial.println(readSize);
    // binFile.write(l_Buff, BUFF_SIZE);
    
    binFile.close();
    // Serial.print("Second chunk (1024): "); Serial.print((char*)l_Buff[0]);
    zip.closeZIP();
  }
}

// Main loop, scan for all .PNG files on the card and display them
void loop() {}

/******************************************************************************/
#endif
