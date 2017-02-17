#include "RN2483Bootloader.h"
#include "Sodaq_wdt.h"
#include <math.h>

#define DEBUG_SYMBOLS_ON

#ifdef DEBUG_SYMBOLS_ON
#define debugPrintLn(...) { if (this->diagStream) this->diagStream->println(__VA_ARGS__); }
#define debugPrint(...) { if (this->diagStream) this->diagStream->print(__VA_ARGS__); }
#warning "Debug mode is ON"
#else
#define debugPrintLn(...)
#define debugPrint(...)
#endif

Sodaq_RN2483Bootloader::Sodaq_RN2483Bootloader():
    loraStream(0),
    diagStream(0),
    inputBufferSize(RN2483_BOOTLOADER_INPUT_BUFFER_SIZE)
{

}

void Sodaq_RN2483Bootloader::initBootloader(Uart& stream)
{
    debugPrintLn("[initBootloader]");
    
    this->loraStream = &stream;
}

void Sodaq_RN2483Bootloader::eraseFirmware()
{
    debugPrintLn("[eraseFirmware]");
    
    this->loraStream->print("sys eraseFW");
    this->loraStream->print("\r\n");
    this->loraStream->flush();
}

bool Sodaq_RN2483Bootloader::getVersionInfo(BootloaderVersionInfo& versionInfo)
{
    sendCommand(GetVersionInfoCommand);
    BootloaderRecord response;
    
    if (readBootloaderResponse(response, (uint8_t*)inputBuffer, inputBufferSize) > 0) {
        memcpy((void*)&versionInfo, inputBuffer, min(sizeof(inputBuffer), sizeof(versionInfo)));
        
        return true;
    }
    
    return false;
}

bool Sodaq_RN2483Bootloader::writeFlash(uint32_t startingAddress, const uint8_t* buffer, size_t size)
{
    sendCommand(WriteFlashCommand, size, startingAddress);
    
    for (size_t i = 0; i < size; i++) {
        loraStream->write((uint8_t)buffer[i]);
    }
    
    BootloaderRecord response;
    
    if (readBootloaderResponse(response, (uint8_t*)inputBuffer, inputBufferSize) > 0) {
        if (inputBuffer[0] == 1) {
            return true;
        }
    }
    
    return false;
    
}

bool Sodaq_RN2483Bootloader::eraseFlash(uint32_t address, uint8_t blockCount)
{
    sendCommand(EraseFlashCommand, blockCount, address);
    BootloaderRecord response;
    
    if (readBootloaderResponse(response, (uint8_t*)inputBuffer, inputBufferSize) > 0) {
        if (inputBuffer[0] == 1) {
            return true;
        }
    }
    
    return false;
}

void Sodaq_RN2483Bootloader::getChecksum()
{
    // TODO
    // sendCommand(CalculateChecksumCommand, Length (from address), Address)
}

inline void printToLength(Stream& stream, const uint8_t* buffer, size_t length)
{
    for (uint8_t i = 0; i < length; i++) {
        stream.print(i, DEC);
        
        if (i < 10) {
            stream.print(" ");
        }
        
        stream.print("   ");
    }
    
    stream.println();
    
    for (uint8_t i = 0; i < length; i++) {
        stream.print("0x");
        
        if (buffer[i] < 0x10) {
            stream.print("0");
        }
        
        stream.print(buffer[i], HEX);
        stream.print(" ");
    }
    
    stream.println();
}

void Sodaq_RN2483Bootloader::bootloaderReset()
{
    debugPrintLn("[bootloaderReset]");
    sendCommand(ResetDeviceCommand);
    // no response
}

uint16_t Sodaq_RN2483Bootloader::readApplicationLn()
{
    int len = this->loraStream->readBytesUntil('\n', this->inputBuffer, this->inputBufferSize);
    
    if (len > 0) {
        this->inputBuffer[len - 1] = 0; // bytes until \n always end with \r, so get rid of it (-1)
    }
    
    return len;
}

bool Sodaq_RN2483Bootloader::expectApplicationString(const char* str, uint16_t timeout)
{
    debugPrint("[expectApplicationString] expecting ");
    debugPrint(str);
    
    unsigned long start = millis();
    
    while (millis() < start + timeout) {
        sodaq_wdt_reset();
        debugPrint(".");
        
        if (readApplicationLn() > 0) {
            debugPrint("(");
            debugPrint(this->inputBuffer);
            debugPrint(")");
            
            if (strstr(this->inputBuffer, str) != NULL) {
                debugPrintLn(" found a match!");
                
                return true;
            }
            
            return false;
        }
    }
    
    return false;
}

