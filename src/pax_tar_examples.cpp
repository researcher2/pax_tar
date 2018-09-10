#include <pax_tar.h> // Will resolve to VCPKG path

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
    //paxReading(testFileName);

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