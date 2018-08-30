# pax_tar
Simple C++ Tar API using PAX Extended Header. Create &amp; Read Tars supporting large file sizes and path names. Currently doesn't support directories or symlinks but could easily be added if needed.

Inspiration and some code taken from https://github.com/rxi/microtar.

I wanted support for 64 bit file sizes under Windows as well as files > 8gb in the archive.  I originally modified microtar to add Pax Extended Headers but decided it would corrupt the projects simplicity.

## Supported Environments & Build Systems
### Prerequisites 
We use the excellent fmtlib: https://github.com/fmtlib/fmt.
- Windows: vcpkg install fmt:x64-windows
- Linux: clone from github, build and install

### Windows 10 x64
- vcpkg install pax_tar:x64-windows
- Visual Studio 2017 Project Included

### Linux (Tested Under CentOS)
cmake build file included

## Basic Usage
The below code is also available in "pax_tar_examples.cpp".

```cpp
#include "pax_tar.h"

#include <iostream>
using std::cout; using std::endl;

void paxWriting(string &testFileName)
{
    string data1 = "Hello world";
    string data2 = "Goodbye world";
    string data3 = "Guess who's back?";

    string path1 = "test1.txt";
    string path2 = "test2.txt";
    string path3 = "test3.txt";

    TarFile tarFile(testFileName);

    tarFile.addFile(path1, data1.size());
    tarFile.writeData((char*)data1.data(), data1.size());
    tarFile.finaliseFile();

    tarFile.addFile(path2, data2.size());
    tarFile.writeData((char*)data2.data(), data2.size());
    tarFile.finaliseFile();

    tarFile.addFile(path3, data3.size());
    tarFile.writeData((char*)data3.data(), data3.size());
    tarFile.finaliseFile();
}

void paxReading(string &testFileName)
{
    TarFile tarFile(testFileName);

    for (TarFileEntry &fileEntry : tarFile.files)
    {
        cout << "Path: " << fileEntry.path << " ";
        cout << "Size: " << fileEntry.size << " ";
        string loadedData;
        tarFile.getFileData(fileEntry, loadedData);
        cout << "Data: " << loadedData << endl;
    }
}
