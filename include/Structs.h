#pragma once

constexpr uint32_t INODE_DIRECT_POINTERS = 12;
constexpr uint32_t BITS_IN_BYTE = 8;
constexpr uint32_t MAX_FILENAME_LEN = 32;

// Describes the entire file system layout.
// Stores critical offsets to find other tables
// Tracks total free space.
struct Superblock {
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

// Describes a file or directory.
// Stores its data and direct/indirect block pointers.
struct Inode {
    long creationTime;
    uint64_t fileSizeInBytes;
    uint32_t dataBlockIndexes[INODE_DIRECT_POINTERS]; 
    uint32_t overflowBlockIndex;
    bool isDirectory;
    uint16_t linksCount;
};

// Describes a directory entry.
// Stores the name and index of the file or directory.
struct DirectoryItem {
    char fileName[MAX_FILENAME_LEN]; 
    uint32_t inodeIndex;
};