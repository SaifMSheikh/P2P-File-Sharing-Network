#include "FileManager.hpp"

using namespace std;
using namespace filesystem;

FileManager::FileManager(string dir) : directory(dir) {}

optional<path> FileManager::FindFile(const char* fileName) {
    // Default Iterator Refers To End Of Directory
    directory_iterator endDir;
    // Search Directory
    for (directory_iterator i(directory); i != endDir; ++i) {
        // Skip Irregular Files
        if (!i->is_regular_file()) continue; 
        // Check For Match
        if (i->path().filename() == fileName) return i->path();
    }
    // If File Not Found
    return nullopt;
}

void FileManager::GetFile(const char* fileName, char* content, size_t bufferSize) {
    // Search Directory For Match
    optional<path> filePath = FindFile(fileName);
    if (filePath == nullopt) 
        throw runtime_error("GetFile : Failed To Find File : ");
    // Get File Size
    ifstream file(*filePath);           // File Stream For Extraction
    file.seekg(0, ifstream::end);       // Set Stream Buffer To End
    streampos size = file.tellg();      // Gets Current Position Of Stream Buffer
    file.seekg(0, ifstream::beg);       // Returns Sream Buffer To Starting Position
    // Check File Size
    if (size > bufferSize)  {
        file.close();
        throw runtime_error("GetFile : Size Limit Exceeded In File : ");
    }
    // Read Contents
    file.read(content, size);
    // Close File
    file.close();
}