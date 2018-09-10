# pax_tar
Simple C++ Tar API using PAX Extended Header. Create &amp; Read Tars supporting large file sizes and path names. Currently doesn't support directories or symlinks but could easily be added if needed.

Inspiration and some code taken from https://github.com/rxi/microtar.

I wanted support for 64 bit file sizes under Windows as well as files > 8gb in the archive.  I originally modified microtar to add Pax Extended Headers but decided it would corrupt the projects simplicity.

## Supported Environments & Build Systems
### Prerequisites 
We use the excellent fmtlib: https://github.com/fmtlib/fmt.
- Windows: automatic dependency in VCPKG
- Linux: clone from github, checkout 5.1.0, build and install with cmake

### Windows 10 x64 VCPKG
As this is not a fully complete TAR library I have not submitted it to the VCPKG curated list. However, I have written the port file so you can still easily install with VCPKG.

- Clone https://github.com/researcher2/pax_tar_vcpkg_port.git into "vcpkg/ports/pax-tar"
```
vcpkg install pax_tar:x64-windows
```

### Windows 10 x64 Visual Studio
Visual Studio 2017 Solution included for self building:
- Static CRT lib
- Dynamic CRT lib
- Test Project
- Examples Project (can be used to test vcpkg dynamic crt linkage)
- Testing vcpkg static crt linkage project

### Linux (Tested Under CentOS)
We use meson: https://mesonbuild.com/index.html

Note that the final install step assumes you want to install into the system directories and you have meson installed by sudo user already.

```sh
git clone https://github.com/researcher2/pax_tar.git
cd pax_tar
mkdir build
cd build
meson ..
ninja
sudo ninja install
```

## Basic Usage
The below code is also available in "pax_tar_examples.cpp".

```cpp
#include <pax_tar.h> // Will resolve to VCPKG or system path

#include <iostream>
using std::cout; using std::endl;

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

void paxWriting(string &testFileName);
void paxReading(string &testFileName);

int main()
{
    string testFileName = "test.tar";
    if (fs::exists(testFileName))
        fs::remove(testFileName);
    
    paxWriting(testFileName);
    paxReading(testFileName);

    getchar();
}

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

    // Iterate through all files
    for (TarFileEntry &fileEntry : tarFile.files)
    {
        string loadedData;
        tarFile.getFileData(fileEntry, loadedData);

        cout << "Path: " << fileEntry.path << " ";
        cout << "Size: " << fileEntry.size << " ";
        cout << "Data: " << loadedData << endl;
    }

    // Find functionality:
    if (tarFile.filesByPath.count("test1.txt"))
    {
        TarFileEntry &fileEntry = *tarFile.filesByPath["test1.txt"];
        string loadedData;
        tarFile.getFileData(fileEntry, loadedData);

        cout << "Path: " << fileEntry.path << " ";
        cout << "Size: " << fileEntry.size << " ";
        cout << "Data: " << loadedData << endl;
    }
}
```
