#include "mono.h"

/*global variables declare*/
int inodeStartAddr;
int inodeStartAddr;
int superBlockStartAddr;
int inodeBitmapStartAddr; //occupy two blocks, monitor no more than 1024 inodes
int blockBitmapStartAddr; //20 blocks size, monitor no more than 10240 blocks
int inodeStartAddr; //inode area start address
int blockStartAddr; //block area start address
int maxFileSize;
int sumSize; //the size of the whole virtual disk

int rootDirAddr; //root directory address
int curDirAddr;
char curDirName[310];
char curHostName[110];
char curUserName[110];
char curGroupName[110];
char curUserDirName[310];

int nextUID; //next uid to allocate
int nextGID;


FILE* fw; //write file pointer
FILE* fr; //read file pointer

bool inodeBitmap[INODE_NUM];
bool blockBitmap[BLOCK_NUM];

//struct SuperBlock superBlock;

static char p1[100];
static char p2[100];
static char p3[100];
static char buf[1000];


int main() {
    //Init the virtual disk
    int res;
    if ((res = Init()) == -1) {
        printf("file system init failed\n");
        return -1;
    }

    //determines if the user has tmpFile directory
    res = access("tmpFile", F_OK);
    if (res == -1) {
        system("mkdir tmpFile");
    }

    char str[100];
    system("reset");
    while (1) {
        char *p;
        if ((p = strstr(curDirName, curUserDirName)) != NULL) {
            printf("[%s@%s %s]# ", curUserName, curHostName, curDirName);
        }
        else {
            printf("[%s@%s ~%s]# ", curUserName, curHostName, curDirName);
        }
        fgets(str, sizeof(str), stdin);
        cmd(str);
    }
    fclose(fw);
    fclose(fr);
    return 0;

}

int Init() {
    //printf("Mono File System initializing...\n");
    //sleep(2);
    superBlockStartAddr = 0;
    inodeBitmapStartAddr = superBlockStartAddr + 1 * BLOCK_SIZE;
    blockBitmapStartAddr = inodeBitmapStartAddr + 2 * BLOCK_SIZE;
    inodeStartAddr = blockBitmapStartAddr + 20 * BLOCK_SIZE;
    blockStartAddr = inodeStartAddr + INODE_NUM * INODE_SIZE;
    sumSize = blockStartAddr + BLOCK_NUM * BLOCK_SIZE;
    maxFileSize = 10 * BLOCK_SIZE + BLOCK_SIZE / sizeof(int) * BLOCK_SIZE;

    int res;

    if ((fr = fopen(FILESYSNAME, "rb")) == NULL) {
        printf("creating virtual disk file: mono.img...\n");
        sleep(1);
        fw = fopen(FILESYSNAME, "wb");
        if (fw == NULL) {
            printf("virtual disk create failed\n");
            return -1;
        }
        fr = fopen(FILESYSNAME, "rb");
        
        nextUID = 0;
        nextGID = 0;
        strcpy(curUserName, "root");
        strcpy(curGroupName, "root");
        strcpy(curHostName, "ubuntu");
        
        rootDirAddr = inodeStartAddr;
        curDirAddr = rootDirAddr;
        strcpy(curDirName, "/");
        
        printf("Mono File System formating\n");
        sleep(2);
        if ((res = Format()) == -1) {
            printf("disk format failed\n");
            return -1;
        }
        printf("Mono File System format done\n");
        sleep(0.5);
        printf("Mono File System installing\n");
        sleep(0.5);
        if ((res = Install()) == -1) {
            printf("file system install failed\n");
            return -1;
        }
        printf("Mono File system install done\n");
    }
    else {
        printf("reading virtual disk file: mono.img...\n");
        sleep(0.5);
        fr = fopen(FILESYSNAME, "rb");
        char* buffer = (char*)malloc(1024);
        FILE* tmp_fw = fopen(TMPFILENAME, "wb");
        FILE* tmp_fr = fopen(TMPFILENAME, "rb");
        if (tmp_fr == NULL || tmp_fw == NULL) {
            printf("temporary disk file open failed\n");
            return -1;
        }
        
        size_t size;
        while (1) {
            size = fread(buffer, sizeof(buffer), 1, fr);
            if (size <= 0) {
                break;
            }
            
            fwrite(buffer, sizeof(buffer), 1, tmp_fw);
            
        }
        
        fw = fopen(FILESYSNAME, "wb");
        if (fw == NULL) {
            printf("virtual disk open failed\n");
            return -1;
        }
        while (1) {
            size = fread(buffer, sizeof(buffer), 1, tmp_fr);
            if (size <= 0) {
                break;
            }
            
            fwrite(buffer, sizeof(buffer), 1, fw);
        }
        free(buffer);
        
        nextUID = 0;
        nextGID = 0;
        strcpy(curUserName, "root");
        strcpy(curGroupName, "root");
        
        strcpy(curHostName, "ubuntu");
        
        rootDirAddr = inodeStartAddr;
        curDirAddr = rootDirAddr;
        strcpy(curDirName, "/");
        printf("Mono File System installing\n");
        sleep(0.5);
        if ((res = Install()) == -1) {
            printf("file system install failed\n");
            return -1;
        }
    }
    return 0;    
}

