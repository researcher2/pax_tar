#include "pax_tar.h"

#include <assert.h>

#include <iostream>
using std::cout; using std::endl;

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <algorithm>

void largeFileSizeTest();
void basicTest();

string testFileName = "pax_test.tar";

int main()
{
    if (fs::exists(testFileName))
        fs::remove(testFileName);

    basicTest();
    //largeFileSizeTest();

    getchar();
}

void basicTest()
{
    string path1 = "test1.txt";
    string path2 = "test2.txt";
    string path3 = "test3.txt";

    string data1 = "Hello world";
    string data2 = "Goodbye world";
    string data3 = "Guess who's back?";

    unordered_map<string, string*> files;
    files.insert({ path1, &data1 });
    files.insert({ path2, &data2 });
    files.insert({ path3, &data3 });

    string path4 = "test4.txt";
    string data4 = "Who Is It?";

    auto verify = [&](TarFile &tarFile)
    {
        // Test Deque Iteration
        for (auto &TarFileEntry : tarFile.files)
        {
            assert(files.count(TarFileEntry.path) == 1);

            string &fileData = *files[TarFileEntry.path];
            assert(TarFileEntry.size == fileData.size());

            string loadedData;
            tarFile.getFileData(TarFileEntry, loadedData);
            assert(loadedData == fileData);
        }

        // Test Index Lookup
        for (auto &kv : files)
        {
            const string &fileName = kv.first;
            string &fileData = *kv.second;

            assert(tarFile.filesByPath.count(fileName) == 1);

            TarFileEntry &entry = *tarFile.filesByPath[fileName];

            assert(entry.path == fileName);
            assert(entry.size == fileData.size());

            string loadedData;
            tarFile.getFileData(entry, loadedData);
            assert(loadedData == fileData);
        }
    };

    {
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

        // Verify before close
        verify(tarFile);
    }

    {
        // Verify after re-open
        TarFile tarFile(testFileName);
        verify(tarFile);

        // add additional file to existing archive
        files.insert({ path4, &data4 });

        tarFile.addFile(path4, data4.size());
        tarFile.writeData((char*)data4.data(), data4.size());
        tarFile.finaliseFile();

        // Verify again
        verify(tarFile);
    }

    {
        // Verify after re-open
        TarFile tarFile(testFileName);
        verify(tarFile);
    }

    cout << "basicTest Successful!" << endl;
}

void largeFileSizeTest()
{
    string data1 = "Hello world";
    string data2 = "Goodbye world";
    string data3 = "Guess who's back?";
    string data4;

    size_t bigFileTestSize = 9000 * 1000000LL; //9gb
    data4.reserve(bigFileTestSize + 5 * 1000000);

    // Approx 1 megabyte
    string reallyLongBase = data3;
    for (int i = 0; i < 16; ++i)        
        reallyLongBase.append(reallyLongBase);

    while (data4.size() < bigFileTestSize)
        data4.append(reallyLongBase);

    string path1 = "test1.txt";
    string path2 = "test2.txt";
    string path3 = "test3.txt";
    string path4 = "bigfile.dat";

    unordered_map<string, string*> files;
    files.insert({ path1, &data1 });
    files.insert({ path2, &data2 });
    files.insert({ path3, &data3 });    
    files.insert({ path4, &data4 });

    {
        TarFile tarFile(testFileName);

        tarFile.addFile(path1, data1.size());
        tarFile.writeData((char*)data1.data(), data1.size());
        tarFile.finaliseFile();

        tarFile.addFile(path2, data2.size());
        tarFile.writeData((char*)data2.data(), data2.size());
        tarFile.finaliseFile();

        tarFile.addFile(path4, data4.size());
        tarFile.writeData((char*)data4.data(), data4.size());
        tarFile.finaliseFile();

        tarFile.addFile(path3, data3.size());
        tarFile.writeData((char*)data3.data(), data3.size());
        tarFile.finaliseFile();
    }

    {
        TarFile tarFile(testFileName);

        // Test Deque Iteration
        for (auto &TarFileEntry : tarFile.files)
        {
            assert(files.count(TarFileEntry.path) == 1);

            string &fileData = *files[TarFileEntry.path];
            assert(TarFileEntry.size == fileData.size());

            string loadedData;
            tarFile.getFileData(TarFileEntry, loadedData);
            assert(loadedData == fileData);
        }

        // Test Index Lookup
        for (auto &kv : files)
        {
            const string &fileName = kv.first;
            string &fileData = *kv.second;

            assert(tarFile.filesByPath.count(fileName) == 1);

            TarFileEntry &entry = *tarFile.filesByPath[fileName];

            assert(entry.path == fileName);
            assert(entry.size == fileData.size());

            string loadedData;
            tarFile.getFileData(entry, loadedData);
            assert(loadedData == fileData);
        }
    }

    cout << "largeFileSizeTest Test Successful!" << endl;
}