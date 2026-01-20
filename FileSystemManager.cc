#include "FileSystemManager.h"
#include <iostream>
using namespace std;

FileSystemManager::FileSystemManager() {
    
}

FileSystemManager::~FileSystemManager() {
    
}

void FileSystemManager::createVirtualFileSystem(int size){
    cout << "Creating virtual file system" << endl;
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