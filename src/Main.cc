/*
    Author: Hanna Biegacz

    This file implements a console interface for the file system manager.
    It provides a menu of options to the user and calls the appropriate functions.

*/

#include <iostream>
#include <cstdlib>
#include "FileSystemManager.h"
#include "HelperMethods.h"

using namespace std;

string FILE_SYSTEM_PATH = "../file_system.bin";

int main(){
    cout << " ---- WELCOME TO THE FILE SYSTEM MANAGER ----" << endl;
    FileSystemManager fsManager(FILE_SYSTEM_PATH);
    int choice;

    do {
        printOptions();
        cin >> choice;

        switch (choice){
            case 1:
                fsManager.createVirtualFileSystem(askForFileSystemSize());
                cout << "You can find it in the following location: " << FILE_SYSTEM_PATH << endl;
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