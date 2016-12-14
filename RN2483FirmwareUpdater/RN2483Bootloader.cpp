#include "RN2483Bootloader.h"
#include "StringLiterals.h"
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

void Sodaq_RN2483Bootloader::initBootloader(SerialType& stream)
{
    debugPrintLn("[initBootloader]");
    
    this->loraStream = &stream;
    
    #ifdef USE_DYNAMIC_BUFFER
    
    // make sure the buffers are only initialized once
    if (!isBufferInitialized) {
        this->inputBuffer = static_cast<char*>(malloc(this->inputBufferSize));
        this->receivedPayloadBuffer = static_cast<char*>(malloc(this->receivedPayloadBufferSize));
        
        isBufferInitialized = true;
    }
    
    #endif
}

void Sodaq_RN2483Bootloader::eraseFirmware()
{
    debugPrintLn("[eraseFirmware]");
    
    this->loraStream->print("sys eraseFW");
    this->loraStream->print(CRLF);
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

inline void printToLength(SerialType& stream, const uint8_t* buffer, size_t length)
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

bool Sodaq_RN2483Bootloader::applicationReset()
{
    debugPrintLn("[applicationReset]");
    return resetDevice();
}

// returns -2 in case of error, -1 if no response at all, 0 if only mainResponse, or the lenth of the secondary response otherwise
int16_t Sodaq_RN2483Bootloader::readBootloaderResponse(BootloaderRecord& mainResponse, uint8_t* secondaryResponse, uint8_t secondaryResponseSize)
{
    debugPrintLn("[readBootloaderResponse]");
    
    int len = this->loraStream->readBytes((uint8_t*)&mainResponse, sizeof(mainResponse));
    
    #ifdef DEBUG_SYMBOLS_ON
    
    if (this->diagStream) {
        printToLength((SerialType&)(*diagStream), (uint8_t*)&mainResponse, sizeof(mainResponse));
    }
    
    #endif
    
    if (len <= 0) {
        debugPrintLn("No response received at all!");
        return -1;
    }
    
    if (mainResponse.Length > secondaryResponseSize) {
        debugPrintLn("The secondary response cannot fit in the buffer!");
        this->loraStream->flush();
        
        return -2;
    }
    
    len = this->loraStream->readBytes(secondaryResponse, secondaryResponseSize);
    
    #ifdef DEBUG_SYMBOLS_ON
    
    if (this->diagStream) {
        printToLength((SerialType&)(*diagStream), (uint8_t*)secondaryResponse, len);
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
