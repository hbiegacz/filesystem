#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include "FileSystemManager.h"

using namespace std;

FileSystemManager::FileSystemManager(string virtual_disk_path, string output_path): 
    virtual_disk_path_(virtual_disk_path), 
    output_path_(output_path) {
}

FileSystemManager::~FileSystemManager() {
}

void FileSystemManager::createVirtualFileSystem(uint64_t requestedDataSize) {
    std::cout << "Creating virtual file system..." << std::endl;

    const uint32_t BLOCK_SIZE = 4096;
    const uint32_t MAX_INODES = 1024;
    const uint32_t MAGIC_NUMBER = 0x53465350;

    uint32_t blocksCount = static_cast<uint32_t>((requestedDataSize + BLOCK_SIZE - 1) / BLOCK_SIZE);
    Superblock sb = initializeSuperblock(MAGIC_NUMBER, BLOCK_SIZE, MAX_INODES, blocksCount);
    
    std::ofstream diskFile(virtual_disk_path_, std::ios::binary);
    if (!diskFile) throw std::runtime_error("Could not create virtual disk file");

    diskFile.write(reinterpret_cast<const char*>(&sb), sizeof(Superblock));
    initializeSection(diskFile, sb.inodeTableOffset, static_cast<uint64_t>(MAX_INODES) * sizeof(Inode), false);    
    initializeSection(diskFile, sb.inodeBitmapOffset, (MAX_INODES + 7) / 8, true);          
    initializeSection(diskFile, sb.blockBitmapOffset, (blocksCount + 7) / 8, true);         
    initializeSection(diskFile, sb.dataBlocksOffset, static_cast<uint64_t>(blocksCount) * BLOCK_SIZE, false); 
    diskFile.close();

    Inode rootInode{};
    rootInode.creationTime = time(nullptr);
    rootInode.isDirectory = true;
    rootInode.hardLinkCount = 1;
    writeInode(0, rootInode);

    sb.freeInodesCount--;
    writeSuperblock(sb);
    setBitmapBit(sb.inodeBitmapOffset, 0, true);
    
    std::cout << "Filesystem created: " << blocksCount << " blocks, " 
              << MAX_INODES << " inodes, free: " 
              << sb.freeBlocksCount << " blocks, " 
              << sb.freeInodesCount << " inodes. (Root directory created at Inode 0)\n";
}


void FileSystemManager::copyFileFromPhysicalDisk(std::string fileName) {
    cout << "Copying file from " << fileName << " from physical to virtual disk..." << endl;

    std::ifstream sourceFile(fileName, std::ios::binary);
    if (!sourceFile) throw std::runtime_error("Could not open source file: " + fileName);

    uint64_t fileSize = getSourceFileSize(sourceFile);
    Superblock sb = readSuperblock();
    uint32_t requiredBlocks = calculateRequiredBlocks(fileSize, sb.blockSize);
    
    uint32_t ptrsPerBlock = sb.blockSize / sizeof(uint32_t);
    validateFileSize(requiredBlocks, 12 + ptrsPerBlock);

    std::vector<uint8_t> inodeBitmap = readBitmap(sb.inodeBitmapOffset, sb.inodesCount);
    std::vector<uint8_t> blockBitmap = readBitmap(sb.blockBitmapOffset, sb.blocksCount);

    int freeInodeIndex = findFreeInodeIndex(inodeBitmap);
    if (freeInodeIndex < 0) throw std::runtime_error("No free inodes available");

    std::vector<uint32_t> dataBlocks = allocateDataBlocks(blockBitmap, requiredBlocks, sb.blockSize);

    uint32_t indirectBlockIndex = 0;
    if (requiredBlocks > 12) indirectBlockIndex = allocateSingleBlock(sb, blockBitmap);

    Inode inode = createInodeForFile(fileSize, dataBlocks);
    inode.singlyIndirectPointer = indirectBlockIndex;

    sb.freeInodesCount--;
    sb.freeBlocksCount -= requiredBlocks;

    writeInode(static_cast<uint32_t>(freeInodeIndex), inode);
    setBitmapBit(sb.inodeBitmapOffset, freeInodeIndex, true);
    writeBitmap(sb.blockBitmapOffset, blockBitmap);
    writeSuperblock(sb);

    if (indirectBlockIndex > 0) writeIndirectPointers(indirectBlockIndex, dataBlocks, sb.blockSize);
    writeFileDataToBlocks(sourceFile, dataBlocks, fileSize, sb.blockSize, sb.dataBlocksOffset);

    std::string baseFileName = std::filesystem::path(fileName).filename().string();
    addDirectoryEntry(0, baseFileName, static_cast<uint32_t>(freeInodeIndex));

    cout << "File '" << fileName << "' (stored as '" << baseFileName << "') copied to virtual disk at Inode " << freeInodeIndex << endl;
}

