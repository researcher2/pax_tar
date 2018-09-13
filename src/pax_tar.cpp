#define _CRT_SECURE_NO_WARNINGS

#include "pax_tar.h"

#include <cstdlib>

#include <fmt/format.h>

#include <iostream>
using std::cout; using std::endl;

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

using std::stoll;

// Tar Ustar Header
// Size field is stored in 11 bytes of octal with a null byte.
// 8gb Max File Size, 99 Character Max File Name
// See: https://www.gnu.org/software/tar/manual/html_chapter/tar_8.html
//      http://www.gnu.org/software/tar/manual/html_node/Standard.html

struct TarUSTARHeader
{
    char name[100];
    char mode[8];
    char owner[8];
    char group[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char linkname[100];
    char ustar[6];
    char ustarVersion[2];
    char _padding[247];
};

static const string nullBlock(512, '\0');

constexpr int rawHeaderSize = sizeof(TarUSTARHeader);

enum
{
    MTAR_TREG = '0',
    MTAR_TLNK = '1',
    MTAR_TSYM = '2',
    MTAR_TCHR = '3',
    MTAR_TBLK = '4',
    MTAR_TDIR = '5',
    MTAR_TFIFO = '6',
    MTAR_PAX = 'x'
};

uint32_t checksum(TarUSTARHeader *rh);
void parseRawHeader(TarUSTARHeader *rh, TarFileEntry &file);
void paxOverride(string &paxHeaderData, TarFileEntry &file);
size_t roundUp(size_t n, size_t incr);
void seekToNearest512(fstream &stream);
string buildPaxHeaderRecord(const char *keyword, const char *value);
void setupHeader(TarUSTARHeader &header, string &name, char type, int mode, unsigned int size);

TarFile::TarFile(string &fileName)
{
    if (!fs::exists(fileName))
    {
        // Create New File
        fileStream.open(fileName, std::fstream::out | std::fstream::binary);
        finaliseArchive(); // We always have 1024 null bytes at end of file for easy close.
        fileStream.close();
    }
    
    fileStream.open(fileName, std::fstream::in | std::fstream::out | std::fstream::binary);   
    if (!fileStream)
        throw std::runtime_error("Failed to open tar file");

    TarUSTARHeader header;

    while (true)
    {
        fileStream.read((char*)&header, rawHeaderSize);
        if (!fileStream)
            throw std::runtime_error("Invalid End Of Tar File");        

        // End Of File? check for 2 x 512 null filled
        if (memcmp(&header, nullBlock.data(), nullBlock.size()) == 0)
        {
            fileStream.read((char*)&header, rawHeaderSize);
            if (!fileStream)
                throw std::runtime_error("Invalid End Of Tar File");

            if (memcmp(&header, nullBlock.data(), nullBlock.size()) == 0)
            {
                endOfFile1024Offset = fileStream.tellg();
                endOfFile1024Offset -= 1024;
                break;
            }
            else
                throw std::runtime_error("Invalid End Of Tar File");
        }

        TarFileEntry fileEntry;
        parseRawHeader(&header, fileEntry);

        if (fileEntry.type == MTAR_PAX)
        {
            // Read The Pax Header
            string paxHeaderData;
            paxHeaderData.resize(fileEntry.size);
            fileStream.read((char*)paxHeaderData.data(), fileEntry.size);
            if (!fileStream)
                break;

            seekToNearest512(fileStream);

            // Read The Proceeding USTAR Header
            TarUSTARHeader actualFileHeader;            
            fileStream.read((char*)&actualFileHeader, rawHeaderSize);
            if (!fileStream)
                throw std::runtime_error("Invalid End Of Tar File");

            if (actualFileHeader.type != MTAR_TREG)
                throw std::runtime_error("Proceeding USTAR Header is not for standard file.");

            TarFileEntry actualFileEntry;
            parseRawHeader(&actualFileHeader, actualFileEntry);
            actualFileEntry.fileOffset = fileStream.tellg();

            // Pax header entries override corresponding USTAR attributes
            paxOverride(paxHeaderData, actualFileEntry);

            files.push_back(actualFileEntry);
            filesByPath.insert({ actualFileEntry.path , &files.back() });

            // Skip Data (which pads up to multiples of 512)
            fileStream.seekg(actualFileEntry.size, fileStream.cur);
            seekToNearest512(fileStream);
        }
        else if (fileEntry.type == MTAR_TREG)
        {
            fileEntry.fileOffset = fileStream.tellg();

            files.push_back(fileEntry);
            filesByPath.insert({ fileEntry.path , &files.back() });

            // Skip Data (which pads up to multiples of 512)
            fileStream.seekg(fileEntry.size, fileStream.cur);
            seekToNearest512(fileStream);
        }
        else
        {
            cout << "Something else" << endl;
        }
    }
}

void TarFile::getFileData(TarFileEntry &entry, string &data)
{
    fileStream.seekg(entry.fileOffset);
    data.resize(entry.size);
    fileStream.read((char*)data.data(), entry.size);
}

void setupHeader(TarUSTARHeader &header, string &name, char type, int mode, unsigned int size)
{
    memset(&header, 0, sizeof(TarUSTARHeader));
    memcpy(header.ustar, "ustar", 6);
    memcpy(header.ustarVersion, "00", 2);

    strcpy(header.name, name.c_str());

    header.type = type;
    sscanf(header.mode, "%o", &mode);
    sprintf(header.size, "%o", size);

    unsigned int chksum = checksum(&header);
    sprintf(header.checksum, "%06o", chksum);
    header.checksum[7] = ' ';
}

void TarFile::addFile(string &fileName, size_t fileSize)
{
    // Build Pax Header
    string paxHeader;
    string pathRecord = buildPaxHeaderRecord("path", fileName.c_str());
    string sizeRecord = buildPaxHeaderRecord("size", fmt::format("{}", fileSize).c_str());
    paxHeader.append(pathRecord);
    paxHeader.append(sizeRecord);

    // Overwrite EOF Marker
    fileStream.seekg(endOfFile1024Offset);

    // Build & Write First USTAR Header (type = 'x')
    {
        TarUSTARHeader header;
        string name = "Pax Extended Header";
        setupHeader(header, name, MTAR_PAX, 0, static_cast<unsigned int>(paxHeader.size()));
        fileStream.write((char*)&header, sizeof(TarUSTARHeader));
    }

    // Write Pax Header
    fileStream.write(paxHeader.data(), paxHeader.size());
    finaliseInternal(); // blank fill to nearest 512

    // Build & Write Second USTAR Header (type = '0')
    {
        TarUSTARHeader header;
        string name = "To be overwritten";
        setupHeader(header, name, MTAR_TREG, 0664, fileSize);
        fileStream.write((char*)&header, sizeof(TarUSTARHeader));

        TarFileEntry fileEntry;
        parseRawHeader(&header, fileEntry);
        paxOverride(paxHeader, fileEntry);
        fileEntry.fileOffset = fileStream.tellg();

        files.push_back(fileEntry);
        filesByPath.insert({ fileEntry.path , &files.back() });
    }
}

void TarFile::writeData(char *data, size_t dataLength)
{
    fileStream.write(data, dataLength);
}

void TarFile::finaliseInternal()
{
    size_t filePosition = fileStream.tellg();
    seekToNearest512(fileStream);
    size_t blockEndPosition = fileStream.tellg();
    int nullBytes = static_cast<unsigned int>(blockEndPosition - filePosition);

    fileStream.seekg(filePosition);
    for (int i = 0; i < nullBytes; ++i)
        fileStream << '\0';

    endOfFile1024Offset = fileStream.tellg();
}

void TarFile::finaliseFile()
{
    finaliseInternal();
    finaliseArchive();
}

void TarFile::finaliseArchive()
{
    for (int i = 0; i < 1024; ++i)
        fileStream << '\0';
}

void seekToNearest512(fstream &stream)
{
    size_t filePosition = stream.tellg();
    filePosition = roundUp(filePosition, 512);
    stream.seekg(filePosition);
}

size_t roundUp(size_t n, size_t incr)
{
    return n + (incr - n % incr) % incr;
}

uint32_t checksum(TarUSTARHeader *rh)
{
    unsigned char *p = (unsigned char*)rh;
    uint32_t res = 256;
    for (uint32_t i = 0; i < offsetof(TarUSTARHeader, checksum); ++i)
        res += p[i];

    for (uint32_t i = offsetof(TarUSTARHeader, type); i < sizeof(*rh); ++i)
        res += p[i];

    return res;
}

// Pax Header Format
//==========================================================================//
// The Pax Extended Header containts key value pairs supporting unlimited file
// and name length among other things. It is of the format: 
//     "%d %s=%s\n", <length>, <keyword>, <value>
// Currently supported:
//     unlimited size file paths
//     unlimited file sizes

string buildPaxHeaderRecord(const char *keyword, const char *value)
{
    // The length section of a record is bytes in decimal representation and includes the characters to
    // represent the length itself. Gross. Yo Dawg.

    string data = fmt::format(" {}={}\n", keyword, value);

    string lengthString = fmt::format("{}", data.size());
    while (true)
    {
        int totalLength = static_cast<int>(lengthString.size() + data.size());
        string lengthString2 = fmt::format("{}", totalLength);

        if (lengthString2.size() == lengthString.size())
            break;
        else
            lengthString = lengthString2;
    }

    string record;
    record.append(lengthString);
    record.append(data);
    return record;
}

void parseRawHeader(TarUSTARHeader *rh, TarFileEntry &file)
{
    unsigned chksum1, chksum2;

    /* If the checksum starts with a null byte we assume the record is NULL */
    if (*rh->checksum == '\0')
        throw std::runtime_error("Null Checksum");

    /* Build and compare checksum */
    chksum1 = checksum(rh);
    sscanf(rh->checksum, "%o", &chksum2);
    if (chksum1 != chksum2)
        throw std::runtime_error("Bad Checksum");

    /* Load raw header into header */
    unsigned int size;
    sscanf(rh->size, "%o", &size);
    file.size = size;

    sscanf(rh->mode, "%o", &file.mode);
    sscanf(rh->owner, "%o", &file.owner);    
    sscanf(rh->mtime, "%o", &file.mtime);
    file.type = rh->type;
    file.path.append(rh->name);
    strcpy(file.linkname, rh->linkname);

}

void paxOverride(string &paxHeaderData, TarFileEntry &file)
{
    char *paxHeaderString = (char*)paxHeaderData.data();

    // Tokenise
    char *token = strtok(paxHeaderString, "=");
    char *pathValue = strtok(NULL, "\n");

    token = strtok(NULL, "=");
    char *sizeValue = strtok(NULL, "\n");

    // Path
    file.path = pathValue;

    // Size
    string sizeString(sizeValue);
    file.size = stoll(sizeString);
}