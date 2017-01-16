#include "IntelHexParser.h"
#include "HexFileImage.h"
#include "Utils.h"

#define DEBUG_SYMBOLS_ON

#ifdef DEBUG_SYMBOLS_ON
#define debugPrintln(...) { if (this->diagStream) this->diagStream->println(__VA_ARGS__); }
#define debugPrint(...) { if (this->diagStream) this->diagStream->print(__VA_ARGS__); }
#warning "Debug mode is ON"
#else
#define debugPrintLn(...)
#define debugPrint(...)
#endif

#define HEX_QUAD_TO_UINT16(LSBh, LSBl, MSBh, MSBl) ((HEX_PAIR_TO_BYTE(MSBh, MSBl) << 8) + (HEX_PAIR_TO_BYTE(LSBh, LSBl)))
#define BYTES_TO_UINT16(LSB, MSB) (MSB << 8 | LSB)


const uint8_t RecordColonOffset = 0;
const uint8_t RecordLengthOffset = RecordColonOffset + 1;
const uint8_t RecordAddressOffset = RecordLengthOffset + 2;
const uint8_t RecordTypeOffset = RecordAddressOffset + 4;
const uint8_t RecordDataOffset = RecordTypeOffset + 2;

const uint8_t LineSizeWithoutData = 11; // Start Code (1), Byte Count (2), Address (4), Record Type (2), Data (0), Checksum (2)
const uint8_t MaxDataSize = 0xFF;
const uint16_t MaxLineSize = LineSizeWithoutData + MaxDataSize;

enum RecordType {
    DataRecord = 0x00,
    EndOfFileRecord = 0x01,
    ExtendedSegmentAddressRecord = 0x02,
    StartSegmentAddressRecord = 0x03,
    ExtendedLinearAddressRecord = 0x04,
    StartLinearAddressRecord = 0x05
};

IntelHexParser::IntelHexParser(size_t pageSize) :
    diagStream(0),
    extendedAddressOffset(0),
    isBufferInitialized(0),
    isLive(0),
    pageSize(0),
    pageBuffer(0),
    pageStartAddress(0),
    isPageDirty(0),
    pageStartCallback(0),
    progressCallback(0),
    pageCompleteCallback(0)
{
    this->pageSize = pageSize;
    
    // make sure the buffer is only initialized once
    if (!isBufferInitialized) {
        this->pageBuffer = static_cast<uint8_t*>(malloc(this->pageSize));
        
        isBufferInitialized = true;
    }
}

// start from an address that is a multiple of the page size and contains the target address
// updates the pageStartAddress
bool IntelHexParser::startNewPage(uint32_t startingAddress)
{
    memset(pageBuffer, 0xFF, pageSize);
    
    pageStartAddress = trunc(startingAddress / pageSize) * pageSize; // find the "enclosing page" starting address
    
    debugPrint("startNewPage(0x");
    debugPrint(startingAddress, HEX);
    debugPrint("): starting at 0x");
    debugPrintln(pageStartAddress, HEX);
    
    isPageDirty = false;
    
    if (isLive && pageStartCallback != 0) {
        return pageStartCallback(pageStartAddress);
    }
    
    return true;
}

// only if there was a write in the page
bool IntelHexParser::completePage()
{
    debugPrintln("completePage()");
    
    if (isLive && isPageDirty && pageCompleteCallback != 0) {
        return pageCompleteCallback(pageStartAddress, const_cast<const uint8_t*>(pageBuffer), pageSize);
    }
    
    return true;
}

void IntelHexParser::reportProgress(size_t currentLine, size_t totalLines)
{
    if (progressCallback != 0) {
        progressCallback(currentLine, totalLines);
    }
}

void IntelHexParser::writeToPage(uint32_t targetAddress, uint8_t b)
{
    pageBuffer[targetAddress - pageStartAddress] = b;
    isPageDirty = true;
}

