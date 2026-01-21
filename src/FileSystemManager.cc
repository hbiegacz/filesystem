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
    cout << "Creating virtual file system..." << endl;

    const uint32_t BLOCK_SIZE = 4096;
    const uint32_t MAX_INODES = 1024;
    const uint32_t MAGIC_NUMBER = 0x53465350;

    uint32_t blocksCount = static_cast<uint32_t>((requestedDataSize + BLOCK_SIZE - 1) / BLOCK_SIZE);
    Superblock sb = initializeSuperblock(MAGIC_NUMBER, BLOCK_SIZE, MAX_INODES, blocksCount);
    
    ofstream diskFile(virtual_disk_path_, ios::binary);
    if (!diskFile) throw runtime_error("Could not create virtual disk file");

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
    
    cout << "Filesystem created: " << blocksCount << " blocks, " 
              << MAX_INODES << " inodes, free: " 
              << sb.freeBlocksCount << " blocks, " 
              << sb.freeInodesCount << " inodes. (Root directory created at Inode 0)\n";
}

void FileSystemManager::copyFileFromPhysicalDisk(string fileName) {
    cout << "Copying " << fileName << "..." << endl;
    ifstream src(fileName, ios::binary);
    if (!src) throw runtime_error("Src fail: " + fileName);

    uint64_t size = getSourceFileSize(src);
    Superblock sb = readSuperblock();
    uint32_t req = calculateRequiredBlocks(size, sb.blockSize);
    
    vector<uint8_t> iMap = readBitmap(sb.inodeBitmapOffset, sb.inodesCount);
    vector<uint8_t> bMap = readBitmap(sb.blockBitmapOffset, sb.blocksCount);

    int iIdx = findFreeInodeIndex(iMap);
    if (iIdx < 0) throw runtime_error("No inodes");

    vector<uint32_t> data = allocateDataBlocks(bMap, req, sb.blockSize);
    uint32_t indir = (req > 12) ? allocateSingleBlock(sb, bMap) : 0;

    Inode inode{};
    inode.creationTime = time(nullptr);
    inode.fileSize = size;
    for (size_t i = 0; i < data.size() && i < 12; ++i) inode.directPointers[i] = data[i];
    inode.singlyIndirectPointer = indir;

    sb.freeInodesCount--;
    sb.freeBlocksCount -= req;
    writeInode(iIdx, inode);
    setBitmapBit(sb.inodeBitmapOffset, iIdx, true);
    writeBitmap(sb.blockBitmapOffset, bMap);
    writeSuperblock(sb);

    if (indir) writeIndirectPointers(indir, data, sb.blockSize);
    writeFileDataToBlocks(src, data, size, sb.blockSize, sb.dataBlocksOffset);
    addDirectoryEntry(0, filesystem::path(fileName).filename().string(), iIdx);
}

void FileSystemManager::copyFileFromVirtualDisk(string file_name){
    cout << "Copying file '" << file_name << "' from virtual disk to physical disk..." << endl;

    Superblock sb = readSuperblock();
    
    int inodeIndex = findInodeInDirectory(0, file_name);
    if (inodeIndex < 0) throw runtime_error("File '" + file_name + "' not found in virtual disk root directory.");

    Inode inode = readInode(static_cast<uint32_t>(inodeIndex));
    if (inode.isDirectory) throw runtime_error("'" + file_name + "' is a directory, cannot copy as a file.");
    extractFileData(inode, file_name, sb);

    cout << "Successfully copied file '" << file_name << "' from virtual disk. You can find it in "<< this->output_path_ << endl;
}

