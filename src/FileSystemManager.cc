#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include "FileSystemManager.h"

using namespace std;

FileSystemManager::FileSystemManager(string virtual_disk_path): virtual_disk_path_(virtual_disk_path) {
}

FileSystemManager::~FileSystemManager() {
}

void FileSystemManager::createVirtualFileSystem(uint64_t requestedDataSize) {
    std::cout << "Creating virtual file system" << std::endl;

    const uint32_t BLOCK_SIZE = 4096;
    const uint32_t MAX_INODES = 1024;
    const uint32_t MAGIC_NUMBER = 0x53465350;

    uint32_t blocksCount = static_cast<uint32_t>((requestedDataSize + BLOCK_SIZE - 1) / BLOCK_SIZE);
    Superblock sb = initializeSuperblock(MAGIC_NUMBER, BLOCK_SIZE, MAX_INODES, blocksCount);
    
    std::ofstream diskFile(virtual_disk_path_, std::ios::binary);
    if (!diskFile) throw std::runtime_error("Could not create virtual disk file");

    // Superblock
    diskFile.write(reinterpret_cast<const char*>(&sb), sizeof(Superblock));
    initializeSection(diskFile, sb.inodeTableOffset, MAX_INODES * sizeof(Inode), false);    
    initializeSection(diskFile, sb.inodeBitmapOffset, (MAX_INODES + 7) / 8, true);          
    initializeSection(diskFile, sb.blockBitmapOffset, (blocksCount + 7) / 8, true);         
    initializeSection(diskFile, sb.dataBlocksOffset, static_cast<uint64_t>(blocksCount) * BLOCK_SIZE, false); 
    diskFile.close();
    
    std::cout << "Filesystem created: " << blocksCount << " blocks, " 
              << MAX_INODES << " inodes, free: " 
              << sb.freeBlocksCount << " blocks, " 
              << sb.freeInodesCount << " inodes.\n";
}


void FileSystemManager::copyFileFromPhysicalDisk(string file_name){
    cout << "Copying file from physical disk" << endl;
}

void FileSystemManager::copyFileFromVirtualDisk(string file_name){
    cout << "Copying file from virtual disk" << endl;
}

void FileSystemManager::displayCatalogue(){
    cout << "Displaying catalogue" << endl;
}

void FileSystemManager::displayFileSystemInformation(){
    cout << "Displaying file system information" << endl;
}

void FileSystemManager::displayVirtualDiskStatus(){
    cout << "Displaying virtual disk status" << endl;
}

int FileSystemManager::deleteFileFromVirtualDisk(string file_name){
    cout << "Deleting file from virtual disk" << endl;
    return 0;
}

int FileSystemManager::deleteVirtualDisk(){
    cout << "Deleting virtual disk" << endl;
    return 0;
}

// PRIVATE HELPER METHODS

// Plans where each filesystem section starts on disk with proper block alignment
Superblock FileSystemManager::initializeSuperblock(uint32_t magic, uint32_t blockSize, uint32_t inodes, uint32_t blocks) {
    Superblock sb{};
    sb.magicNumber = magic;
    sb.blockSize = blockSize;
    sb.inodesCount = inodes;
    sb.blocksCount = blocks;
    sb.freeInodesCount = inodes;
    sb.freeBlocksCount = blocks;

    uint64_t currentOffset = sizeof(Superblock);
    currentOffset = alignToBlock(currentOffset, blockSize);
    sb.inodeTableOffset = currentOffset;
    
    currentOffset += static_cast<uint64_t>(inodes) * sizeof(Inode);
    currentOffset = alignToBlock(currentOffset, blockSize);
    sb.inodeBitmapOffset = currentOffset;
    
    uint64_t inodeBitmapSize = (inodes + 7) / 8;
    currentOffset += inodeBitmapSize;
    currentOffset = alignToBlock(currentOffset, blockSize);
    sb.blockBitmapOffset = currentOffset;
    
    uint64_t blockBitmapSize = (blocks + 7) / 8;
    currentOffset += blockBitmapSize;
    sb.dataBlocksOffset = alignToBlock(currentOffset, blockSize);

    return sb;
}

// Moves offset forward to start at next full block boundary
uint64_t FileSystemManager::alignToBlock(uint64_t offset, uint32_t blockSize) {
    return ((offset + blockSize - 1) / blockSize) * blockSize;
}

// Fills specific disk area with zeros from exact byte position for given length
void FileSystemManager::initializeSection(std::ofstream& file, uint64_t offset, uint64_t size, bool isBitmap) {
    file.seekp(offset);
    std::vector<char> buffer(1024, 0); 
    
    if (!isBitmap) {
        writeBuffer(file, buffer.data(), size);
        return;
    }
    
    writeBuffer(file, buffer.data(), size);
}

// Copies 1024-byte zero buffer repeatedly until exact number of bytes is written
void FileSystemManager::writeBuffer(std::ofstream& file, const char* data, uint64_t size) {
    uint64_t bytesWritten = 0;
    const uint64_t BUFFER_SIZE = 1024;
    while (bytesWritten < size) {
        uint64_t toWrite = std::min(BUFFER_SIZE, size - bytesWritten);
        file.write(data, toWrite);
        bytesWritten += toWrite;
    }
}