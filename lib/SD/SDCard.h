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
         * @brief Open a file for reading or writing; operation should be either w or r 
         * @param fname: filename to open for writing or reading
         * @returns File object from SD library
        */
        File open(const char *fname, const char &operation);

        /**
         * @brief Lists the contents of a directory.
         * @param fs The file system to operate on (e.g., SD card or SPIFFS).
         * @param dirname The name of the directory to list.
         * @param levels The number of levels to descend into subdirectories (default is 0).
         */
        void listDir(fs::FS &fs, const char *dirname, uint8_t levels);

        /**
         * @brief Creates a directory.
         * @param fs The file system to operate on (e.g., SD card or SPIFFS).
         * @param path The path of the directory to create.
         */
        void createDir(fs::FS &fs, const char *path);

        /**
         * @brief Removes a directory.
         * @param fs The file system to operate on (e.g., SD card or SPIFFS).
         * @param path The path of the directory to remove.
         */
        void removeDir(fs::FS &fs, const char *path);

        /**
         * @brief REads data to a file.
         * @param fs The file system to operate on (e.g., SD card or SPIFFS).
         * @param path The path of the file to read to.
         */
        void readFile(fs::FS &fs, const char *path);

        /**
         * @brief Writes data from a file.
         * @param fs The file system to operate on (e.g., SD card or SPIFFS).
         * @param path The path of the file to read from.
         * @param message string to write to file 
         */
        void writeFile(fs::FS &fs, const char *path, const char *message);

        /**
         * Tests file I/O operations.
         * @param fs The file system to operate on (e.g., SD card object).
         * @param path The path of the file to perform I/O operations on.
         */
        void testFileIO(fs::FS &fs, const char *path);

        /**
         * Performs setup tasks,  initializes the SD card.
         */
        bool setup();
};

#endif