void FileSystemManager::createDirectory(string path) {
    uint32_t parentInodeIndex;
    string dirName;
    resolvePath(path, parentInodeIndex, dirName);
    
    if (dirName.empty()) throw runtime_error("Invalid directory name");
    if (findInodeInDirectory(parentInodeIndex, dirName) != -1) throw runtime_error("Directory or file already exists: " + dirName);
    
    Superblock sb = readSuperblock();
    vector<uint8_t> inodeBitmap = readBitmap(sb.inodeBitmapOffset, sb.inodesCount);
    
    int freeInodeIndex = findFreeInodeIndex(inodeBitmap);
    if (freeInodeIndex < 0) throw runtime_error("No free inodes available");
    
    Inode newDirInode{};
    newDirInode.creationTime = time(nullptr);
    newDirInode.isDirectory = true;
    newDirInode.hardLinkCount = 1;
    
    sb.freeInodesCount--;
    
    writeInode(static_cast<uint32_t>(freeInodeIndex), newDirInode);
    setBitmapBit(sb.inodeBitmapOffset, freeInodeIndex, true);
    writeSuperblock(sb);
    addDirectoryEntry(parentInodeIndex, dirName, static_cast<uint32_t>(freeInodeIndex));
    cout << "Directory '" << dirName << "' created successfully." << endl;
}

void FileSystemManager::deleteDirectory(string path) {
    uint32_t parentInodeIndex;
    string dirName;
    resolvePath(path, parentInodeIndex, dirName);
    
    int inodeIndex = findInodeInDirectory(parentInodeIndex, dirName);
    if (inodeIndex < 0) throw runtime_error("Directory not found: " + dirName);
    
    Inode dirInode = readInode(static_cast<uint32_t>(inodeIndex));
    if (!dirInode.isDirectory) throw runtime_error("Not a directory: " + dirName);
    if (dirInode.fileSize > 0) throw runtime_error("Directory not empty: " + dirName);
    
    Superblock sb = readSuperblock();
    sb.freeInodesCount++;
    writeSuperblock(sb);
    setBitmapBit(sb.inodeBitmapOffset, inodeIndex, false);
    
    removeDirectoryEntry(parentInodeIndex, dirName);
    cout << "Directory '" << dirName << "' deleted successfully." << endl;
}

void FileSystemManager::displayDirectory(string path) {
    int inodeIndex = findInodeByPath(path);
    if (inodeIndex < 0) {
        cout << "Directory not found: " << path << endl;
        return;
    }
    
    Inode dirInode = readInode(static_cast<uint32_t>(inodeIndex));
    if (!dirInode.isDirectory) {
        cout << path << " is not a directory." << endl;
        return;
    }

    cout << "\n----------------- Directory Catalogue: " << path << " -----------------" << endl;

    try {
        Superblock sb = readSuperblock();
        vector<DirectoryEntry> entries = readDirectoryEntries(dirInode);
        
        if (entries.empty()) {
            cout << "(Directory is empty)" << endl;
        } else {
            cout << left << setw(32) << "Filename" << " | " << setw(10) << "Inode" << " | " << setw(10) << "Size" << " | " << "Type\n";
            cout << string(32, '-') << "-+------------+------------+--------------\n";
            for (const auto& entry : entries) {
                Inode entryInode = readInode(entry.inodeIndex);
                string type = entryInode.isDirectory ? "DIR" : "FILE";
                cout << left << setw(32) << entry.fileName << " | " << setw(10) << entry.inodeIndex << " | " << setw(10) << entryInode.fileSize << " | " << type << endl;
            }
        }
        uint64_t freeSpace = static_cast<uint64_t>(sb.freeBlocksCount) * sb.blockSize;
        cout << "\nFree space: " << freeSpace << " bytes (" << sb.freeBlocksCount << " blocks)" << endl;
    } catch (const exception& e) {
        cout << "Error reading directory: " << e.what() << endl;
    }
    cout << "------------------------------------------------------------\n";
}

void FileSystemManager::displayFileSystemInformation(){
    cout << "\n--------------- File System Information --------------" << endl;

    try {
        Superblock sb = readSuperblock();
        cout << "Magic Number:      0x" << hex << sb.magicNumber << dec << endl;
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
    } catch (const exception& e) {
        cout << "Error reading filesystem info: " << e.what() << endl;
    }
    cout << "------------------------------------------------------------\n";
}

