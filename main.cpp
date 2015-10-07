#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include "getopt.h"
#define PATH_MAX_LEN 128

typedef int (__stdcall * FuncRtlAdjustPrivilege)(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN);

static BOOL gRecursion = FALSE;
static BOOL gScan      = FALSE;
static PSTR gObject    = NULL;
static PSTR gTarget    = NULL;
static std::vector<std::string> gList;
static struct option long_options[] = {
    { NULL,     no_argument,       NULL, 'r' },
    { "scan",   no_argument,       NULL, 's' },
    { "object", required_argument, NULL, 'o' },
    { "target", required_argument, NULL, 't' }
};

static void makePath(const char * dir, std::string &path);
static void ceFind(std::string &path, const char * relativePath= "\\");
static void ceReplace();
static void ceScan();

int main(int argc, char * argv[]) {
    char opt = 0;
    std::string path;
    HMODULE module;

    if ((module = LoadLibrary("ntdll.dll")) != NULL) {
        char currentDir[PATH_MAX_LEN] = { 0 };

        FuncRtlAdjustPrivilege RtlAdjustPrivilege = (FuncRtlAdjustPrivilege)GetProcAddress(module, "RtlAdjustPrivilege");
        if (RtlAdjustPrivilege != NULL) {
            BOOLEAN status;
            RtlAdjustPrivilege(19, TRUE, FALSE, &status);
        }

        GetCurrentDirectory(PATH_MAX_LEN, currentDir);
        makePath(currentDir, path);
        // printf("%s", path.c_str());

        while ((opt = getopt_long(argc, argv, "rso:t:", long_options, NULL)) != EOF) {
            switch (opt) {
                case 'o': gObject = optarg; break;
                case 'r': gRecursion = TRUE; break;
                case 's': gScan = TRUE; break;
                case 't': gTarget = optarg; break;
                default: exit(-1);
            }
        }
        // printf("%s %s %d %d", gObject, gTarget, gRecursion, gScan);
    }
    ceFind(path);
    ceReplace();

    return 0;
}

void makePath(const char * dir, std::string &path) {
    path.append(dir);
    path.append("\\*.*");
}

void ceFind(std::string &path, const char * relativePath/*= "\\"*/) {
    WIN32_FIND_DATA fileData;
    HANDLE findHandle = NULL;
    
    if ((findHandle = FindFirstFile(path.c_str(), &fileData)) != INVALID_HANDLE_VALUE) {
        while (TRUE) {
            if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && gRecursion) {
                if (fileData.cFileName[0] != '.') {
                    std::string childPath = path;
                    std::string subDir;

                    childPath.erase(childPath.end() - 3, childPath.end());
                    childPath.append(fileData.cFileName);
                    childPath.append("\\*.*");
                    subDir.append(relativePath);
                    subDir.append(fileData.cFileName);
                    subDir.append("\\");
                    ceFind(childPath, subDir.c_str());
                }
            } else {
                char * dot = strrchr(fileData.cFileName, '.');
                char * ext = strstr(dot, gObject);

                if (ext && ext > dot) {
                    std::string t;

                    t.append(relativePath);
                    t.append(fileData.cFileName);
                    gList.push_back(t);
                }
            }
            if (!FindNextFile(findHandle, &fileData)) {
                break;
            }
        }
    }
}

static void ceReplace() {
    char path[PATH_MAX_LEN] = { 0 };
    if (gScan)
        ceScan();

    GetCurrentDirectory(PATH_MAX_LEN, path);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    for (auto &item : gList) {
        std::string oldName = path;
        std::string newName;

        oldName.append(item);
        newName = oldName;
        newName.erase(newName.end() - (strlen(gObject)), newName.end());
        newName.append(gTarget);
        rename(oldName.c_str(), newName.c_str());
        if (gScan) {
            std::cout << item << "..." << "Done." << std::endl;
        }
        Sleep(80);
    }
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE);
}

static void ceScan() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    printf("Total of %d File(s) Were Found.\n", gList.size());
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
    printf("List...\n");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    for (auto &item : gList) {
        std::cout << item << std::endl;
    }
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::cout << "Start to Replace..." << std::endl;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE);
}