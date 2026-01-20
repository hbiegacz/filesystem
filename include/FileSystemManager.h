#pragma once
#include <string>
#include <cstdint>
#include <fstream>

struct Superblock {
    uint32_t magicNumber;      
    uint32_t blockSize;        
    uint32_t inodesCount;      
    uint32_t blocksCount;      
    
    uint64_t inodeTableOffset;
    uint64_t inodeBitmapOffset;
    uint64_t blockBitmapOffset;
    uint64_t dataBlocksOffset;

    uint32_t freeBlocksCount;
    uint32_t freeInodesCount;
};

struct Inode {
    long creationTime;
    uint64_t fileSize;
    uint32_t dataBlockPointers[12]; 
    bool isDirectory;
    uint16_t hardLinkCount;
};

struct DirectoryEntry {
    char fileName[32]; 
    uint32_t inodeIndex;
};



class FileSystemManager{
    public:
        FileSystemManager(std::string virtual_disk_path);
        ~FileSystemManager();

        void createVirtualFileSystem(uint64_t requestedDataSize);
        void copyFileFromPhysicalDisk(std::string file_name);
        void copyFileFromVirtualDisk(std::string file_name);
        void displayCatalogue();
        void displayFileSystemInformation();
        void displayVirtualDiskStatus();
        int deleteFileFromVirtualDisk(std::string file_name);
        int deleteVirtualDisk();
    private:
        std::string virtual_disk_path_;
        Superblock initializeSuperblock(uint32_t magic, uint32_t blockSize, uint32_t inodes, uint32_t blocks);
        void initializeSection(std::ofstream& file, uint64_t offset, uint64_t size, bool isBitmap);
        void writeBuffer(std::ofstream& file, const char* data, uint64_t size);
        uint64_t alignToBlock(uint64_t offset, uint32_t blockSize);
};