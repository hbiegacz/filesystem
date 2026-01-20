#include "HelperMethods.h"
#include <iostream>

using namespace std;

void printOptions(){
    cout << "\n\n";
    cout << "     ---------------------------------------------------" << endl;
    cout << "      Please choose one of the following actions: " << endl;
    cout << "      1. Create virtual file system" << endl;
    cout << "      2. Copy file from physical disk" << endl;
    cout << "      3. Copy file from virtual disk" << endl;
    cout << "      4. Display catalogue" << endl;
    cout << "      5. Display file system information" << endl;
    cout << "      6. Display virtual disk status" << endl;
    cout << "      7. Delete file from virtual disk" << endl;
    cout << "      8. Delete virtual disk" << endl;
    cout << "      9. Exit" << endl << endl;
    cout << "Your choice: ";
}

int askForFileSystemSize(){
    int size;
    cout << "Please enter the size of the virtual file system in bytes. Example: 1024, 2048, 4096, 8192." << endl;
    cout << "Your input: ";
    cin >> size;
    cout << endl;
    return size;
}