#pragma once
#include <string>


class FileSystemManager{
    public:
        FileSystemManager();
        ~FileSystemManager();

        void createVirtualFileSystem(int size);
        void copyFileFromPhysicalDisk(std::string file_name);
        void copyFileFromVirtualDisk(std::string file_name);
        void displayCatalogue();
        void displayFileSystemInformation();
        void displayVirtualDiskStatus();
        int deleteFileFromVirtualDisk(std::string file_name);
        int deleteVirtualDisk();
    private:
        
};