void FileSystemManager::copyFileFromVirtualDisk(string file_name){
    cout << "Copying file '" << file_name << "' from virtual disk to physical disk..." << endl;

    Superblock sb = readSuperblock();
    
    int inodeIndex = findInodeInDirectory(0, file_name);
    if (inodeIndex < 0) {
        throw std::runtime_error("File '" + file_name + "' not found in virtual disk root directory.");
    }

    Inode inode = readInode(static_cast<uint32_t>(inodeIndex));
    if (inode.isDirectory) {
        throw std::runtime_error("'" + file_name + "' is a directory, cannot copy as a file.");
    }

    extractFileData(inode, file_name, sb);

    cout << "Successfully copied file '" << file_name << "' from virtual disk. You can find it in "<< this->output_path_ << endl;
}

void FileSystemManager::createDirectory(std::string path) {
    uint32_t parentInodeIndex;
    std::string dirName;
    resolvePath(path, parentInodeIndex, dirName);
    
    if (dirName.empty()) throw std::runtime_error("Invalid directory name");
    
    if (findInodeInDirectory(parentInodeIndex, dirName) != -1) {
        throw std::runtime_error("Directory or file already exists: " + dirName);
    }
    
    Superblock sb = readSuperblock();
    std::vector<uint8_t> inodeBitmap = readBitmap(sb.inodeBitmapOffset, sb.inodesCount);
    
    int freeInodeIndex = findFreeInodeIndex(inodeBitmap);
    if (freeInodeIndex < 0) throw std::runtime_error("No free inodes available");
    
    Inode newDirInode{};
    newDirInode.creationTime = time(nullptr);
    newDirInode.isDirectory = true;
    newDirInode.hardLinkCount = 1;
    
    sb.freeInodesCount--;
    
    writeInode(static_cast<uint32_t>(freeInodeIndex), newDirInode);
    setBitmapBit(sb.inodeBitmapOffset, freeInodeIndex, true);
    writeSuperblock(sb);
    
    addDirectoryEntry(parentInodeIndex, dirName, static_cast<uint32_t>(freeInodeIndex));
    std::cout << "Directory '" << dirName << "' created successfully." << std::endl;
}

void FileSystemManager::deleteDirectory(std::string path) {
    uint32_t parentInodeIndex;
    std::string dirName;
    resolvePath(path, parentInodeIndex, dirName);
    
    int inodeIndex = findInodeInDirectory(parentInodeIndex, dirName);
    if (inodeIndex < 0) throw std::runtime_error("Directory not found: " + dirName);
    
    Inode dirInode = readInode(static_cast<uint32_t>(inodeIndex));
    if (!dirInode.isDirectory) throw std::runtime_error("Not a directory: " + dirName);
    
    if (dirInode.fileSize > 0) throw std::runtime_error("Directory not empty: " + dirName);
    
    Superblock sb = readSuperblock();
    sb.freeInodesCount++;
    writeSuperblock(sb);
    setBitmapBit(sb.inodeBitmapOffset, inodeIndex, false);
    
    removeDirectoryEntry(parentInodeIndex, dirName);
    std::cout << "Directory '" << dirName << "' deleted successfully." << std::endl;
}