bool Sodaq_RN2483Bootloader::applicationReset(char* deviceResponseBuffer, size_t size)
{
    debugPrintLn("[applicationReset]");
    
    this->loraStream->print("sys reset\r\n");

    sodaq_wdt_safe_delay(100);

    if (expectApplicationString("RN")) {
        debugPrintLn("[RN Module]");
        if ((strstr(this->inputBuffer, "RN2483") != NULL) || (strstr(this->inputBuffer, "RN2903") != NULL)) {
            if (deviceResponseBuffer && (size > strlen(this->inputBuffer))) {
                debugPrintLn("Copying the response to the given buffer.");
                memcpy(deviceResponseBuffer, this->inputBuffer, min(size, inputBufferSize));
                deviceResponseBuffer[min(size, inputBufferSize)] = '\0'; // make sure the string is terminated
            }

            return true;
        }
        else {
            debugPrintLn("Unknown device type!");
            
            return false;
        }
    }
    
    return false;
}

// returns -2 in case of error, -1 if no response at all, 0 if only mainResponse, or the lenth of the secondary response otherwise
int16_t Sodaq_RN2483Bootloader::readBootloaderResponse(BootloaderRecord& mainResponse, uint8_t* secondaryResponse, uint8_t secondaryResponseSize)
{
    debugPrintLn("[readBootloaderResponse]");
    
    int len = this->loraStream->readBytes((uint8_t*)&mainResponse, sizeof(mainResponse));
    
    #ifdef DEBUG_SYMBOLS_ON
    
    if (this->diagStream) {
        printToLength((Stream&)(*diagStream), (uint8_t*)&mainResponse, sizeof(mainResponse));
    }
    
    #endif
    
    if (len <= 0) {
        debugPrintLn("No response received at all!");
        return -1;
    }

    uint8_t expectLen = 0;

    switch (mainResponse.Command) {
      case ReadFlashCommand :
      case ReadEeCommand :
      case ReadConfigurationWordsCommand :
        expectLen = mainResponse.Length;
        break;
            
      case WriteFlashCommand :
      case EraseFlashCommand :
      case WriteEeCommand :
      case WriteConfigurationWordsCommand :
        expectLen = 1;
        break;

      // Documentation is unclear, it shows a 2 byte checksum in the
      // repsonse, but also mentions a 'status'?
      case CalculateChecksumCommand :
        expectLen = 3; 
        break;

      // These will read until a time out.
      // GetVersionInfoCommand does not send the length of the 
      // response as per documentation.
      case GetVersionInfoCommand : 
      case ResetDeviceCommand :
        expectLen = secondaryResponseSize;
        break;
    }
 
    if (expectLen > secondaryResponseSize) {
        debugPrintLn("The secondary response cannot fit in the buffer!");
        this->loraStream->flush();
        
        return -2;
    }

    len = this->loraStream->readBytes(secondaryResponse, expectLen);
    
    #ifdef DEBUG_SYMBOLS_ON
    
    if (this->diagStream) {
        printToLength((Stream&)(*diagStream), (uint8_t*)secondaryResponse, len);
    }
    
    #endif
    
    return len;
}

void Sodaq_RN2483Bootloader::sendCommand(uint8_t command, uint8_t length, uint32_t address)
{
    this->loraStream->write((uint8_t)0x55); // autobaud
    
    this->loraStream->write((uint8_t)command); // command
    this->loraStream->write((uint8_t)length); // length
    this->loraStream->write((uint8_t)0x00); // unused part of length
    this->loraStream->write((uint8_t)0x55); // Key1 = 0x55
    this->loraStream->write((uint8_t)0xAA); // Key2 = 0xAA
    this->loraStream->write((uint8_t)address); // address part 0 (LSB side)
    this->loraStream->write((uint8_t)(address >> 8)); // address part 1
    this->loraStream->write((uint8_t)(address >> 16)); // address part 2
    this->loraStream->write((uint8_t)(address >> 24));  // address part 3 (MSB Side)
    
    this->loraStream->flush();
}
