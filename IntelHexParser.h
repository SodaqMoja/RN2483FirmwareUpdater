#ifndef _INTEL_HEX_PARSER_H_
#define _INTEL_HEX_PARSER_H_

#include "Arduino.h"

typedef bool (*PageStartCallback)(uint32_t startingAddress);
typedef void (*ProgressCallback)(size_t currentLine, size_t totalLines);
typedef bool (*PageCompleteCallback)(uint32_t startingAddress, const uint8_t* buffer, size_t size);

class IntelHexParser
{
    public:
        IntelHexParser(size_t pageSize);
        
        void setDiag(Stream& stream) { diagStream = &stream; };
        
        void setProgressCallback(ProgressCallback cb);
        void setPageStartCallback(PageStartCallback cb);
        void setPageCompleteCallback(PageCompleteCallback cb);
        
        bool verifyImageIntegrity();
        bool parseImage();
    protected:
        Stream* diagStream;
        
        uint32_t extendedAddressOffset;
        
        bool isBufferInitialized;
        
        bool isLive;
        size_t pageSize;
        uint8_t* pageBuffer;
        uint32_t pageStartAddress;
        bool isPageDirty;
        
        PageStartCallback pageStartCallback;
        ProgressCallback progressCallback;
        PageCompleteCallback pageCompleteCallback;
        
        bool startNewPage(uint32_t startingAddress);
        bool completePage();
        void reportProgress(size_t currentLine, size_t totalLines);
        void writeToPage(uint32_t targetAddress, uint8_t b);
        bool parseLine(const char* line);
        bool iterateThroughImage();
};

#endif