bool IntelHexParser::parseLine(const char* line)
{
    size_t lineLength = strlen(line);
    
    // sum up all bytes, including the checksum field (which is the 2's complement of the LSB of the sum), should be 0 at the end
    uint8_t calculatedChecksum = 0;
    
    // sanity check
    if ((lineLength < LineSizeWithoutData) || (lineLength > MaxLineSize) || (line[0] != ':')) {
        debugPrintln("Sanity check failed.");
        return false;
    }
    
    uint8_t recordLength = HEX_PAIR_TO_BYTE(
                               line[RecordLengthOffset + 0],
                               line[RecordLengthOffset + 1]);
    calculatedChecksum += recordLength;
    
    uint16_t recordAddress = HEX_QUAD_TO_UINT16(
                                 line[RecordAddressOffset + 2],
                                 line[RecordAddressOffset + 3],
                                 line[RecordAddressOffset + 0],
                                 line[RecordAddressOffset + 1]); // swapped because addresses are Big Endian in the file
    calculatedChecksum += (uint8_t)(recordAddress >> 8);
    calculatedChecksum += (uint8_t)recordAddress;
    
    RecordType recordType = (RecordType)HEX_PAIR_TO_BYTE(
                                line[RecordTypeOffset + 0],
                                line[RecordTypeOffset + 1]);
    calculatedChecksum += recordType;
    
    uint8_t recordChecksum = HEX_PAIR_TO_BYTE(
                                 line[lineLength - 2],
                                 line[lineLength - 1]);
    calculatedChecksum += recordChecksum;
    
    // check record length validity before converting the data section
    if (lineLength != (size_t)(LineSizeWithoutData + 2 * recordLength)) {
        debugPrintln("The input line length is not equal to the reported record length!");
        
        return false;
    }
    
    uint8_t data[recordLength];
    
    for (uint8_t i = 0; i < recordLength; i++) {
        data[i] = HEX_PAIR_TO_BYTE(
                      line[RecordDataOffset + i * 2 + 0],
                      line[RecordDataOffset + i * 2 + 1]);
                      
        calculatedChecksum += data[i];
    }
    
    // check record checksum before proceeding
    if (calculatedChecksum != 0) {
        debugPrintln("The input line checksum is not equal to the reported record checksum!");
        return false;
    }
    
    switch (recordType) {
        case DataRecord: {
                // debugPrintln("Data Record");
                
                uint32_t startAddress = extendedAddressOffset + recordAddress;
                
                for (uint8_t i = 0; i < recordLength; i++) {
                    uint32_t targetAddress = startAddress + i;
                    
                    if (targetAddress < pageStartAddress || targetAddress > pageStartAddress + pageSize - 1) {
                        if (!completePage()) {
                            debugPrintln("The Callback to complete the current page failed!");
                            return false;
                        }
                        
                        // start from an address that is a multiple of the page size and contains the target address
                        if (!startNewPage(targetAddress)) { // updates the pageStartAddress
                            debugPrintln("The Callback to start a new page failed!");
                            return false;
                        }
                    }
                    
                    writeToPage(targetAddress, data[i]);
                }
            }
            
            break;
            
        case ExtendedLinearAddressRecord: {
                // debugPrintln("Extended Linear Address Record");
                
                extendedAddressOffset = BYTES_TO_UINT16(data[1], data[0]) << 16; // swapped because addresses are in big endian
            }
            break;
            
        case ExtendedSegmentAddressRecord: {
                // debugPrintln("Extended Segment Address Record");
                
                extendedAddressOffset = BYTES_TO_UINT16(data[1], data[0]) * 16U; // swapped because addresses are in big endian
            }
            break;
            
        case EndOfFileRecord: {
                // debugPrintln("End Of File Record");
                // TODO report when (not) found?
                if (!completePage()) {
                    debugPrintln("The Callback to complete the current page failed!");
                    return false;
                }
            }
            break;
            
        case StartSegmentAddressRecord: {
                // debugPrintln("Start Segment Address Record");
                // TODO not really needed
                
                return false;
            }
            break;
            
        case StartLinearAddressRecord: {
                // debugPrintln("Start Linear Address Record");
                // TODO not really needed
                
                return false;
            }
            break;
            
        default:
            debugPrintln("Unknown record type!");
            
            return false;
    }
    
    return true;
}

void IntelHexParser::setProgressCallback(ProgressCallback cb)
{
    progressCallback = cb;
}

void IntelHexParser::setPageStartCallback(PageStartCallback cb)
{
    pageStartCallback = cb;
}

void IntelHexParser::setPageCompleteCallback(PageCompleteCallback cb)
{
    pageCompleteCallback = cb;
}

bool IntelHexParser::verifyImageIntegrity()
{
    isLive = false;
    return iterateThroughImage();
}

bool IntelHexParser::parseImage()
{
    isLive = true;
    return iterateThroughImage();
}

bool IntelHexParser::iterateThroughImage()
{
    extendedAddressOffset = 0;
    
    size_t totalLines = ARRAY_SIZE(HexFileImage);
    
    for (size_t i = 0; i < totalLines; i++) {
        reportProgress(i, totalLines);
        delay(1);
        
        if (!parseLine(HexFileImage[i])) {
            debugPrintln("Failure!");
            
            return false;
        }
    }
    
    return true;
}
