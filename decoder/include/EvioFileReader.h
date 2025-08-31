#ifndef EVIO_FILE_READER_H
#define EVIO_FILE_READER_H

////////////////////////////////////////////////////////////////
// A Wrapper for "evio.h"
// We choose to use the c version evio header "evio.h" over the
// c++ version "evioUtil.hxx"; 
// For two reasons:
//     1) the c version is slightly faster
//     2) the c++ standard has evolved a lot. Up to now, C++20 
//        has been finalized, however c++ in "evioUtil.hxx"
//        lags behind, it used some features that has been 
//        deprecated by the new c++ standard. It pops up lots of
//        error/warning messeages while compiling "evioUtil.hxx"
//        using modern c++ compiler

#include "evio.h"

#include <string>

////////////////////////////////////////////////////////////////
// Read an evio file, return event by event

class EvioFileReader
{
public:
    EvioFileReader();
    EvioFileReader(const char*);
    EvioFileReader(std::string);

    ~EvioFileReader();

    bool OpenFile();
    void CloseFile();
    void SetFile(const char*);
    void SetFile(std::string);
    void SetFileOpenMode(const char* mode);

    // get event, load the event to a buffer
    int ReadNoCopy(const uint32_t **buf, uint32_t *buflen);
    int ReadAlloc(uint32_t  **buf, uint32_t *buflen);
    int Read(uint32_t *buf, uint32_t size);
    int ReadEventNum(const uint32_t **pEvent, uint32_t *buflen, uint32_t eventNumber);

    int GetEventNumber();
    std::string GetFilePath(){return fFileName;}

private:
    std::string fFileName;
    int fFileHandle;
    const char* pReadFlag = "r";
    int fEventNumber = 0;
};

#endif
