#include "RN2483Bootloader.h"
#include "IntelHexParser.h"
#include "Utils.h"
#include "Sodaq_wdt.h"

#include "HexFileImage.h"

// TODO ask user if should erase blocks
// TODO investigate larger writing blocks for higher speed

#define DEBUG_SYMBOLS_ON


#if defined (ARDUINO_ARCH_ESP32)
#define CONSOLE_STREAM Serial
#define DEBUG_STREAM Serial

#elif defined (ARDUINO_SAMD_ZERO)
#define CONSOLE_STREAM SerialUSB
#define DEBUG_STREAM SerialUSB

#else
#define CONSOLE_STREAM SERIAL_PORT_MONITOR
#define DEBUG_STREAM SERIAL_PORT_MONITOR
#endif

#if defined(ARDUINO_SODAQ_EXPLORER)
#define LORA_STREAM Serial2

#elif defined(ARDUINO_SODAQ_AUTONOMO) || defined(ARDUINO_SODAQ_ONE) || defined(ARDUINO_SODAQ_ONE_BETA)
#define LORA_STREAM Serial1

#elif defined(ARDUINO_SAMD_ZERO) 
#define LORA_STREAM Serial
#define LORA_RESET 6

#elif defined(ARDUINO_ARCH_ESP32) 
// Serial1 on ESP32 default Serial1 pins are for SPI Flash
// can't be used as is without remap, here use Serial2 GPIO16/17
HardwareSerial Serial1(1); // using Hardware UART Serial1 
#define LORA_STREAM Serial1
#define LORA_RESET 4

#endif

#ifdef DEBUG_SYMBOLS_ON
#define debugPrintln(...) { if (isDebugOn) DEBUG_STREAM.println(__VA_ARGS__); }
#define debugPrint(...) { if (isDebugOn) DEBUG_STREAM.print(__VA_ARGS__); }
#warning "Debug mode is ON"
#else
#define debugPrintLn(...)
#define debugPrint(...)
#endif

#define consolePrintln(...) { CONSOLE_STREAM.println(__VA_ARGS__); }
#define consolePrint(...) { CONSOLE_STREAM.print(__VA_ARGS__); }

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define HexFileImageName STR(HexFileImage)

const uint8_t VersionMajor = 1;
const uint8_t VersionMinor = 4;
const uint8_t PageSize = 64;

Sodaq_RN2483Bootloader bootloader;
IntelHexParser hexParser(PageSize);

bool isDebugOn = false;
bool shouldEraseBlocks = true;
int8_t lastHexParserProgressPercent = -1;
bool shouldUseBootloaderMode = false;

bool onPageStart(uint32_t startingAddress);
bool onPageComplete(uint32_t startingAddress, const uint8_t* buffer, size_t size);
void onHexParserProgress(size_t currentLine, size_t totalLines);

bool onPageStart(uint32_t startingAddress)
{
    if (shouldEraseBlocks) {
        if (bootloader.eraseFlash(startingAddress, 1)) {
            debugPrint("Successfully erased block starting at 0x");
            debugPrintln(startingAddress, HEX);
            
            return true;
        }
        else {
            debugPrint("Failed to erase block starting at 0x");
            debugPrintln(startingAddress, HEX);
            
            return false;
        }
    }
    
    return true;
}

bool onPageComplete(uint32_t startingAddress, const uint8_t* buffer, size_t size)
{
    if (bootloader.writeFlash(startingAddress, buffer, size)) {
        debugPrint("Successfully wrote block starting at 0x");
        debugPrintln(startingAddress, HEX);
        
        return true;
    }
    else {
        debugPrint("Failed to write block starting at 0x");
        debugPrintln(startingAddress, HEX);
        
        return false;
    }
}

void onHexParserProgress(size_t currentLine, size_t totalLines)
{
    const uint8_t progressBarStepPercent = 2; // 1 step every x% done
    const uint8_t progressBarTextPercent = 25; // 1 text reference per x% done
    
    const uint8_t percent = (currentLine * 100) / (totalLines - 1);
    
    if (percent != lastHexParserProgressPercent) {
        lastHexParserProgressPercent = percent;
        
        if (percent % progressBarTextPercent == 0) {
            consolePrint(" ");
            consolePrint(percent);
            consolePrint("% ")
        }
        else if (percent % progressBarStepPercent == 0) {
            consolePrint("|");
        }
        
        if (percent == 100) {
            consolePrintln();
        }
    }
}

