#pragma once
#include <string>
#include <cstdint>
#include <fstream>
#include <vector>
#include "Structs.h"

class FileSystemManager{
    public:
        FileSystemManager(std::string virtual_disk_path, std::string output_path);
        ~FileSystemManager();

        void createVirtualFileSystem(uint64_t requestedDataSize);

        void copyFileFromPhysicalDisk(std::string file_name);
        void copyFileFromVirtualDisk(std::string file_name);
        void deleteFileFromVirtualDisk(std::string path); // TODO: take links into account
        
        void createDirectory(std::string directory_name);
        void deleteDirectory(std::string directory_name);
        void displayDirectory(std::string directory_name);
        
        void displayFileSystemInformation(); // TODO: display link info?
        void displayDiskUsage();

        void addLink(std::string source_path, std::string target_path);
        void removeLink(std::string file_path);

        void addBytes(std::string file_path, uint64_t bytes);
        void removeBytes(std::string file_path, uint64_t bytes);


    private:
        std::string virtual_disk_path_;
        std::string output_path_;
        
        std::fstream openDiskStream(std::ios_base::openmode mode);
        
        Superblock initializeSuperblock(uint32_t blockSize, uint32_t inodes, uint32_t blocks);
        void initializeSection(std::ostream& file, uint64_t offset, uint64_t size, bool isBitmap);
        void fillWithZeros(std::ostream& file, const char* data, uint64_t size);
        uint64_t alignToBlock(uint64_t offset, uint32_t blockSize);

        Superblock readSuperblock();
        void writeSuperblock(const Superblock& superblock);
        
        std::vector<uint8_t> readBitmap(uint64_t offset, uint32_t bitsCount);
        bool getBitmapBit(const std::vector<uint8_t>& bitmap, uint32_t bitIndex);
        void setBitmapBit(std::vector<uint8_t>& bitmap, uint32_t bitIndex, bool value);
        void writeBitmap(uint64_t offset, const std::vector<uint8_t>& bitmap);

        int findFreeInodeIndex(const std::vector<uint8_t>& inodeBitmap);
        int findInodeByPath(const std::string& path);
        int findInodeInDirectory(uint32_t directoryInodeIndex, const std::string& name);
        Inode readInode(uint32_t inodeIndex);
        void writeInode(uint32_t inodeIndex, const Inode& inode);

        void addDirectoryItem(uint32_t directoryInodeIndex, const std::string& name, uint32_t targetInodeIndex);
        void removeDirectoryItem(uint32_t directoryInodeIndex, const std::string& name);
        std::vector<DirectoryItem> readDirectoryItems(const Inode& directoryInode);

        std::pair<uint32_t, std::string> resolvePath(const std::string& path);
        uint64_t getSourceFileSize(std::ifstream& sourceFile);
        void validateFileSize(uint32_t requiredBlocks, uint32_t maxSupportedBlocks);
        uint32_t calculateRequiredBlocks(uint64_t fileSize, uint32_t blockSize);
        void extractFileData(const Inode& inode, const std::string& destinationPath, const Superblock& sb);

        std::vector<uint32_t> allocateDataBlocks(std::vector<uint8_t>& blockBitmap, uint32_t requiredBlocks, uint32_t blockSize);
        uint32_t allocateSingleBlock(Superblock& sb, std::vector<uint8_t>& blockBitmap);
        void deallocateInode(uint32_t inodeIndex, Inode& inode);
        std::vector<uint32_t> getInodeBlockIndices(const Inode& inode);
        void writeIndirectPointers(uint32_t indirectBlockIndex, const std::vector<uint32_t>& dataBlocks, uint32_t blockSize);
        void writeFileDataToBlocks(std::ifstream& sourceFile, const std::vector<uint32_t>& dataBlocks, uint64_t fileSize, uint32_t blockSize, uint64_t dataBlocksOffset);

        template <typename T>
        T readStruct(uint64_t offset);

        template <typename T>
        void writeStruct(uint64_t offset, const T& data);
};