int Format() { //When you first time install the file system, some format need to be done.
    //format super block
    if (&superBlock == NULL) {
        printf("super block not initialized\n");
        return -1;
    }
    superBlock.s_inode_num = INODE_NUM;
    superBlock.s_block_num = BLOCK_NUM;
    superBlock.s_free_inode_num = INODE_NUM;
    superBlock.s_free_block_num = BLOCK_NUM;

    superBlock.s_block_size = BLOCK_SIZE;
    superBlock.s_inode_size = INODE_SIZE;
    superBlock.s_superblock_size = sizeof(struct SuperBlock);
    superBlock.s_blocks_per_group = BLOCKS_PER_GROUP;
    
    superBlock.s_free_addr = blockStartAddr;
    superBlock.s_superblock_startaddr = superBlockStartAddr;
    superBlock.s_inodebitmap_startaddr = inodeBitmapStartAddr;
    superBlock.s_blockbitmap_startaddr = blockBitmapStartAddr;
    superBlock.s_inode_startaddr = inodeStartAddr;
    superBlock.s_block_startaddr = blockStartAddr;
    
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
            superBlock.s_free[0] = -1;
        }
        else {
            superBlock.s_free[0] = blockStartAddr + (i + 1) *  BLOCKS_PER_GROUP * BLOCK_SIZE;
        }
        for (j = 1; j < BLOCKS_PER_GROUP; j++) {
            superBlock.s_free[j] = blockStartAddr + (i * BLOCKS_PER_GROUP + j) * BLOCK_SIZE;
        }
        fseek(fw, blockStartAddr + i * BLOCKS_PER_GROUP * BLOCK_SIZE, SEEK_SET);
        fwrite(&superBlock.s_free, sizeof(superBlock.s_free), 1, fw);
    }
    
    fseek(fw, superBlockStartAddr, SEEK_SET);
    fwrite(&superBlock, sizeof(struct SuperBlock), 1, fw);
    fflush(fw);
    
    //read inode bitmap
    fseek(fr, inodeBitmapStartAddr, SEEK_SET);
    fread(inodeBitmap, sizeof(inodeBitmap), 1, fr);
    
    //read block bitmap
    fseek(fr, blockBitmapStartAddr, SEEK_SET);
    fread(blockBitmap, sizeof(blockBitmap), 1, fr);
    fflush(fr);
    
    //create root directory
    struct Inode curInode;
    struct Inode* cur = &curInode;
    int inodeAddr = ialloc();
    if (inodeAddr != inodeStartAddr) {
        printf("root inode addr not equal to inode start addr\n");
        return -1;
    }
    if (inodeAddr == -1) {
        printf("inode allocate failed\n");
        return -1;
    }
    int blockAddr = balloc();
    if (blockAddr == -1) {
        printf("block allocate failed\n");
        return -1;
    }
    struct Dir dirList[16] = {
        0
    };
    strcpy(dirList[0].dirName, ".");
    dirList[0].inodeAddr = inodeAddr;
    
    fseek(fw, blockAddr, SEEK_SET);
    fwrite(dirList, sizeof(dirList), 1, fw);
    
    //assign the root inode
    cur->i_inode = 0;
    cur->i_mode = MODE_DIR | DIR_DFT_PERMISSION;
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
    cur->i_size = superBlock.s_block_size;
    cur->i_indirBlock_1 = -1;
    cur->i_mode = MODE_DIR | DIR_DFT_PERMISSION;
    
    //write inode
    fseek(fw, inodeAddr, SEEK_SET);
    fwrite(cur, sizeof(struct Inode), 1, fw);
    fflush(fw);
    
    return 0;

}

int Install() {
    //read the super block
    fseek(fr, superBlockStartAddr, SEEK_SET);
    fread(&superBlock, sizeof(struct SuperBlock), 1, fr);
    
    //read the inode bitmap
    fseek(fr, inodeBitmapStartAddr, SEEK_SET);
    fread(inodeBitmap, sizeof(inodeBitmap), 1, fr);
    
    //read the block bitmap
    fseek(fr, blockBitmapStartAddr, SEEK_SET);
    fread(blockBitmap, sizeof(blockBitmap), 1, fr);
    fflush(fr);
    
    return 0;
}

void cmd(char str[]) {
    memset(p1, 0, sizeof(p1));
    memset(p2, 0, sizeof(p2));
    memset(p3, 0, sizeof(p3));
    int tmp = 0, i, res;
    sscanf(str, "%s", p1);
    
    if (strcmp(p1, "mkdir") == 0) {
        sscanf(str, "%s%s", p1, p2);
        mkDir(curDirAddr, p2);
    }
    else if (strcmp(p1, "rmdir") == 0) {
        sscanf(str, "%s%s", p1, p2);
        rmDir(curDirAddr, p2);
    }
    else if (strcmp(p1, "touch") == 0) {
        sscanf(str, "%s%s", p1, p2);
        char content[1];
        content[0] = '\0';
        if ((res = create(curDirAddr, p2, content)) == -1) {
            printf("touch file failed\n");
        }
    }
    else if (strcmp(p1, "rm") == 0) {
        sscanf(str, "%s%s", p1, p2);
        del(curDirAddr, p2);
    }
    else if (strcmp(p1, "ls") == 0) {
        sscanf(str, "%s%s", p1, p2);
        if (strcmp(p2, "") == 0) {
            list(curDirAddr);
        }
        else {
            //cd
        }
    }
    else if (strcmp(p1, "cd") == 0) {
        sscanf(str, "%s%s", p1, p2);
        if (strcmp(p2, "") == 0) {
            chRoot();
        }
        else {
            chDir(curDirAddr, p2);
        }
    }
    else if (strcmp(p1, "") == 0) {
        //do nothing
    }
    else if (strcmp(p1, "exit") == 0) {
        exit(0);
    }
    else {
        printf("command not found\n");
    }
}

