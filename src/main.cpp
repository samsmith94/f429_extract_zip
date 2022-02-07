#include <Arduino.h>
#include <HexDump.h>
#include <unzipLIB.h>
#include "SdFat.h"
#include "sdios.h"

#define BUFF_SIZE 1024
static uint8_t l_Buff[BUFF_SIZE];

SPIClass SPI_4(PE6, PE5, PE2); // MOSI, MISO. SCLK
const uint8_t SD_CS_PIN = PE4;
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(4), &SPI_4)

UNZIP zip; // statically allocate the UNZIP structure (41K)
SdFat sd;
File root;

static File myfile;
static File myfileAnother;
static File binFile;

void * myOpen(const char *filename, int32_t *size) {
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

  // Create a application.bin file to write RAM to SD card
  if (!binFile.open("application.bin", O_WRONLY | O_CREAT))
  {
    Serial.println("Create application.bin failed");
  }
  else
  {
    Serial.println("application.bin created");
  }
  
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

    // IF THIS LINE IS COMPILED, IT IS NOT WORKING, NOTHING IS PRINTED ON SERIAL CONSOLE
    zip.locateFile("application.bin");
    zip.openCurrentFile();

    int counter = 0;
    int number_of_chunks = (fi.uncompressed_size / BUFF_SIZE);
    int rc, i;
    rc = 1;
    i = 0;
    while (rc > 0) {
        if (counter == number_of_chunks) {
          break;
        }
        rc = zip.readCurrentFile(l_Buff, BUFF_SIZE);
        binFile.write(l_Buff, BUFF_SIZE);
        counter++;
        if (rc >= 0) {
            i += rc;
        } else {
            Serial.println("Error reading from file");
            break;
        }
    }
    Serial.print("Total bytes read = ");
    Serial.println(i);

    // First block
    // int readSize = zip.readCurrentFile(l_Buff, BUFF_SIZE);
    // Serial.print("Bytes read into buffer: "); Serial.println(readSize);
    // binFile.write(l_Buff, BUFF_SIZE);
    // HexDump(Serial, l_Buff, sizeof(l_Buff));

    // IF THIS 4 LINE IS COMPILED, IT IS NOT WORKING, NOTHING IS PRINTED ON SERIAL CONSOLE
    // // Second block
    // readSize = zip.readCurrentFile(l_Buff, BUFF_SIZE);
    // Serial.print("Bytes read into buffer: "); Serial.println(readSize);
    // binFile.write(l_Buff, BUFF_SIZE);
    // HexDump(Serial, l_Buff, sizeof(l_Buff));
    
    zip.closeCurrentFile();
    zip.closeZIP();

    binFile.close();
    Serial.println("Now you can remove SD card.");
  }
}

// Main loop, scan for all .PNG files on the card and display them
void loop() {}
