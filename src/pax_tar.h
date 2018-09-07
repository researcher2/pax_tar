#pragma once

#include <fstream>
using std::fstream;

#include <unordered_map>
using std::unordered_map;

#include <deque>
using std::deque;

#include <string>
using std::string;

struct TarFileEntry
{
    size_t fileOffset;

    string path; // supported by pax extended
    int64_t size; // supported by pax extended
    unsigned int mode;
    unsigned int owner;
    unsigned int mtime;
    unsigned int type;
    char linkname[100];
};

class TarFile
{
public:
    deque<TarFileEntry> files;
    unordered_map<string, TarFileEntry*> filesByPath; // index

public:
    TarFile(string &fileName);
    void addFile(string &fileName, size_t fileSize);
    void writeData(char *data, size_t dataLength);
    void finaliseFile();    

    void getFileData(TarFileEntry &entry, string &data);

private:
    fstream fileStream;
    size_t endOfFile1024Offset;

private:
    void finaliseArchive();
};