void FileSystemManager::displayDirectory(std::string path) {
    int inodeIndex = findInodeByPath(path);
    if (inodeIndex < 0) {
        std::cout << "Directory not found: " << path << std::endl;
        return;
    }
    
    Inode dirInode = readInode(static_cast<uint32_t>(inodeIndex));
    if (!dirInode.isDirectory) {
        std::cout << path << " is not a directory." << std::endl;
        return;
    }

    std::cout << "\n----------------- Directory Catalogue: " << path << " -----------------" << std::endl;

    try {
        vector<DirectoryEntry> entries = readDirectoryEntries(dirInode);
        
        if (entries.empty()) {
            cout << "(Directory is empty)" << endl;
        } else {
            cout << left << setw(32) << "Filename" << " | " << setw(10) << "Inode" << " | " << "Type\n";
            cout << string(32, '-') << "-+------------+--------------\n";
            for (const auto& entry : entries) {
                Inode entryInode = readInode(entry.inodeIndex);
                string type = entryInode.isDirectory ? "DIR" : "FILE";
                cout << left << setw(32) << entry.fileName << " | " << setw(10) << entry.inodeIndex << " | " << type << endl;
            }
        }
    } catch (const std::exception& e) {
        cout << "Error reading directory: " << e.what() << endl;
    }
    cout << "------------------------------------------------------------\n";
}

