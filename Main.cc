/*
    Author: Hanna Biegacz

    This file implements a console interface for the file system manager.
    It provides a menu of options to the user and calls the appropriate functions.

*/

#include <iostream>
#include <cstdlib>
#include "FileSystemManager.h"
using namespace std;


void printOptions(){
    cout << "Please choose one of the following actions: " << endl;
    cout << "1. Create virtual file system" << endl;
    cout << "2. Copy file from physical disk" << endl;
    cout << "3. Copy file from virtual disk" << endl;
    cout << "4. Display catalogue" << endl;
    cout << "5. Display file system information" << endl;
    cout << "6. Display virtual disk status" << endl;
    cout << "7. Delete file from virtual disk" << endl;
    cout << "8. Delete virtual disk" << endl;
    cout << "9. Exit" << endl << endl;
    cout << "Your choice: ";
}

int main(){
    cout << " ---- WELCOME TO THE FILE SYSTEM MANAGER ----" << endl;
    FileSystemManager fsManager;
    int choice;

    do {
        printOptions();
        cin >> choice;

        switch (choice){
            case 1:
                // ASK FOR SIZE
                fsManager.createVirtualFileSystem(1024);
                break;
            case 2:
                // ASK FOR FILE NAME
                fsManager.copyFileFromPhysicalDisk("test.txt");
                break;
            case 3:
                // ASK FOR FILE NAME
                fsManager.copyFileFromVirtualDisk("test.txt");
                break;
            case 4:
                fsManager.displayCatalogue();
                break;
            case 5:
                fsManager.displayFileSystemInformation();
                break;
            case 6:
                fsManager.displayVirtualDiskStatus();
                break;
            case 7:
                // ASK FOR FILE NAME
                fsManager.deleteFileFromVirtualDisk("test.txt");
                break;
            case 8:
                fsManager.deleteVirtualDisk();
                break;
            case 9:
                cout << "Goodbye!" << endl;
                exit(0);
                break;
            default:
                cout << "Invalid choice" << endl;
                break;
        }
    } while (choice != 9);

    return 0;
}