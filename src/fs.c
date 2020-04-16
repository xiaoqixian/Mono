#include "fs.h"

const int superBlockStartAddr = 0;
const int inodeBitmapStartAddr = 1 * BLOCK_SIZE;
const int blockBitmapStartAddr = inodeBitmapStartAddr + 2 * BLOCK_SIZE;
const int inodeStartAddr = blockBitmapStartAddr + 20 * BLOCK_SIZE;
const int blockStartAddr = inodeStartAddr + INODE_NUM * INODE_SIZE;
const int sumSize = blockStartAddr + BLOCK_NUM * BLOCK_SIZE;

const int maxFileSize = 10 * BLOCK_SIZE + BLOCK_SIZE/sizeof(int) * BLOCK_SIZE;

int rootDirAddr;
int curDirAddr;
char curDirName[310];
char curHostName[110];
char curUserName[110];
char curGroupNmae[110];
char curUserDirName[310];

int nextUID, nextGID;
bool isLogin;

FILE* fw;
FILE* fr;

SuperBlock* superBlock;
bool inodeBitmap[INODE_NUM];
bool blockBitmap[BLOCK_NUM];

char buffer[10000000] = {0};

int main() {
    //Init the virtual disk
    Init();
}


int Init() {
    if ((fr = fopen(FILESYSNAME, "rb")) == NULL) {
        fw = fopen(FILESYSNAME, "wb");
        if (fw == NULL) {
            printf("virtual disk create failed\n");
            return -1;
        }
        fr = fopen(FILESYSNAME, "rb");
        
        nextUID = 0;
        nextGID = 0;
        isLogin = false;
        strcpy(curUserName, "root");
        strcpy(curGroupName, "root");
        strcpy(curHostName, "lunar");
        
        rootDirAddr = inodeStartAddr;
        curDirAddr = rootDirAddr;
        strcpy(curDirName, "/");
        
        printf("file system formating\n");
        if (Format() == -1) {
            printf("disk format failed\n");
            return -1;
        }
        printf("file system format done\n");
        printf("file system installing\n");
        if (Install() == -1) {
            printf("file system install failed\n");
            return -1;
        }
    }
}

int Format() { //When you first time install the file system, some format need to be done.


}

int Install() {
    //read the super block
    fseek(fr, superBlockStartAddr, SEEK_SET);
    fread(superBlock, sizeof(SuperBlock), 1, fr);
    
    //read the inode bitmap
    fseek(fr, inodeBitmapStartAddr, SEEK_SET);
    fread(inodeBitmap, sizeof(inodeBitmap), 1, fr);
    
    //read the block bitmap
    fseek(fr, blockBitmapStartAddr, SEEK_SET);
    fread(blockBitmap, sizeof(blockBitmap), 1, fr);
    
    return 0;
}

void Login() {
    write(1,"Login Interface\nUserName: ", "Login Interface\nUserName: ");

}