void FileSystemManager::displayFileSystemInformation(){
    cout << "\n--- File System Information ---" << endl;
    try {
        Superblock sb = readSuperblock();
        cout << "Magic Number:      0x" << std::hex << sb.magicNumber << std::dec << endl;
        cout << "Block Size:        " << sb.blockSize << " bytes" << endl;
        cout << "Total Inodes:      " << sb.inodesCount << endl;
        cout << "Free Inodes:       " << sb.freeInodesCount << endl;
        cout << "Total Data Blocks: " << sb.blocksCount << endl;
        cout << "Free Data Blocks:  " << sb.freeBlocksCount << endl;
        cout << "Offsets:" << endl;
        cout << "  Inode Table:     " << sb.inodeTableOffset << endl;
        cout << "  Inode Bitmap:    " << sb.inodeBitmapOffset << endl;
        cout << "  Block Bitmap:    " << sb.blockBitmapOffset << endl;
        cout << "  Data Blocks:     " << sb.dataBlocksOffset << endl;
    } catch (const std::exception& e) {
        cout << "Error reading filesystem info: " << e.what() << endl;
    }
    cout << "--------------------------------\n" << endl;
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

/* ---------------------------------------------------------------PRIVATE HELPER METHODS ------------------------------------------------------- */

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

Superblock FileSystemManager::readSuperblock() {
    auto diskFile = openDiskStream(std::ios::in);
    Superblock superblock{};
    diskFile.read(reinterpret_cast<char*>(&superblock), sizeof(Superblock));
    return superblock;
}

void FileSystemManager::writeSuperblock(const Superblock& superblock) {
    auto diskFile = openDiskStream(std::ios::in | std::ios::out);
    diskFile.seekp(0);
    diskFile.write(reinterpret_cast<const char*>(&superblock), sizeof(Superblock));
}

vector<uint8_t> FileSystemManager::readBitmap(uint64_t offset, uint32_t bitsCount) {
    uint64_t bytesCount = (bitsCount + 7) / 8;
    vector<uint8_t> bitmap(bytesCount, 0);

    auto diskFile = openDiskStream(std::ios::in);
    diskFile.seekg(static_cast<std::streamoff>(offset));
    diskFile.read(reinterpret_cast<char*>(bitmap.data()), static_cast<std::streamsize>(bytesCount));

    return bitmap;
}


void FileSystemManager::writeBitmap(uint64_t offset, const vector<uint8_t>& bitmap) {
    auto diskFile = openDiskStream(std::ios::in | std::ios::out);
    diskFile.seekp(static_cast<std::streamoff>(offset));
    diskFile.write(reinterpret_cast<const char*>(bitmap.data()), static_cast<std::streamsize>(bitmap.size()));
}

int FileSystemManager::findFreeInodeIndex(const vector<uint8_t>& inodeBitmap) {
    for (uint32_t index = 0; index < inodeBitmap.size() * 8; ++index) {
        uint32_t byteIndex = index / 8;
        uint32_t bitIndex = index % 8;

        if (byteIndex >= inodeBitmap.size()) {
            break;
        }

        bool bitSet = (inodeBitmap[byteIndex] & static_cast<uint8_t>(1u << bitIndex)) != 0;
        if (!bitSet) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

vector<uint32_t> FileSystemManager::allocateDataBlocks(
    vector<uint8_t>& blockBitmap,
    uint32_t requiredBlocks,
    uint32_t
) {
    vector<uint32_t> allocatedBlocks;
    allocatedBlocks.reserve(requiredBlocks);

    uint32_t totalBlocks = static_cast<uint32_t>(blockBitmap.size() * 8);

    for (uint32_t index = 0; index < totalBlocks && allocatedBlocks.size() < requiredBlocks; ++index) {
        uint32_t byteIndex = index / 8;
        uint32_t bitIndex = index % 8;

        if (byteIndex >= blockBitmap.size()) {
            break;
        }

        bool isUsed = (blockBitmap[byteIndex] & static_cast<uint8_t>(1u << bitIndex)) != 0;
        if (!isUsed) {
            blockBitmap[byteIndex] |= static_cast<uint8_t>(1u << bitIndex);
            allocatedBlocks.push_back(index);
        }
    }

    if (allocatedBlocks.size() != requiredBlocks) {
        throw std::runtime_error("Not enough free data blocks");
    }

    return allocatedBlocks;
}

Inode FileSystemManager::createInodeForFile(
    uint64_t fileSize,
    const vector<uint32_t>& dataBlocks
) {
    Inode inode{};
    inode.creationTime = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );
    inode.fileSize = fileSize;
    inode.isDirectory = false;
    inode.hardLinkCount = 1;

    std::fill(std::begin(inode.directPointers), std::end(inode.directPointers), 0);
    inode.singlyIndirectPointer = 0;

    for (size_t i = 0; i < dataBlocks.size() && i < 12; ++i) {
        inode.directPointers[i] = dataBlocks[i];
    }

    return inode;
}

void FileSystemManager::writeInode(uint32_t inodeIndex, const Inode& inode) {
    uint64_t inodeOffset = readSuperblock().inodeTableOffset + static_cast<uint64_t>(inodeIndex) * sizeof(Inode);
    auto diskFile = openDiskStream(std::ios::in | std::ios::out);
    diskFile.seekp(static_cast<std::streamoff>(inodeOffset));
    diskFile.write(reinterpret_cast<const char*>(&inode), sizeof(Inode));
}

Inode FileSystemManager::readInode(uint32_t inodeIndex) {
    uint64_t inodeOffset = readSuperblock().inodeTableOffset + static_cast<uint64_t>(inodeIndex) * sizeof(Inode);
    auto diskFile = openDiskStream(std::ios::in);
    diskFile.seekg(static_cast<std::streamoff>(inodeOffset));
    Inode inode{};
    diskFile.read(reinterpret_cast<char*>(&inode), sizeof(Inode));
    return inode;
}


void FileSystemManager::writeFileDataToBlocks(std::ifstream& sourceFile, const vector<uint32_t>& dataBlocks, 
                                               uint64_t fileSize, uint32_t blockSize, uint64_t dataBlocksOffset) {
    if (dataBlocks.empty()) return;
    vector<char> buffer(blockSize);
    uint64_t bytesRemaining = fileSize;
    auto diskFile = openDiskStream(std::ios::in | std::ios::out);

    for (uint32_t blockIndex : dataBlocks) {
        uint64_t writeSize = std::min<uint64_t>(blockSize, bytesRemaining);
        sourceFile.read(buffer.data(), static_cast<std::streamsize>(writeSize));
        
        uint64_t blockOffset = dataBlocksOffset + static_cast<uint64_t>(blockIndex) * blockSize;
        diskFile.seekp(static_cast<std::streamoff>(blockOffset));
        diskFile.write(buffer.data(), static_cast<std::streamsize>(writeSize));

        bytesRemaining -= writeSize;
        if (bytesRemaining == 0) break;
    }
}

void FileSystemManager::extractFileData(const Inode& inode, const std::string& fileName, const Superblock& sb) {
    if (!std::filesystem::exists(output_path_)) std::filesystem::create_directories(output_path_);
    std::filesystem::path destPath = std::filesystem::path(output_path_) / fileName;
    std::ofstream destFile(destPath, std::ios::binary);
    if (!destFile) throw std::runtime_error("Could not create physical file: " + destPath.string());

    uint64_t bytesRemaining = inode.fileSize;
    std::ifstream diskFile(virtual_disk_path_, std::ios::binary);
    if (!diskFile) throw std::runtime_error("Could not open virtual disk file for reading data");

    readDirectBlocks(diskFile, inode, destFile, sb.blockSize, bytesRemaining, sb.dataBlocksOffset);
    if (bytesRemaining > 0 && inode.singlyIndirectPointer > 0) {
        readIndirectBlocks(diskFile, inode, destFile, sb.blockSize, bytesRemaining, sb.dataBlocksOffset);
    }
}

void FileSystemManager::readDirectBlocks(std::ifstream& diskFile, const Inode& inode, std::ofstream& destFile, uint32_t blockSize, uint64_t& bytesRemaining, uint64_t dataBlocksOffset) {
    std::vector<char> buffer(blockSize);
    for (int i = 0; i < 12 && bytesRemaining > 0; ++i) {
        uint32_t blockIndex = inode.directPointers[i];
        uint64_t readSize = std::min<uint64_t>(blockSize, bytesRemaining);

        diskFile.seekg(static_cast<std::streamoff>(dataBlocksOffset + static_cast<uint64_t>(blockIndex) * blockSize));
        diskFile.read(buffer.data(), static_cast<std::streamsize>(readSize));
        destFile.write(buffer.data(), static_cast<std::streamsize>(readSize));
        bytesRemaining -= readSize;
    }
}

void FileSystemManager::readIndirectBlocks(std::ifstream& diskFile, const Inode& inode, std::ofstream& destFile, uint32_t blockSize, uint64_t& bytesRemaining, uint64_t dataBlocksOffset) {
    uint32_t ptrsPerBlock = blockSize / sizeof(uint32_t);
    std::vector<uint32_t> indirectPointers(ptrsPerBlock);
    
    diskFile.seekg(static_cast<std::streamoff>(dataBlocksOffset + static_cast<uint64_t>(inode.singlyIndirectPointer) * blockSize));
    diskFile.read(reinterpret_cast<char*>(indirectPointers.data()), blockSize);
    
    std::vector<char> buffer(blockSize);
    for (uint32_t i = 0; i < ptrsPerBlock && bytesRemaining > 0; ++i) {
        uint64_t readSize = std::min<uint64_t>(blockSize, bytesRemaining);
        diskFile.seekg(static_cast<std::streamoff>(dataBlocksOffset + static_cast<uint64_t>(indirectPointers[i]) * blockSize));
        diskFile.read(buffer.data(), static_cast<std::streamsize>(readSize));
        destFile.write(buffer.data(), static_cast<std::streamsize>(readSize));
        bytesRemaining -= readSize;
    }
}

void FileSystemManager::addDirectoryEntry(uint32_t directoryInodeIndex, const std::string& name, uint32_t targetInodeIndex) {
    Inode dirInode = readInode(directoryInodeIndex);
    if (!dirInode.isDirectory) throw std::runtime_error("Target inode is not a directory");

    DirectoryEntry newEntry{};
    size_t nameLen = std::min(name.length(), sizeof(newEntry.fileName) - 1);
    std::copy(name.begin(), name.begin() + nameLen, newEntry.fileName);
    newEntry.fileName[nameLen] = '\0';
    newEntry.inodeIndex = targetInodeIndex;

    Superblock sb = readSuperblock();
    uint32_t entryIndexInDir = static_cast<uint32_t>(dirInode.fileSize / sizeof(DirectoryEntry));
    uint32_t entriesPerBlock = sb.blockSize / sizeof(DirectoryEntry);
    uint32_t blockIdxInInode = entryIndexInDir / entriesPerBlock;
    uint32_t offsetInBlock = (entryIndexInDir % entriesPerBlock) * sizeof(DirectoryEntry);

    if (blockIdxInInode >= 12) throw std::runtime_error("Directory full");

    if (offsetInBlock == 0) {
        allocateDirectoryBlock(sb, dirInode, blockIdxInInode);
    } 

    uint32_t dataBlockIndex = dirInode.directPointers[blockIdxInInode];
    auto diskFile = openDiskStream(std::ios::in | std::ios::out);
    uint64_t entryOffset = sb.dataBlocksOffset + static_cast<uint64_t>(dataBlockIndex) * sb.blockSize + offsetInBlock;
    diskFile.seekp(static_cast<std::streamoff>(entryOffset));
    diskFile.write(reinterpret_cast<const char*>(&newEntry), sizeof(DirectoryEntry));
    
    dirInode.fileSize += sizeof(DirectoryEntry);
    writeInode(directoryInodeIndex, dirInode);
}

void FileSystemManager::allocateDirectoryBlock(Superblock& sb, Inode& dirInode, uint32_t blockIdxInInode) {
    std::vector<uint8_t> blockBitmap = readBitmap(sb.blockBitmapOffset, sb.blocksCount);
    dirInode.directPointers[blockIdxInInode] = allocateSingleBlock(sb, blockBitmap);
    writeBitmap(sb.blockBitmapOffset, blockBitmap);
    writeSuperblock(sb);
}

uint64_t FileSystemManager::getSourceFileSize(std::ifstream& sourceFile) {
    sourceFile.seekg(0, std::ios::end);
    uint64_t fileSize = static_cast<uint64_t>(sourceFile.tellg());
    sourceFile.seekg(0, std::ios::beg);
    return fileSize;
}

uint32_t FileSystemManager::calculateRequiredBlocks(uint64_t fileSize, uint32_t blockSize) {
    return static_cast<uint32_t>((fileSize + blockSize - 1) / blockSize);
}

void FileSystemManager::validateFileSize(uint32_t requiredBlocks, uint32_t maxSupportedBlocks) {
    if (requiredBlocks > maxSupportedBlocks) {
        throw std::runtime_error("File too large. Max supported blocks: " + std::to_string(maxSupportedBlocks));
    }
}


void FileSystemManager::writeIndirectPointers(uint32_t indirectBlockIndex, const std::vector<uint32_t>& dataBlocks, uint32_t blockSize) {
    uint32_t ptrsPerBlock = blockSize / sizeof(uint32_t);
    std::vector<uint32_t> indirectPointers(ptrsPerBlock, 0);
    for (size_t i = 12; i < dataBlocks.size(); ++i) {
        indirectPointers[i - 12] = dataBlocks[i];
    }
    
    auto diskFile = openDiskStream(std::ios::in | std::ios::out);
    Superblock sb = readSuperblock();
    uint64_t indirectOffset = sb.dataBlocksOffset + static_cast<uint64_t>(indirectBlockIndex) * blockSize;
    diskFile.seekp(static_cast<std::streamoff>(indirectOffset));
    diskFile.write(reinterpret_cast<const char*>(indirectPointers.data()), blockSize);
}

int FileSystemManager::findInodeInDirectory(uint32_t directoryInodeIndex, const std::string& name) {
    Inode dirInode = readInode(directoryInodeIndex);
    if (!dirInode.isDirectory) return -1;

    std::vector<DirectoryEntry> entries = readDirectoryEntries(dirInode);
    for (const auto& entry : entries) {
        if (name == entry.fileName) {
            return static_cast<int>(entry.inodeIndex);
        }
    }
    return -1;
}

std::vector<DirectoryEntry> FileSystemManager::readDirectoryEntries(const Inode& directoryInode) {
    std::vector<DirectoryEntry> entries;
    if (!directoryInode.isDirectory) return entries;

    Superblock sb = readSuperblock();
    uint32_t blockSize = sb.blockSize;
    uint32_t entriesTotal = static_cast<uint32_t>(directoryInode.fileSize / sizeof(DirectoryEntry));
    entries.reserve(entriesTotal);

    auto diskFile = openDiskStream(std::ios::in);
    uint32_t entriesRead = 0;

    for (int i = 0; i < 12 && entriesRead < entriesTotal; ++i) {
        uint32_t blockIndex = directoryInode.directPointers[i];
        if (blockIndex == 0 && i > 0) break; 

        uint64_t blockOffset = sb.dataBlocksOffset + static_cast<uint64_t>(blockIndex) * blockSize;
        diskFile.seekg(static_cast<std::streamoff>(blockOffset));

        uint32_t entriesInThisBlock = std::min(entriesTotal - entriesRead, blockSize / (uint32_t)sizeof(DirectoryEntry));
        for (uint32_t j = 0; j < entriesInThisBlock; ++j) {
            DirectoryEntry entry{};
            diskFile.read(reinterpret_cast<char*>(&entry), sizeof(DirectoryEntry));
            entries.push_back(entry);
            entriesRead++;
        }
    }
    return entries;
}


int FileSystemManager::findInodeByPath(const std::string& path) {
    if (path == "/" || path.empty()) return 0;
    
    uint32_t parentInode;
    std::string component;
    resolvePath(path, parentInode, component);
    
    return findInodeInDirectory(parentInode, component);
}

void FileSystemManager::resolvePath(const std::string& path, uint32_t& parentInodeIndex, std::string& componentName) {
    if (path.empty()) throw std::runtime_error("Empty path");
    
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string item;
    while (std::getline(ss, item, '/')) {
        if (!item.empty()) parts.push_back(item);
    }
    
    if (parts.empty()) {
        parentInodeIndex = 0; 
        componentName = "";
        return;
    }
    
    uint32_t currentInode = 0; 
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        int nextInode = findInodeInDirectory(currentInode, parts[i]);
        if (nextInode < 0) throw std::runtime_error("Path component not found: " + parts[i]);
        
        Inode inode = readInode(nextInode);
        if (!inode.isDirectory) throw std::runtime_error("Path component is not a directory: " + parts[i]);
        
        currentInode = static_cast<uint32_t>(nextInode);
    }
    
    parentInodeIndex = currentInode;
    componentName = parts.back();
}

void FileSystemManager::removeDirectoryEntry(uint32_t directoryInodeIndex, const std::string& name) {
    Inode dirInode = readInode(directoryInodeIndex);
    std::vector<DirectoryEntry> entries = readDirectoryEntries(dirInode);
    
    int indexToRemove = -1;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (name == entries[i].fileName) {
            indexToRemove = i;
            break;
        }
    }
    
    if (indexToRemove == -1) return;
    
    Superblock sb = readSuperblock();
    uint32_t entriesPerBlock = sb.blockSize / sizeof(DirectoryEntry);

    if (indexToRemove < static_cast<int>(entries.size()) - 1) {
        const DirectoryEntry& lastEntry = entries.back();
        
        uint32_t blockIdx = indexToRemove / entriesPerBlock;
        uint32_t offsetInBlock = (indexToRemove % entriesPerBlock) * sizeof(DirectoryEntry);
        uint32_t dataBlockIndex = dirInode.directPointers[blockIdx];
        
        uint64_t diskOffset = sb.dataBlocksOffset + static_cast<uint64_t>(dataBlockIndex) * sb.blockSize + offsetInBlock;
        
        auto diskFile = openDiskStream(std::ios::in | std::ios::out);
        diskFile.seekp(static_cast<std::streamoff>(diskOffset));
        diskFile.write(reinterpret_cast<const char*>(&lastEntry), sizeof(DirectoryEntry));
    }

    
    dirInode.fileSize -= sizeof(DirectoryEntry);
    writeInode(directoryInodeIndex, dirInode);
}

std::fstream FileSystemManager::openDiskStream(std::ios_base::openmode mode) {
    std::fstream stream(virtual_disk_path_, mode | std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Could not open virtual disk file: " + virtual_disk_path_);
    }
    return stream;
}

void FileSystemManager::setBitmapBit(uint64_t bitmapOffset, uint32_t index, bool value) {
    std::vector<uint8_t> bitmap = readBitmap(bitmapOffset, index + 1);
    uint32_t byteIndex = index / 8;
    uint32_t bitIndex = index % 8;
    
    if (value) {
        bitmap[byteIndex] |= static_cast<uint8_t>(1u << bitIndex);
    } else {
        bitmap[byteIndex] &= ~static_cast<uint8_t>(1u << bitIndex);
    }
    writeBitmap(bitmapOffset, bitmap);
}

uint32_t FileSystemManager::allocateSingleBlock(Superblock& sb, std::vector<uint8_t>& blockBitmap) {
    auto allocated = allocateDataBlocks(blockBitmap, 1, sb.blockSize);
    sb.freeBlocksCount--;
    return allocated[0];
}