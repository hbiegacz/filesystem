#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include "FileSystemManager.h"

class FileSystemCreationTest : public ::testing::Test {
protected:
    std::string testDiskPath = "test_virtual_disk.bin";
    std::string testOutputPath = "test_output";
    FileSystemManager fsManager;

    FileSystemCreationTest() : fsManager(testDiskPath, testOutputPath) {}

    void SetUp() override {
        if (std::filesystem::exists(testDiskPath)) {
            std::filesystem::remove(testDiskPath);
        }
    }

    void TearDown() override {
        if (std::filesystem::exists(testDiskPath)) {
            std::filesystem::remove(testDiskPath);
        }
    }
};

TEST_F(FileSystemCreationTest, CreatesFileOnDisk) {
    fsManager.createVirtualFileSystem(1024 * 1024);
    ASSERT_TRUE(std::filesystem::exists(testDiskPath));
}

TEST_F(FileSystemCreationTest, FileHasCorrectSize) {
    uint64_t expectedSize = 1024 * 1024;
    fsManager.createVirtualFileSystem(expectedSize);
    uintmax_t actualSize = std::filesystem::file_size(testDiskPath);
    ASSERT_GE(actualSize, expectedSize);
}

TEST_F(FileSystemCreationTest, SuperblockHasCorrectMagicNumber) {
    fsManager.createVirtualFileSystem(1024 * 1024);
    
    std::ifstream diskFile(testDiskPath, std::ios::binary);
    uint32_t magicNumber;
    diskFile.read(reinterpret_cast<char*>(&magicNumber), sizeof(uint32_t));
    
    ASSERT_EQ(magicNumber, 0x53465350u);
}

TEST_F(FileSystemCreationTest, SuperblockFieldsAreCorrect) {
    fsManager.createVirtualFileSystem(1024 * 1024);
    
    std::ifstream diskFile(testDiskPath, std::ios::binary);
    Superblock sb;
    diskFile.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));
    
    ASSERT_EQ(sb.magicNumber, 0x53465350u);
    ASSERT_EQ(sb.blockSize, 4096u);
    ASSERT_EQ(sb.inodesCount, 1024u);
    ASSERT_EQ(sb.freeInodesCount, 1023u);
}

TEST_F(FileSystemCreationTest, InodeTableIsZeroFilled) {
    fsManager.createVirtualFileSystem(1024 * 1024);
    
    std::ifstream diskFile(testDiskPath, std::ios::binary);
    Superblock sb;
    diskFile.seekg(0);
    diskFile.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));
    
    diskFile.seekg(sb.inodeTableOffset + sizeof(Inode));
    Inode testInode;
    diskFile.read(reinterpret_cast<char*>(&testInode), sizeof(Inode));
    
    ASSERT_EQ(testInode.creationTime, 0);
    ASSERT_EQ(testInode.fileSize, 0);
    ASSERT_FALSE(testInode.isDirectory);
    ASSERT_EQ(testInode.hardLinkCount, 0);
}

TEST_F(FileSystemCreationTest, InodeBitmapIsZeroFilled) {
    fsManager.createVirtualFileSystem(1024 * 1024);
    
    std::ifstream diskFile(testDiskPath, std::ios::binary);
    Superblock sb;
    diskFile.seekg(0);
    diskFile.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));
    
    diskFile.seekg(sb.inodeBitmapOffset);
    uint8_t bitmapByte;
    diskFile.read(reinterpret_cast<char*>(&bitmapByte), 1);
    
    ASSERT_EQ(bitmapByte, 1);
}

TEST_F(FileSystemCreationTest, BlockBitmapIsZeroFilled) {
    fsManager.createVirtualFileSystem(1024 * 1024);
    
    std::ifstream diskFile(testDiskPath, std::ios::binary);
    Superblock sb;
    diskFile.seekg(0);
    diskFile.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));
    
    diskFile.seekg(sb.blockBitmapOffset);
    uint8_t bitmapByte;
    diskFile.read(reinterpret_cast<char*>(&bitmapByte), 1);
    
    ASSERT_EQ(bitmapByte, 0);
}

TEST_F(FileSystemCreationTest, DataBlocksAreZeroFilled) {
    fsManager.createVirtualFileSystem(1024 * 1024);
    
    std::ifstream diskFile(testDiskPath, std::ios::binary);
    Superblock sb;
    diskFile.seekg(0);
    diskFile.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));
    
    diskFile.seekg(sb.dataBlocksOffset);
    uint8_t dataByte;
    diskFile.read(reinterpret_cast<char*>(&dataByte), 1);
    
    ASSERT_EQ(dataByte, 0);
}

TEST_F(FileSystemCreationTest, HandlesSmallDiskSize) {
    fsManager.createVirtualFileSystem(4096);
    ASSERT_TRUE(std::filesystem::exists(testDiskPath));
}

TEST_F(FileSystemCreationTest, ThrowsOnInvalidPath) {
    FileSystemManager invalidFs("/invalid/path/test.bin", testOutputPath);
    ASSERT_THROW(invalidFs.createVirtualFileSystem(1024 * 1024), std::runtime_error);
}

TEST_F(FileSystemCreationTest, CalculatesCorrectBlockCount) {
    fsManager.createVirtualFileSystem(4096);
    std::ifstream diskFile(testDiskPath, std::ios::binary);
    Superblock sb;
    diskFile.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));
    ASSERT_EQ(sb.blocksCount, 1u);
    ASSERT_EQ(sb.freeBlocksCount, 1u);
}

TEST_F(FileSystemCreationTest, CalculatesCorrectBlockCountLargerSize) {
    fsManager.createVirtualFileSystem(8192);
    std::ifstream diskFile(testDiskPath, std::ios::binary);
    Superblock sb;
    diskFile.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));
    ASSERT_EQ(sb.blocksCount, 2u);
    ASSERT_EQ(sb.freeBlocksCount, 2u);
}
class DirectoryOperationTest : public FileSystemCreationTest {
protected:
    void SetUp() override {
        FileSystemCreationTest::SetUp();
        fsManager.createVirtualFileSystem(1024 * 1024);
    }
};

TEST_F(DirectoryOperationTest, CreateNestedDirectory) {
    ASSERT_NO_THROW(fsManager.createDirectory("dir1"));
    ASSERT_NO_THROW(fsManager.createDirectory("dir1/dir2"));
}

TEST_F(DirectoryOperationTest, DeleteNestedDirectory) {
    fsManager.createDirectory("dir1");
    fsManager.createDirectory("dir1/dir2");
    ASSERT_NO_THROW(fsManager.deleteDirectory("dir1/dir2"));
}

TEST_F(DirectoryOperationTest, ThrowsOnNonEmptyDelete) {
    fsManager.createDirectory("dir1");
    fsManager.createDirectory("dir1/dir2");
    ASSERT_THROW(fsManager.deleteDirectory("dir1"), std::runtime_error);
}

TEST_F(DirectoryOperationTest, ThrowsOnCreatingExisting) {
    fsManager.createDirectory("dir1");
    ASSERT_THROW(fsManager.createDirectory("dir1"), std::runtime_error);
}

TEST_F(DirectoryOperationTest, ThrowsOnInvalidPath) {
    ASSERT_THROW(fsManager.createDirectory("nonexistent/dir"), std::runtime_error);
}
