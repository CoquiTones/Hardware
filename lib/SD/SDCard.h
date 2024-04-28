#ifndef SDCard
#define SDCard

#include <SD.h>
#include <FS.h>
#include <SPI.h>
#include <../shared/config.h>


class SDCARD {
    public:
        SDCARD() {};
        /**
         * 
         * @
        */
        File open(const char *fname);
        void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
        void createDir(fs::FS &fs, const char *path);
        void removeDir(fs::FS &fs, const char *path);
        void writeFile(fs::FS &fs, const char *path, const char *message);
        void readFile(fs::FS &fs, const char *path);
        void testFileIO(fs::FS &fs, const char *path);
        bool setup();
};

#endif