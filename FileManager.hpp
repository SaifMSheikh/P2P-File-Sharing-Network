#include "Definitions.hpp"

#include <fstream>
#include <filesystem>

#include <iostream>
#include <optional>

using namespace std;
using namespace filesystem;

class FileManager {
    private:
        string directory;   // Path To Files
    public:
        FileManager(string dir);

        optional<path> FindFile(const char*);       // Search Directory For Specified File
        void GetFile(const char*, char*, size_t);   // Reads File From Directory
};