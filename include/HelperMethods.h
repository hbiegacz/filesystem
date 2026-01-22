#pragma once
#include <string>
#include <cstdint>

void printOptions();

int askForFileSystemSize();

std::string askForFileName();

std::string askForDirectoryPath();

std::string askForSourcePath();

std::string askForLinkPath();

std::string askForFilePath();

uint64_t askForBytes();