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
string FILE_SYSTEM_OUTPUT_PATH = "../output";

int main(){
    cout << " +-------------------------------------------------------+" << endl;
    cout << " |          WELCOME TO THE FILE SYSTEM MANAGER           |" << endl;
    cout << " +-------------------------------------------------------+" << endl;

    FileSystemManager fsManager(FILE_SYSTEM_PATH, FILE_SYSTEM_OUTPUT_PATH);
    int choice;

    do {
        printOptions();
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Invalid input. Please enter a number." << endl;
            continue;
        }

        try {
            switch (choice){
                case 1:
                    fsManager.createVirtualFileSystem(askForFileSystemSize());
                    cout << "You can find it in the following location: " << FILE_SYSTEM_PATH << endl;
                    break;
                case 2:
                    fsManager.copyFileFromPhysicalDisk(askForFileName());
                    break;
                case 3:
                    fsManager.copyFileFromVirtualDisk(askForFileName());
                    break;

                case 4:
                    fsManager.deleteFileFromVirtualDisk(askForFileName());
                    break;
                case 5:
                    fsManager.createDirectory(askForDirectoryPath());
                    break;
                case 6:
                    fsManager.deleteDirectory(askForDirectoryPath());
                    break;
                case 7:
                    fsManager.displayDirectory(askForDirectoryPath());
                    break;
                case 8:
                    fsManager.displayFileSystemInformation();
                    break;
                case 9:
                    fsManager.addLink(askForSourcePath(), askForLinkPath());
                    break;
                case 10:
                    fsManager.removeLink(askForLinkPath());
                    break;
                case 11:
                    fsManager.addBytes(askForFilePath(), askForBytes());
                    break;
                case 12:
                    fsManager.removeBytes(askForFilePath(), askForBytes());
                    break;
                case 13:
                    cout << "Goodbye!" << endl;
                    exit(0);
                    break;
                default:
                    cout << "Invalid choice" << endl;
                    break;
            }
        } catch (const std::exception& e) {
            cerr << "\n[ERROR] An error occurred: " << e.what() << endl;
            cout << "Returning to main menu..." << endl;
        }
    } while (choice != 13);

    return 0;
}