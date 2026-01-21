#pragma once
#include <string>
#include <cstdint>
#include <fstream>
#include <vector>

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
    uint32_t directPointers[12]; 
    uint32_t singlyIndirectPointer;
    bool isDirectory;
    uint16_t hardLinkCount;
};

struct DirectoryEntry {
    char fileName[32]; 
    uint32_t inodeIndex;
};


// - Tworzenie twardych dowiązań do plików i katalogów.
// - Usuwanie plików i dowiązań.
// - Dodawanie lub usuwanie n bajtów w plikach.
// - Wyświetlanie zajętości dysku wirtualnego.


class FileSystemManager{
    public:
        FileSystemManager(std::string virtual_disk_path, std::string output_path);
        ~FileSystemManager();

        void createVirtualFileSystem(uint64_t requestedDataSize);

        void copyFileFromPhysicalDisk(std::string file_name);
        void copyFileFromVirtualDisk(std::string file_name);
        int deleteFileFromVirtualDisk(std::string path);
        
        void createDirectory(std::string directory_name);
        void deleteDirectory(std::string directory_name);
        void displayDirectory(std::string directory_name);
        
        void displayFileSystemInformation();


    private:
        std::string virtual_disk_path_;
        std::string output_path_;
        
        Superblock initializeSuperblock(uint32_t magic, uint32_t blockSize, uint32_t inodes, uint32_t blocks);
        void initializeSection(std::ofstream& file, uint64_t offset, uint64_t size, bool isBitmap);
        void writeBuffer(std::ofstream& file, const char* data, uint64_t size);
        uint64_t alignToBlock(uint64_t offset, uint32_t blockSize);

        Superblock readSuperblock();
        void writeSuperblock(const Superblock& superblock);
        
        std::vector<uint8_t> readBitmap(uint64_t offset, uint32_t bitsCount);
        void writeBitmap(uint64_t offset, const std::vector<uint8_t>& bitmap);
        void setBitmapBit(uint64_t bitmapOffset, uint32_t index, bool value);
        int findFreeInodeIndex(const std::vector<uint8_t>& inodeBitmap);
        std::vector<uint32_t> allocateDataBlocks(std::vector<uint8_t>& blockBitmap, uint32_t requiredBlocks, uint32_t blockSize);
        uint32_t allocateSingleBlock(Superblock& sb, std::vector<uint8_t>& blockBitmap);

        void writeInode(uint32_t inodeIndex, const Inode& inode);
        Inode readInode(uint32_t inodeIndex);

        template <typename T>
        T readStruct(uint64_t offset);

        template <typename T>
        void writeStruct(uint64_t offset, const T& data);

        uint64_t getSourceFileSize(std::ifstream& sourceFile);
        uint32_t calculateRequiredBlocks(uint64_t fileSize, uint32_t blockSize);
        void validateFileSize(uint32_t requiredBlocks, uint32_t maxSupportedBlocks);
        void writeIndirectPointers(uint32_t indirectBlockIndex, const std::vector<uint32_t>& dataBlocks, uint32_t blockSize);
        void writeFileDataToBlocks(std::ifstream& sourceFile, const std::vector<uint32_t>& dataBlocks, uint64_t fileSize, uint32_t blockSize, uint64_t dataBlocksOffset);
        void extractFileData(const Inode& inode, const std::string& destinationPath, const Superblock& sb);
        void readDirectBlocks(std::ifstream& diskFile, const Inode& inode, std::ofstream& destFile, uint32_t blockSize, uint64_t& bytesRemaining, uint64_t dataBlocksOffset);
        void readIndirectBlocks(std::ifstream& diskFile, const Inode& inode, std::ofstream& destFile, uint32_t blockSize, uint64_t& bytesRemaining, uint64_t dataBlocksOffset);

        void addDirectoryEntry(uint32_t directoryInodeIndex, const std::string& name, uint32_t targetInodeIndex);
        void removeDirectoryEntry(uint32_t directoryInodeIndex, const std::string& name);
        int findInodeInDirectory(uint32_t directoryInodeIndex, const std::string& name);
        std::vector<DirectoryEntry> readDirectoryEntries(const Inode& directoryInode);

        int findInodeByPath(const std::string& path);
        void resolvePath(const std::string& path, uint32_t& parentInodeIndex, std::string& componentName);

        std::fstream openDiskStream(std::ios_base::openmode mode);
};