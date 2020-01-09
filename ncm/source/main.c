#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <switch.h>

typedef struct{
    u16 numExtraDataBytes;
    u16 numContentRecords;
    u16 numMetaRecords;
    u16 padding;
} NcmContentRecordListingHeader;

void printBuffer(void* buffer, size_t size){
    u8* b = (u8*)buffer;
    for(size_t i=0; i<size; i++){
        printf("0x%02X ", b[i]);
    }
    printf("\n");
}

void printInstallType(NcmContentInstallType installType){
    if(installType == NcmContentInstallType_FragmentOnly){
        printf("FragmentOnly\n");
    }else if(installType == NcmContentInstallType_Full){
        printf("Full\n");
    }else if(installType == NcmContentInstallType_Unknown){
        printf("Unknown\n");
    }
}

void changeInstallType(NcmContentInstallType* installType){
    if(*installType == NcmContentInstallType_FragmentOnly){
        *installType = NcmContentInstallType_Full;
    }else if(*installType == NcmContentInstallType_Full){
        *installType = NcmContentInstallType_Unknown;
    }else if(*installType == NcmContentInstallType_Unknown){
        *installType = NcmContentInstallType_FragmentOnly;
    }
}

void printMetaType(NcmContentMetaType metaType){
    if(metaType == NcmContentMetaType_AddOnContent){
        printf("AddOnContent\n");
    }else if(metaType == NcmContentMetaType_Application){
        printf("Application\n");
    }else if(metaType == NcmContentMetaType_BootImagePackage){
        printf("BootImagePackage\n");
    }else if(metaType == NcmContentMetaType_BootImagePackageSafe){
        printf("BootImagePackageSafe\n");
    }else if(metaType == NcmContentMetaType_Delta){
        printf("Delta\n");
    }else if(metaType == NcmContentMetaType_Patch){
        printf("Patch\n");
    }else if(metaType == NcmContentMetaType_SystemData){
        printf("SystemData\n");
    }else if(metaType == NcmContentMetaType_SystemProgram){
        printf("SystemProgram\n");
    }else if(metaType == NcmContentMetaType_SystemUpdate){
        printf("SystemUpdate\n");
    }else if(metaType == NcmContentMetaType_Unknown){
        printf("Unknown\n");
    }
}

void changeMetaType(NcmContentMetaType* metaType){
    if(*metaType == NcmContentMetaType_AddOnContent){
        *metaType = NcmContentMetaType_Application;
    }else if(*metaType == NcmContentMetaType_Application){
        *metaType = NcmContentMetaType_BootImagePackage;
    }else if(*metaType == NcmContentMetaType_BootImagePackage){
        *metaType = NcmContentMetaType_BootImagePackageSafe;
    }else if(*metaType == NcmContentMetaType_BootImagePackageSafe){
        *metaType = NcmContentMetaType_Delta;
    }else if(*metaType == NcmContentMetaType_Delta){
        *metaType = NcmContentMetaType_Patch;
    }else if(*metaType == NcmContentMetaType_Patch){
        *metaType = NcmContentMetaType_SystemData;
    }else if(*metaType == NcmContentMetaType_SystemData){
        *metaType = NcmContentMetaType_SystemProgram;
    }else if(*metaType == NcmContentMetaType_SystemProgram){
        *metaType = NcmContentMetaType_SystemUpdate;
    }else if(*metaType == NcmContentMetaType_SystemUpdate){
        *metaType = NcmContentMetaType_Unknown;
    }else if(*metaType == NcmContentMetaType_Unknown){
        *metaType = NcmContentMetaType_AddOnContent;
    }
}

void printContentType(NcmContentType contentType){
    if(contentType == NcmContentType_Meta){
        printf("Meta\n");
    }else if(contentType == NcmContentType_Program){
        printf("Program\n");
    }else if(contentType == NcmContentType_Data){
        printf("Data\n");
    }else if(contentType == NcmContentType_Control){
        printf("Control\n");
    }else if(contentType == NcmContentType_HtmlDocument){
        printf("HtmlDocument\n");
    }else if(contentType == NcmContentType_LegalInformation){
        printf("LegalInformation\n");
    }else if(contentType == NcmContentType_DeltaFragment){
        printf("DeltaFragment\n");
    }
}

