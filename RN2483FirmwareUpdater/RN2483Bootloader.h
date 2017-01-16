// RN2483Bootloader.h

#ifndef RN2483BOOTLOADER_H_
#define RN2483BOOTLOADER_H_

#include "Arduino.h"

// NOTE: Both the Bootloader and the ARM M0-based arduinos are little endian

#define RN2483_BOOTLOADER_INPUT_BUFFER_SIZE 128
#define RN2483_BOOTLOADER_DEFAULT_TIMEOUT 120

struct BootloaderRecord {
    uint8_t AutoBaudChar;
    uint8_t Command;
    uint8_t Length;
    uint8_t Dummy1;
    uint8_t Key1;
    uint8_t Key2;
    
    uint8_t Address1;
    uint8_t Address2;
    uint8_t Address3;
    uint8_t Address4;
};

enum Command {
    GetVersionInfoCommand = 0x00,
    ReadFlashCommand = 0x01,
    WriteFlashCommand = 0x02,
    EraseFlashCommand = 0x03,
    ReadEeCommand = 0x04,
    WriteEeCommand = 0x05,
    ReadConfigurationWordsCommand = 0x06,
    WriteConfigurationWordsCommand = 0x07,
    CalculateChecksumCommand = 0x08,
    ResetDeviceCommand = 0x09
};

struct BootloaderVersionInfo {
    union {
        uint16_t BootloaderVersion;
        uint8_t BootloaderVersionLowByte;
        uint8_t BootloaderVersionHighByte;
    };
    uint8_t Reserved1;
    uint8_t Reserved2;
    uint8_t Reserved3;
    uint8_t Reserved4;
    union {
        uint16_t DeviceId;
        uint8_t DeviceIdLowByte;
        uint8_t DeviceIdHighByte;
    };
    uint8_t Reserved5;
    uint8_t Reserved6;
    uint8_t EraseRowSize;
    uint8_t WriteLatchSize;
    uint8_t UserId1;
    uint8_t UserId2;
    uint8_t UserId3;
    uint8_t UserId4;
};

class Sodaq_RN2483Bootloader
{
    public:
        Sodaq_RN2483Bootloader();
        
        uint32_t getDefaultBootloaderBaudRate() { return 38400; };
        
        uint32_t getDefaultApplicationBaudRate() { return 57600; };
        
        void initBootloader(Uart& stream);
        
        void setDiag(Stream& stream) { diagStream = &stream; };
        
        void eraseFirmware();
        
        bool getVersionInfo(BootloaderVersionInfo& versionInfo);
        
        bool writeFlash(uint32_t startingAddress, const uint8_t* buffer, size_t size);
        
        bool eraseFlash(uint32_t address, uint8_t blockCount);
        
        void getChecksum();
        
        void bootloaderReset();
        
        bool applicationReset(char* deviceResponseBuffer, size_t size);

        bool applicationReset() { return applicationReset(0, 0); };
    private:
        Uart* loraStream;
        
        Stream* diagStream;
        
        uint16_t inputBufferSize;
        
        char inputBuffer[RN2483_BOOTLOADER_INPUT_BUFFER_SIZE];
        
        uint16_t readApplicationLn();
        
        bool expectApplicationString(const char* str, uint16_t timeout = RN2483_BOOTLOADER_DEFAULT_TIMEOUT);
        
        int16_t readBootloaderResponse(BootloaderRecord& mainResponse, uint8_t* secondaryResponse, uint8_t secondaryResponseSize);
        
        void sendCommand(uint8_t command, uint8_t length = 0, uint32_t address = 0);
};

#endif