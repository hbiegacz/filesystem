#include "HelperMethods.h"
#include <iostream>
#include <cstdint>

using namespace std;

void printOptions(){
    cout << " +-------------------------------------------------------+" << endl;
    cout << " |     Please choose one of the following actions:       |" << endl;
    cout << " |     1. Create virtual file system                     |" << endl;
    cout << " |     2. Copy file from physical disk                   |" << endl;
    cout << " |     3. Copy file from virtual disk                    |" << endl;
    cout << " |     4. Delete file from virtual disk                  |" << endl;
    cout << " |     5. Create a directory                             |" << endl;
    cout << " |     6. Remove a directory                             |" << endl;
    cout << " |     7. Display directory                              |" << endl;
    cout << " |     8. Display file system information                |" << endl;
    cout << " |     9. Add a link                                     |" << endl;
    cout << " |    10. Remove a link                                  |" << endl;
    cout << " |    11. Add bytes                                      |" << endl;
    cout << " |    12. Remove bytes                                   |" << endl;
    cout << " |    13. Exit                                           |" << endl;
    cout << " +-------------------------------------------------------+" << endl<<endl;
    cout << "Your choice: ";
}

int askForFileSystemSize(){
    int size;
    cout << "Please enter the size of the virtual file system in bytes. \nExample: 1024, 2048, 4096, 8192." << endl;
    cout << "Your input: ";
    cin >> size;
    cout << endl;
    return size;
}

string askForFileName(){
    string fileName;
    cout << "Please enter the name of the file to copy. Example: file.txt" << endl;
    cout << "Your input: ";
    cin >> fileName;
    cout << endl;
    return fileName;
}

string askForDirectoryPath() {
    string directoryPath;
    cout << "Please enter the path of the directory.\nExample: / (for root directory), your_directory_name or your_directory_name2/other_directory_name" << endl;
    cout << "Your input: ";
    cin >> directoryPath;
    cout << endl;
    return directoryPath;
}

string askForSourcePath() {
    string linkPath;
    cout << "Please enter the path for the file that the link will lead to.\nThe source of the link. Example: dir/link" << endl;
    cout << "Your input: ";
    cin >> linkPath;
    cout << endl;
    return linkPath;
}

string askForLinkPath() {
    string linkPath;
    cout << "Please enter the path to the link. \nExample: dir/link_name" << endl;
    cout << "Your input: ";
    cin >> linkPath;
    cout << endl;
    return linkPath;
}

uint64_t askForBytes() {
    uint64_t bytes;
    cout << "Please enter the number of bytes to add or remove. Example: 1024, 2048, 4096, 8192." << endl;
    cout << "Your input: ";
    cin >> bytes;
    cout << endl;
    return bytes;
}

string askForFilePath() {
    string filePath;
    cout << "Please enter the path to the file. \nExample: /file.txt or dir/another_file.txt" << endl;
    cout << "Your input: ";
    cin >> filePath;
    cout << endl;
    return filePath;
}

