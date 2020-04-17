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

struct SuperBlock* superBlock = (struct SuperBlock*)malloc(sizeof(struct SuperBlock));
bool inodeBitmap[INODE_NUM];
bool blockBitmap[BLOCK_NUM];

char buffer[10000000] = {0};

int main() {
    //Init the virtual disk
    //Init();
    return 0;
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
    //format super block
    superBlock->s_inode_num = INODE_NUM;
    superBlock->s_block_num = BLOCK_NUM;
    superBlock->s_free_inode_num = INODE_NUM;
    superBlock->s_free_block_num = BLOCK_NUM;

    superBlock->s_block_size = BLOCK_SIZE;
    superBlock->s_inode_size = INODE_SIZE;
    superBlock->s_superblock_size = sizeof(SuperBlock);
    superBlock->s_blocks_per_group = BLOCK_PER_GROUP;
    
    superBlock->s_free_addr = blockStartAddr;
    superBlock->s_superblock_startaddr = superBlockStartAddr;
    superBlock->s_inodebitmap_startaddr = inodeBitmapStartAddr;
    superBlock->s_blockbitmap_startaddr = blockBitmapStartAddr;
    superBlock->s_inode_startaddr = inodeStartAddr;
    superBlock->s_block_startaddr = blockStartAddr;
    
    //format inode bitmap
    memset(inodeBitmap, 0, sizeof(inodeBitmap));
    fseek(fw, inodeBitmapStartAddr, SEEK_SET);
    fwrite(inodeBitmap, sizeof(inodeBitmap), 1, fw);
    
    //format block bitmap
    memset(blockBitmap, 0, sizeof(blockBitmap));
    fseek(fw, blockBitmapStartAddr, SEEK_SET);
    fwrite(blockBitmap, sizeof(blockBitmap), 1, fw);
    
    int i,j;
    //format block area
    for (i = BLOCK_NUM / BLOCKS_PER_GROUP - 1; i >= 0; i--) {
        if (i == BLOCK_NUM / BLOCKS_PER_GROUP - 1) {
            superBlock->s_free[0] = -1;
        }
        else {
            superBlock->s_free[0] = blockStartAddr + (i + 1) *  BLOCKS_PER_GROUP * BLOCK_SIZE;
        }
        for (j = 1; j < BLOCKS_PER_GROUP; j++) {
            superBlock->s_free[j] = blockStartAddr + (i * BLOCKS_PER_GROUP + j) * BLOCK_SIZE;
        }
        fseek(fw, blockStartAddr + i * BLOCK_PER_GROUP * BLOCK_SIZE, SEEK_SET);
        fwrite(superBlock->s_free, sizeof(superBlock->s_free), 1, fw);
    }
    
    fseek(fw, superBlockStartAddr, SEEK_SET);
    fwrite(superBlock, sizeof(SuperBlock), 1, fw);
    fflush(fw);
    
    //read inode bitmap
    fseek(fr, inodeBitmapStartAddr, SEEK_SET);
    fread(inodeBitmap, sizeof(inodeBitmap), fr);
    
    //read block bitmap
    fseek(fr, blockBitmapStartAddr, SEEK_SET);
    fread(blockBitmap, sizeof(blockBitmap), 1, fr);
    fflush(fr);
    
    //create root directory
    struct Inode* cur = (struct Inode*)malloc(sizeof(struct Inode));
    int inodeAddr = ialloc();
    int blockAddr = balloc();
    struct Dir dirList[16];
    strcpy(dirList->dirName, ".");
    dirList->inodeAddr = inodeAddr;
    
    fseek(fw, blockAddr, SEEK_SET);
    fwrite(dirList, sizeof(dirList), 1, fw);
    
    //assign the root inode
    cur->i_inode = 0;
    cur->i_mode = MODE_DIR | DIR_DEF_PERMISSION;
    cur->i_atime = time(NULL);
    cur->i_ctime = time(NULL);
    cur->i_mtime = time(NULL);
    strcpy(cur->i_uname, curUserName);
    strcpy(cur->i_gname, curGroupName);
    cur->i_count = 1;
    cur->i_dirBlock[0] = blockAddr;
    for (i = 1; i < 10; i++) {
        cur->i_dirBlock[i] = -1;
    }
    cur->i_size = superBlock->s_BLOCK_SIZE;
    cur->i_indirBlock_1 = -1;
    
    //write inode
    fseek(fw, inodeAddr, SEEK_SET);
    fwrite(cur, sizeof(struct Inode), 1, fw);
    
    //create directory and config file
    mkdir(rootDirAddr, "home");
    cd(rootDirAddr, "home");
    mkdir(curDirAddr, "root");
    
    cd(curDirAddr, "..");
    mkdir(curDirAddr, "etc");
    cd(curDirAddr, "etc");
    
    char buf[1000] = {0};
    sprintf(buf, "root:x:%d:%d\n", nextUID++, nextGID++); //format user:passwd:userID:groupID
    create(curDirAddr, "passwd", buf);
    sprintf(buf, "root:root\n"); 
    create(curDirAddr, "shadow", buf); //create user password file
    chmod(curDirAddr, "shadow", 0660);
    
    sprintf(buf, "root::0:root\n");
    sprintf(buf + strlen(buf), "user::1:\n");
    create(curDirAddr, "group", buf);
    
    cd(curDirAddr, "..");
    return 0;

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

int balloc() { //block allocate funciton
    int top;
    if (superBlock->s_free_addr == 0) {
        write("there is no enough block\n");
        return -1;
    }
}

void print(char* str) {
    write(1, str, strlen(str));
}

void Login() {
    write(1,"Login Interface\nUserName: ", "Login Interface\nUserName: ");

}