int FileSystemManager::deleteFileFromVirtualDisk(string path) {
    cout << "Deleting file from virtual disk" << endl;
    uint32_t pIdx, inIdx; string name;
    resolvePath(path, pIdx, name);
    inIdx = findInodeInDirectory(pIdx, name);
    if (inIdx < 0) throw runtime_error("File not found");

    Inode in = readInode(inIdx);
    if (in.isDirectory) throw runtime_error("Cannot delete directory as file");

    Superblock sb = readSuperblock();
    vector<uint8_t> bMap = readBitmap(sb.blockBitmapOffset, sb.blocksCount);
    
    auto freeB = [&](uint32_t& b) {
        if (b == 0) return;
        bMap[b / 8] &= ~(1u << (b % 8));
        sb.freeBlocksCount++;
        b = 0;
    };

    for (int i = 0; i < 12; ++i) freeB(in.directPointers[i]);
    if (in.singlyIndirectPointer) {
        vector<uint32_t> indir(sb.blockSize / 4);
        auto disk = openDiskStream(ios::in);
        disk.seekg(sb.dataBlocksOffset + (uint64_t)in.singlyIndirectPointer * sb.blockSize);
        disk.read((char*)indir.data(), sb.blockSize);
        for (auto b : indir) freeB(b);
        freeB(in.singlyIndirectPointer);
    }

    sb.freeInodesCount++;
    writeBitmap(sb.blockBitmapOffset, bMap);
    setBitmapBit(sb.inodeBitmapOffset, inIdx, false);
    writeSuperblock(sb);
    removeDirectoryEntry(pIdx, name);
    
    cout << "Deleted " << name << endl;
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
void FileSystemManager::initializeSection(ofstream& file, uint64_t offset, uint64_t size, bool isBitmap) {
    file.seekp(offset);
    vector<char> buffer(1024, 0); 
    
    if (!isBitmap) {
        writeBuffer(file, buffer.data(), size);
        return;
    }
    
    writeBuffer(file, buffer.data(), size);
}

// Copies 1024-byte zero buffer repeatedly until exact number of bytes is written
void FileSystemManager::writeBuffer(ofstream& file, const char* data, uint64_t size) {
    uint64_t bytesWritten = 0;
    const uint64_t BUFFER_SIZE = 1024;
    while (bytesWritten < size) {
        uint64_t toWrite = min(BUFFER_SIZE, size - bytesWritten);
        file.write(data, toWrite);
        bytesWritten += toWrite;
    }
}

Superblock FileSystemManager::readSuperblock() { return readStruct<Superblock>(0); }
void FileSystemManager::writeSuperblock(const Superblock& sb) { writeStruct<Superblock>(0, sb); }

template <typename T>
T FileSystemManager::readStruct(uint64_t offset) {
    T data;
    auto disk = openDiskStream(ios::in);
    disk.seekg(static_cast<streamoff>(offset));
    disk.read(reinterpret_cast<char*>(&data), sizeof(T));
    return data;
}

template <typename T>
void FileSystemManager::writeStruct(uint64_t offset, const T& data) {
    auto disk = openDiskStream(ios::in | ios::out);
    disk.seekp(static_cast<streamoff>(offset));
    disk.write(reinterpret_cast<const char*>(&data), sizeof(T));
}

// Explicitly instantiate for used types
template Inode FileSystemManager::readStruct<Inode>(uint64_t);
template void FileSystemManager::writeStruct<Inode>(uint64_t, const Inode&);
template Superblock FileSystemManager::readStruct<Superblock>(uint64_t);
template void FileSystemManager::writeStruct<Superblock>(uint64_t, const Superblock&);

vector<uint8_t> FileSystemManager::readBitmap(uint64_t offset, uint32_t bitsCount) {
    uint64_t bytesCount = (bitsCount + 7) / 8;
    vector<uint8_t> bitmap(bytesCount, 0);

    auto diskFile = openDiskStream(ios::in);
    diskFile.seekg(static_cast<streamoff>(offset));
    diskFile.read(reinterpret_cast<char*>(bitmap.data()), static_cast<streamsize>(bytesCount));

    return bitmap;
}


void FileSystemManager::writeBitmap(uint64_t offset, const vector<uint8_t>& bitmap) {
    auto diskFile = openDiskStream(ios::in | ios::out);
    diskFile.seekp(static_cast<streamoff>(offset));
    diskFile.write(reinterpret_cast<const char*>(bitmap.data()), static_cast<streamsize>(bitmap.size()));
}

int FileSystemManager::findFreeInodeIndex(const vector<uint8_t>& bitmap) {
    for (uint32_t i = 0; i < bitmap.size() * 8; ++i) {
        if (!(bitmap[i / 8] & (1u << (i % 8)))) return i;
    }
    return -1;
}

vector<uint32_t> FileSystemManager::allocateDataBlocks(vector<uint8_t>& bMap, uint32_t req, uint32_t) {
    vector<uint32_t> blocks;
    for (uint32_t i = 0; i < bMap.size() * 8 && blocks.size() < req; ++i) {
        if (!(bMap[i / 8] & (1u << (i % 8)))) {
            bMap[i / 8] |= (1u << (i % 8));
            blocks.push_back(i);
        }
    }
    if (blocks.size() != req) throw runtime_error("No space");
    return blocks;
}


void FileSystemManager::writeInode(uint32_t idx, const Inode& in) {
    writeStruct<Inode>(readSuperblock().inodeTableOffset + static_cast<uint64_t>(idx) * sizeof(Inode), in);
}

Inode FileSystemManager::readInode(uint32_t idx) {
    return readStruct<Inode>(readSuperblock().inodeTableOffset + static_cast<uint64_t>(idx) * sizeof(Inode));
}


void FileSystemManager::writeFileDataToBlocks(ifstream& sourceFile, const vector<uint32_t>& dataBlocks, 
                                               uint64_t fileSize, uint32_t blockSize, uint64_t dataBlocksOffset) {
    if (dataBlocks.empty()) return;
    vector<char> buffer(blockSize);
    uint64_t bytesRemaining = fileSize;
    auto diskFile = openDiskStream(ios::in | ios::out);

    for (uint32_t blockIndex : dataBlocks) {
        uint64_t writeSize = min<uint64_t>(blockSize, bytesRemaining);
        sourceFile.read(buffer.data(), static_cast<streamsize>(writeSize));
        
        uint64_t blockOffset = dataBlocksOffset + static_cast<uint64_t>(blockIndex) * blockSize;
        diskFile.seekp(static_cast<streamoff>(blockOffset));
        diskFile.write(buffer.data(), static_cast<streamsize>(writeSize));

        bytesRemaining -= writeSize;
        if (bytesRemaining == 0) break;
    }
}

void FileSystemManager::extractFileData(const Inode& in, const string& name, const Superblock& sb) {
    if (!filesystem::exists(output_path_)) filesystem::create_directories(output_path_);
    ofstream dest(filesystem::path(output_path_) / name, ios::binary);
    ifstream disk(virtual_disk_path_, ios::binary);
    
    uint64_t rem = in.fileSize;
    auto readB = [&](uint32_t bIdx) {
        if (!rem || bIdx == 0) return;
        uint64_t len = min<uint64_t>(sb.blockSize, rem);
        vector<char> buf(len);
        disk.seekg(sb.dataBlocksOffset + (uint64_t)bIdx * sb.blockSize);
        disk.read(buf.data(), len);
        dest.write(buf.data(), len);
        rem -= len;
    };

    for (int i = 0; i < 12; ++i) readB(in.directPointers[i]);
    if (rem > 0 && in.singlyIndirectPointer > 0) {
        vector<uint32_t> indir(sb.blockSize / 4);
        disk.seekg(sb.dataBlocksOffset + (uint64_t)in.singlyIndirectPointer * sb.blockSize);
        disk.read((char*)indir.data(), sb.blockSize);
        for (auto b : indir) readB(b);
    }
}


void FileSystemManager::addDirectoryEntry(uint32_t directoryInodeIndex, const string& name, uint32_t targetInodeIndex) {
    Inode dirInode = readInode(directoryInodeIndex);
    if (!dirInode.isDirectory) throw runtime_error("Target inode is not a directory");

    DirectoryEntry newEntry{};
    size_t nameLen = min(name.length(), sizeof(newEntry.fileName) - 1);
    copy(name.begin(), name.begin() + nameLen, newEntry.fileName);
    newEntry.fileName[nameLen] = '\0';
    newEntry.inodeIndex = targetInodeIndex;

    Superblock sb = readSuperblock();
    uint32_t entryIndexInDir = static_cast<uint32_t>(dirInode.fileSize / sizeof(DirectoryEntry));
    uint32_t entriesPerBlock = sb.blockSize / sizeof(DirectoryEntry);
    uint32_t blockIdxInInode = entryIndexInDir / entriesPerBlock;
    uint32_t offsetInBlock = (entryIndexInDir % entriesPerBlock) * sizeof(DirectoryEntry);

    if (blockIdxInInode >= 12) throw runtime_error("Directory full");

    if (offsetInBlock == 0) {
        vector<uint8_t> blockBitmap = readBitmap(sb.blockBitmapOffset, sb.blocksCount);
        dirInode.directPointers[blockIdxInInode] = allocateSingleBlock(sb, blockBitmap);
        writeBitmap(sb.blockBitmapOffset, blockBitmap);
        writeSuperblock(sb);
    } 

    uint32_t dataBlockIndex = dirInode.directPointers[blockIdxInInode];
    auto diskFile = openDiskStream(ios::in | ios::out);
    uint64_t entryOffset = sb.dataBlocksOffset + static_cast<uint64_t>(dataBlockIndex) * sb.blockSize + offsetInBlock;
    diskFile.seekp(static_cast<streamoff>(entryOffset));
    diskFile.write(reinterpret_cast<const char*>(&newEntry), sizeof(DirectoryEntry));
    
    dirInode.fileSize += sizeof(DirectoryEntry);
    writeInode(directoryInodeIndex, dirInode);
}


uint64_t FileSystemManager::getSourceFileSize(ifstream& sourceFile) {
    sourceFile.seekg(0, ios::end);
    uint64_t fileSize = static_cast<uint64_t>(sourceFile.tellg());
    sourceFile.seekg(0, ios::beg);
    return fileSize;
}

uint32_t FileSystemManager::calculateRequiredBlocks(uint64_t fileSize, uint32_t blockSize) {
    return static_cast<uint32_t>((fileSize + blockSize - 1) / blockSize);
}

void FileSystemManager::validateFileSize(uint32_t requiredBlocks, uint32_t maxSupportedBlocks) {
    if (requiredBlocks > maxSupportedBlocks) {
        throw runtime_error("File too large. Max supported blocks: " + to_string(maxSupportedBlocks));
    }
}


void FileSystemManager::writeIndirectPointers(uint32_t indirectBlockIndex, const vector<uint32_t>& dataBlocks, uint32_t blockSize) {
    uint32_t ptrsPerBlock = blockSize / sizeof(uint32_t);
    vector<uint32_t> indirectPointers(ptrsPerBlock, 0);
    for (size_t i = 12; i < dataBlocks.size(); ++i) {
        indirectPointers[i - 12] = dataBlocks[i];
    }
    
    auto diskFile = openDiskStream(ios::in | ios::out);
    Superblock sb = readSuperblock();
    uint64_t indirectOffset = sb.dataBlocksOffset + static_cast<uint64_t>(indirectBlockIndex) * blockSize;
    diskFile.seekp(static_cast<streamoff>(indirectOffset));
    diskFile.write(reinterpret_cast<const char*>(indirectPointers.data()), blockSize);
}

int FileSystemManager::findInodeInDirectory(uint32_t directoryInodeIndex, const string& name) {
    Inode dirInode = readInode(directoryInodeIndex);
    if (!dirInode.isDirectory) return -1;

    vector<DirectoryEntry> entries = readDirectoryEntries(dirInode);
    for (const auto& entry : entries) {
        if (name == entry.fileName) {
            return static_cast<int>(entry.inodeIndex);
        }
    }
    return -1;
}

vector<DirectoryEntry> FileSystemManager::readDirectoryEntries(const Inode& directoryInode) {
    vector<DirectoryEntry> entries;
    if (!directoryInode.isDirectory) return entries;

    Superblock sb = readSuperblock();
    uint32_t blockSize = sb.blockSize;
    uint32_t entriesTotal = static_cast<uint32_t>(directoryInode.fileSize / sizeof(DirectoryEntry));
    entries.reserve(entriesTotal);

    auto diskFile = openDiskStream(ios::in);
    uint32_t entriesRead = 0;

    for (int i = 0; i < 12 && entriesRead < entriesTotal; ++i) {
        uint32_t blockIndex = directoryInode.directPointers[i];
        if (blockIndex == 0 && i > 0) break; 

        uint64_t blockOffset = sb.dataBlocksOffset + static_cast<uint64_t>(blockIndex) * blockSize;
        diskFile.seekg(static_cast<streamoff>(blockOffset));

        uint32_t entriesInThisBlock = min(entriesTotal - entriesRead, blockSize / (uint32_t)sizeof(DirectoryEntry));
        for (uint32_t j = 0; j < entriesInThisBlock; ++j) {
            DirectoryEntry entry{};
            diskFile.read(reinterpret_cast<char*>(&entry), sizeof(DirectoryEntry));
            entries.push_back(entry);
            entriesRead++;
        }
    }
    return entries;
}


int FileSystemManager::findInodeByPath(const string& path) {
    if (path == "/" || path.empty()) return 0;
    
    uint32_t parentInode;
    string component;
    resolvePath(path, parentInode, component);
    
    return findInodeInDirectory(parentInode, component);
}

void FileSystemManager::resolvePath(const string& path, uint32_t& parentInodeIndex, string& componentName) {
    if (path.empty()) throw runtime_error("Empty path");
    
    vector<string> parts;
    stringstream ss(path);
    string item;
    while (getline(ss, item, '/')) {
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
        if (nextInode < 0) throw runtime_error("Path component not found: " + parts[i]);
        
        Inode inode = readInode(nextInode);
        if (!inode.isDirectory) throw runtime_error("Path component is not a directory: " + parts[i]);
        
        currentInode = static_cast<uint32_t>(nextInode);
    }
    
    parentInodeIndex = currentInode;
    componentName = parts.back();
}

void FileSystemManager::removeDirectoryEntry(uint32_t directoryInodeIndex, const string& name) {
    Inode dirInode = readInode(directoryInodeIndex);
    vector<DirectoryEntry> entries = readDirectoryEntries(dirInode);
    
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
        
        auto diskFile = openDiskStream(ios::in | ios::out);
        diskFile.seekp(static_cast<streamoff>(diskOffset));
        diskFile.write(reinterpret_cast<const char*>(&lastEntry), sizeof(DirectoryEntry));
    }

    
    dirInode.fileSize -= sizeof(DirectoryEntry);
    writeInode(directoryInodeIndex, dirInode);
}

fstream FileSystemManager::openDiskStream(ios_base::openmode mode) {
    fstream stream(virtual_disk_path_, mode | ios::binary);
    if (!stream) {
        throw runtime_error("Could not open virtual disk file: " + virtual_disk_path_);
    }
    return stream;
}

void FileSystemManager::setBitmapBit(uint64_t offset, uint32_t idx, bool val) {
    vector<uint8_t> map = readBitmap(offset, idx + 1);
    if (val) map[idx / 8] |= (1u << (idx % 8));
    else     map[idx / 8] &= ~(1u << (idx % 8));
    writeBitmap(offset, map);
}

uint32_t FileSystemManager::allocateSingleBlock(Superblock& sb, vector<uint8_t>& blockBitmap) {
    auto allocated = allocateDataBlocks(blockBitmap, 1, sb.blockSize);
    sb.freeBlocksCount--;
    return allocated[0];
}