void printContentRecord(NcmContentInfo record){
    printf("ContentID: ");
    for(u8 i=0;i<0x10;i++){
        printf("%02X", record.content_id.c[i]);
    }
    printf("\n");

    u64 contentSize = 0;
    memcpy(&contentSize, &record.content_id.c, 6);

    printf("Content size: %lu bytes\n", contentSize);

    printf("Content type: ");
    printContentType(record.content_type);

    printf("Content index: %u\n", record.id_offset);
}

void listContentMeta(NcmContentMetaType* metaType){
    ncmInitialize();

    NcmContentMetaDatabase metaDb;
    Result rc = ncmOpenContentMetaDatabase(&metaDb, NcmStorageId_SdCard);
    if(R_FAILED(rc)){
        printf("Cannot open content meta database");
        return;
    }

    s32 entriesOut, entriesWritten;

    NcmContentMetaKey metaKeys[20];

    rc = ncmContentMetaDatabaseList(&metaDb, &entriesOut, &entriesWritten, metaKeys,
        sizeof(metaKeys) / sizeof(NcmContentMetaKey), *metaType,
        0, 0, U64_MAX, NcmContentInstallType_Full);
    
    if(R_FAILED(rc)){
        printf("Cannot query content meta database");
        ncmContentMetaDatabaseClose(&metaDb);
        return;
    }

    for(s32 i=0; i<entriesWritten; i++){
        NcmContentMetaKey* metaKey = &metaKeys[i];
        printf("#%d\n", i);
        printf("Id: 0x%016lX\n", metaKey->id);
        printf("Type: ");
        printMetaType(metaKey->type);
        printf("Type install: ");
        printInstallType(metaKey->install_type);
        printf("Version: %d\n", metaKey->version);

        u64 contentRecordSize;
        rc = ncmContentMetaDatabaseGetSize(&metaDb, &contentRecordSize, metaKey);
        if(R_FAILED(rc)){
            printf("Cannot get content record size");
            ncmContentMetaDatabaseClose(&metaDb);
            return;
        }

        printf("Content record size: %lu\n", contentRecordSize);

        u64 outData;
        void* buffer = malloc(contentRecordSize);
        rc = ncmContentMetaDatabaseGet(&metaDb, metaKey, &outData, buffer, contentRecordSize);
        if(R_FAILED(rc)){
            printf("Cannot get content records from 0x%016lX\n", metaKey->id);
            continue;
        }

        NcmContentRecordListingHeader* header = (NcmContentRecordListingHeader*)buffer;

        printf("Extra bytes: %u bytes\n", header->numExtraDataBytes);

        printBuffer(buffer + sizeof(NcmContentRecordListingHeader), header->numExtraDataBytes);

        NcmContentInfo* record = (NcmContentInfo*)(buffer + sizeof(NcmContentRecordListingHeader) + header->numExtraDataBytes);
        
        for(u16 z=0; z<header->numContentRecords; z++){
            printf("Content record %u\n", z);
            printContentRecord(record[z]);
        }

        printf("\n");

        free(buffer);
    }

    ncmContentMetaDatabaseClose(&metaDb);
    ncmExit();
}

void menuLoop(NcmContentMetaType* metaTypeForQuery, NcmContentInstallType* installTypeForQuery, u64 keysDown){

    if (keysDown & KEY_R){
        changeMetaType(metaTypeForQuery);
    }else if (keysDown & KEY_L){
        changeInstallType(installTypeForQuery);
    }

    if(keysDown != 0){
        consoleClear();
        printf("NCM Content Meta Database querier\n");
        printf("Type: ");
        printMetaType(*metaTypeForQuery);
        printf("Press R to change\n\n");

        printf("Install type: ");
        printInstallType(*installTypeForQuery);
        printf("Press L to change\n\n");

        printf("Press A to perform the query\n");
    }
}

int main(int argc, char **argv)
{
    consoleInit(NULL);

    socketInitializeDefault();

    nxlinkStdio();

    bool inMenu = true;
    NcmContentMetaType metaTypeForQuery = NcmContentMetaType_AddOnContent;
    NcmContentInstallType installTypeForQuery = NcmContentInstallType_FragmentOnly;

    while(appletMainLoop())
    {
        
        hidScanInput();

        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if(inMenu){
            menuLoop(&metaTypeForQuery, &installTypeForQuery, kDown);
            if (kDown & KEY_A) {
                inMenu = false;
                consoleClear();
                listContentMeta(&metaTypeForQuery);
                
            }
        }else{
            if (kDown & KEY_PLUS) break; // break in order to return to hbmenu
        }

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