void setup()
{
    // Enable LoRaBee on Autonomo
    #if defined(ARDUINO_SODAQ_AUTONOMO)
    pinMode(BEE_VCC, OUTPUT);
    digitalWrite(BEE_VCC, HIGH); //set input power BEE high
    #endif

    #if defined(LORA_RESET)
    pinMode(LORA_RESET, OUTPUT);
    digitalWrite(LORA_RESET, LOW);
    sodaq_wdt_safe_delay(100);
    digitalWrite(LORA_RESET, HIGH);
    sodaq_wdt_safe_delay(100);
    #endif

    #if defined (ARDUINO_ARCH_ESP32)
        CONSOLE_STREAM.begin(115200);
    #else

    sodaq_wdt_safe_delay(5000);

    if (DEBUG_STREAM != CONSOLE_STREAM) {
        CONSOLE_STREAM.begin(115200);
    }
    #endif
    
    consolePrint("** ");

#if defined(HEXFILE_RN2483_101)
    consolePrint("RN2483 V1.0.1");
#elif defined(HEXFILE_RN2483_103)
    consolePrint("RN2483 V1.0.3");
#elif defined(HEXFILE_RN2903AU_097rc7)
    consolePrint("RN2903 V0.9.7");
#elif defined(HEXFILE_RN2903_098)
    consolePrint("RN2903 V0.9.8");
#else
    consolePrint("Unknown");
#endif

    consolePrintln(" Firmware Updater **");
    consolePrint("Version ");
    consolePrint(VersionMajor);
    consolePrint(".");
    consolePrintln(VersionMinor);

    
    consolePrintln("\nPress:");
    consolePrintln(" - \'b\' to enable bootloader mode");
    consolePrintln(" - \'d\' to enable debug");
    
    for (uint8_t i = 0; i < 5 * 4; i++) {
        while (CONSOLE_STREAM.available() > 0) {
            char c = CONSOLE_STREAM.read();
            
            if (c == 'b') {
                shouldUseBootloaderMode = true;
                
                consolePrintln("\nBootloader mode is now enabled.");
            }
            
            if (c == 'd') {
                isDebugOn = true;
                
                consolePrintln("\nDebug is now enabled.");
            }
        }
        
        sodaq_wdt_safe_delay(250);
        consolePrint(".");
    }
    
    consolePrintln();
    
    if (isDebugOn) {
        DEBUG_STREAM.begin(115200);
        
        bootloader.setDiag(DEBUG_STREAM);
        hexParser.setDiag(DEBUG_STREAM);
    }
    
    hexParser.setPageStartCallback(onPageStart);
    hexParser.setPageCompleteCallback(onPageComplete);
    hexParser.setProgressCallback(onHexParserProgress);
    
    consolePrintln("\n* Starting HEX File Image Verification...");
    
    // verify image first
    if (hexParser.verifyImageIntegrity()) {
        consolePrintln("HEX File Image Verification Successful!");
    }
    else {
        consolePrintln("HEX File Image Verification Failed!");
        consolePrintln("Cannot continue with firmware update!");
        
        while (true) { }
    }
    
    bootloader.initBootloader(LORA_STREAM);
}

void loop()
{
    if (shouldUseBootloaderMode) {
        uint32_t startMS = millis();

        LORA_STREAM.begin(bootloader.getDefaultBootloaderBaudRate());

        sodaq_wdt_safe_delay(200);
        
        BootloaderVersionInfo versionInfo;
        
        if (bootloader.getVersionInfo(versionInfo)) {
            consolePrintln("\n* The module is in Bootloader mode.");
            consolePrint("Bootloader Version: ");
            consolePrintln(versionInfo.BootloaderVersion, HEX);
            consolePrint("Device ID: ");
            consolePrintln(versionInfo.DeviceId, HEX);
            
            consolePrintln("\n* Starting firmware update...");
            
            if (hexParser.parseImage()) {
                consolePrintln("Firmware update has finished successfully! Please unplug the module to restart.");
            }
            else {
                consolePrintln("Failed to upload the firmware. Please unplug and restart.");
                
                while (true) { }
            }
            
            // consolePrintln("Resetting the module...");
            // bootloader.bootloaderReset();
            
            shouldUseBootloaderMode = false;
        }
        else {
            consolePrintln("The module did not respond in bootloader mode. Please unplug and retry in application mode.");
        }

        consolePrintln("Elapsed Time: " + String((float)(millis() - startMS) / 1000) + "s");
    }
    else {

        #if defined(LORA_RESET)
        consolePrint("Reseting module with PIN ");
        consolePrintln(LORA_RESET);
        digitalWrite(LORA_RESET, LOW);
        sodaq_wdt_safe_delay(100);
        digitalWrite(LORA_RESET, HIGH);
        sodaq_wdt_safe_delay(1000);
        #endif
        LORA_STREAM.end();
        LORA_STREAM.flush();

        LORA_STREAM.begin(bootloader.getDefaultApplicationBaudRate(), SERIAL_8N1, 17, 16);
        //Serial2.begin(9600, SERIAL_8N2, 16, 17)
        sodaq_wdt_safe_delay(100);

        
        char applicationResetResponse[64];
        if (bootloader.applicationReset(applicationResetResponse, sizeof(applicationResetResponse))) {
            consolePrintln("\n* The module is in Application mode: ");
            consolePrintln(applicationResetResponse);

            consolePrintln("\nReady to start firmware update...");
            consolePrint("Firmware Image: ");
            consolePrintln(HexFileImageName);
            consolePrintln("\nPlease press \'c\' to continue...");
            
            while (CONSOLE_STREAM.read() != 'c') { }
            
            consolePrintln("Erasing firmware and attempting to start bootloader...");
            bootloader.eraseFirmware();
            sodaq_wdt_safe_delay(1000);
            
            shouldUseBootloaderMode = true;
        }
        else {
            consolePrintln("The module did not respond in application mode. Please unplug and retry in bootloader mode.");
            consolePrintln(applicationResetResponse);
        }
    }
}
