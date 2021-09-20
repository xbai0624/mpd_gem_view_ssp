#include "EvioFileReader.h"

#include <iostream>

////////////////////////////////////////////////////////////////
// default ctor

EvioFileReader::EvioFileReader()
{
    // place holder
}

////////////////////////////////////////////////////////////////
// ctor

EvioFileReader::EvioFileReader(const char* file_name)
{
    fFileName = file_name;
    OpenFile();
}

////////////////////////////////////////////////////////////////
// ctor

EvioFileReader::EvioFileReader(std::string file_name)
{
    fFileName = file_name;
    OpenFile();
}

////////////////////////////////////////////////////////////////
// dtor

EvioFileReader::~EvioFileReader()
{
    // place holder
}

////////////////////////////////////////////////////////////////
// set evio file

void EvioFileReader::SetFile(const char* path)
{
    fFileName = path;
}

////////////////////////////////////////////////////////////////
// set evio file

void EvioFileReader::SetFile(std::string path)
{
    fFileName = path;
}

////////////////////////////////////////////////////////////////
// open evio file

bool EvioFileReader::OpenFile()
{
    int open_status = evOpen(const_cast<char*>(fFileName.c_str()), 
            const_cast<char*>(pReadFlag), &fFileHandle);

    if(open_status != 0) {
        std::cout<<"Error: EvioFileReader cannot open file: "<<fFileName
                 <<std::endl;
        return false;
    }

    std::cout<<"EvioFileReader:: openning file: "<<fFileName<<std::endl;

    return true;
}

////////////////////////////////////////////////////////////////
// close evio file

void EvioFileReader::CloseFile()
{
    evClose(fFileHandle);
}

////////////////////////////////////////////////////////////////
// read event buffer from evio file, no memory copy (fast)

int EvioFileReader::ReadNoCopy(const uint32_t **buf, uint32_t *buflen)
{
    int status = evReadNoCopy(fFileHandle, buf, buflen);
    
    if(status == S_SUCCESS)
        fEventNumber++;

    return status;
}

////////////////////////////////////////////////////////////////
// read event buffer from evio file, with memory copy

int EvioFileReader::ReadAlloc(uint32_t **buf, uint32_t *buflen)
{
    int status = evReadAlloc(fFileHandle, buf, buflen);

    if(status == S_SUCCESS) 
        fEventNumber++;

    return status;
}

////////////////////////////////////////////////////////////////
// read event buffer from evio file, if the event length is known

int EvioFileReader::Read(uint32_t *buf, uint32_t size)
{
    int status = evRead(fFileHandle, buf, size);

    if(status == S_SUCCESS)
        fEventNumber++;

    return status;
}

////////////////////////////////////////////////////////////////
// read event buffer from evio file, a specific event

int EvioFileReader::ReadEventNum(const uint32_t **pEvent, uint32_t *buflen,
        uint32_t eventNumber)
{
    return evReadRandom(fFileHandle, pEvent, buflen, eventNumber);
}

////////////////////////////////////////////////////////////////
// get current event number being processed

int EvioFileReader::GetEventNumber()
{
    return fEventNumber;
}

////////////////////////////////////////////////////////////////
// set file open mode
// "w" : writing mode
// "r" : reading mode
// "a" : appending mode
// "ra": random access
// "s" : splitting file

void EvioFileReader::SetFileOpenMode(const char* s)
{
    pReadFlag = const_cast<char*>(s);
}

