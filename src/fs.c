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
        print("there is no enough block\n");
        return -1;
    }
    else {
        top = (superBlock->s_free_block_num - 1) % superBlock->s_blocks_per_group;
    }
    int retAddr;
    
    if (top == 0) {
        retAddr = superBlock->s_free_addr;
        superBlock->s_free_addr = superBlock->s_free[0];
        fseek(fr, superBlock->s_free_addr, SEEK_SET);
        fread(superBlock->s_free, sizeof(superBlock->s_free), 1, fr);
        fflush(fr);
        
        superBlock->s_free_block_num--;
    }
    else {
        retAddr = superBlock->s_free[top];
        superBlock->s_free[top] = -1;
        top--;
        superBlock->s_free_block_num--;
    }
    
    //update super block
    fseek(fw, superBlockStartAddr, SEEK_SET);
    fwrite(superBlock, sizeof(superBlock), 1, fw);
    fflush(fw);
    
    //update block bitmap
    fseek(fw, (retAddr - blockStartAddr) / BLOCK_SIZE + blockBitmapStartAddr, SEEK_SET);
    fwrite(&blockBitmap[(retAddr - blockStartAddr) / BLOCK_SIZE], sizeof(bool), 1, fw);
    fflush(fw);

    return retAddr;
}

//block free function
int bfree(int addr) {
    if (addr < blockStartAddr || (addr - blockStartAddr) % superBlock->s_block_size != 0) {
        print("function bfree(): invalid block address\n");
        return -1;
    }
    unsigned int bno = (addr - blockStartAddr) % superBlock->s_block_size; //block number
    if (blockBitmap[bno] == 0) {
        print("function bfree(): this is an unused block, not allowed to free\n");
        return -1;
    }
    
    int top;
    if (superBlock->s_free_block_num == superBlock->s_block_num) {
        print("function bfree(): no non-idle blocks, not allowed to free\n");
        return -1;
    }
    else {
        top = (superBlock->s_free_block_num - 1) % superBlock->s_blocks_per_group;
        char tmp[BLOCK_SIZE] = {0}; 
        fseek(fw, addr, SEEK_SET);
        fwrite(tmp, sizeof(tmp), 1, fw);
        free(tmp); //always remember to free up unwanted memory

        if (top == superBlock->s_blocks_per_group - 1) {
            superBlock->s_free[0] = superBlock->s_free_addr;
            superBlock->s_free_addr = addr; // I add this line
            int i;
            for (int i = 1;i < superBlock->s_blocks_per_group; i++) {
                superBlock->s_free[i] = -1;
            }
            fseek(fw, addr, SEEK_SET);
            fwrite(superBlock->s_free, sizeof(superBlock->s_free), 1, fw);
        }
        else {
            top++;
            superBlock->s_free[top] = addr;
        }
    }
    
    superBlock->s_free_block_num++;
    fseek(fw, superBlockStartAddr, SEEK_SET);
    fwrite(superBlock, sizeof(superBlock), 1, fw);
    
    //update block bitmap
    blockBitmap[bno] = 0;
    fseek(fw, bno + blockBitmapStartAddr, SEEK_SET);
    fwrite(&blockBitmap[bno], sizeof(bool), 1, fw);
    fflush(fw);
    
    return 0;

}

int ialloc() { //inode allocate function
    if (superBlock->s_free_inode_num == 0) {
        print("no enough inode\n");
        return -1;
    }
    else {
        int i;
        for (i = 0; i < superBlock->s_inode_num;i++) {
            if (inodeBitmap[i] == 0) {
                break;
            }
        }
        
        //update super block
        superBlock->s_free_inode_num--;
        fseek(fw, superBlockStartAddr, SEEK_SET);
        fwrite(superBlock, sizeof(superBlock), 1 ,fw);
        
        //update inode bitmap
        inodeBitmap[i] = 1;
        fseek(fw, inodeBitmapStartAddr + i, SEEK_SET);
        fwrite(&inodeBitmap[i], sizeof(bool), 1, fw);
        fflush(fw);
        
        return inodeStartAddr + i * superBlock->s_inode_size;
    }
}

//inode free function
int ifree(int addr) {
    if (addr < inodeStartAddr || (addr - inodeStartAddr) % superBlock->s_inode_size != 0) {
        print("function ifree(): invalid inode address\n");
        return -1;
    }
    unsigned short ino = (addr - inodeStartAddr) % superBlock->s_inode_size;
    if (inodeBitmap[ino] == 0) {
        print("function ifree(): the inode is unused, not allowed to free\n");
        return -1;
    }
    
    //clear inode
    char tmp[INODE_SIZE] = {0}; 
    fseek(fw, addr, SEEK_SET);
    fwrite(tmp, sizeof(tmp), 1, fw);
    free(tmp); 
    
    //update super block
    superBlock->s_free_inode_num++;
    fseek(fw, superBlockStartAddr, SEEK_SET);
    fwrite(superBlock, sizeof(superBlock), 1, fw);
    
    //update inode bitmap
    inodeBitmap[ino] = 0;
    fseek(fw, inodeBitmapStartAddr + ino, SEEK_SET);
    fwrite(&inodeBitmap[ino], sizeof(bool), 1, fw);
    fflush(fw);

    return 0;
}

int mkdir(int parinoAddr, char name[]) {
    if (strlen(name) >= MAX_NAME_SIZE) {
        print("function mkdir(): directory name too long\n");
        return -1;
    }
    Dir dirlist[16];
    
    struct Inode* cur;
    fseek(fr, parinoAddr, SEEK_SET);
    fread(cur, sizeof(cur), 1, fr);
    
    int i = 0, count = cur->i_count + 1, posi = -1, posj = -1;
    while (i < 160) {
        //in 160 directories, directly search in direct blocks
        int dno = i / 16;
        
        if (cur->i_dirBlock[dno] == -1) {
            i += 16;
            continue;
        }
        
        fseek(fr, cur->i_dirBlock[dno], SEEK_SET);
        fread(dirlist, sizeof(dirlist), 1, fr);
        fflush(fr);
        
        int j;
        for (j = 0; j < 16; j++) {
            if (strcmp(dirlist[j].dirName, name) == 0) {
                print("cannot create directory: File exists\n");
                return -1;
            }
            else if (strcmp(dirlist[j].dirName, "") == 0) {
                if (posi == -1) {
                    posi = dno;
                    posj = j;
                }
            }
            i++;
        }
    }
    if (posi != -1) {
        fseek(fr, cur->i_dirBlock[posi], SEEK_SET);
        fread(dirlist, sizeof(dirlist), 1, fr);
        fflush(fr);
        
        strcpy(dirlist[posj].dirName, name);
        int chiinoAddr = ialloc();
        if (chiinoAddr == -1) {
            print("function mkdir(): inode allocate failed\n");
            return -1;
        }
        dirlist[posj].inodeAddr = chiinoAddr;
        
        struct Inode* inode = (struct Inode*)malloc(sizeof(struct Inode));
        inode->i_inode = (chiinoAddr - inodeStartAddr) / superBlock->s_inode_size;
        inode->i_atime = time(NULL);
        inode->i_ctime = time(NULL);
        inode->i_mtime = time(NULL);
        strcpy(inode->i_uname, curUserName);
        strcpy(inode->i_gname, curGroupName);
        inode->i_count = 2;
        
        int curBlockAddr = balloc();
        if (curBlockAddr == -1) {
            print("block allocate failed\n");
            return -1;
        }
        Dir dirlist2[16] = {0};
        strcpy(dirlist2[0].dirName, ".");
        strcpy(dirlist2[1].dirName, "..");
        dirlist2[0].inodeAddr = chiinoAddr; //current directory address
        dirlist2[1].inodeAddr = parinoAddr; //parent directory address
        
        fseek(fw, curBlockAddr, SEEK_SET);
        fwrite(dirlist2, sizeof(dirlist2), 1, fw);
        
        inode->i_dirBlock[0] = curBlockAddr;
        int k;
        for (k = 1; k < 10; k++) {
            inode->i_dirBlock[k] = -1;
        }
        inode->i_size = superBlock->s_block_size;
        inode->i_indirBlock_1 = -1;
        inode->i_mode = MODE_DIR | DIR_DEF_PERMISSION;

        fseek(fw, chiinoAddr, SEEK_SET);
        fwrite(inode, sizeof(struct Inode), 1, fw);
        
        fseek(fw, cur->i_dirBlock[posi], SEEK_SET);
        fwrite(dirlist, sizeof(dirlist), 1, fw);
        
        cur->i_count++;
        fseek(fw, parinoAddr, SEEK_SET);
        fwrite(cur, sizeof(struct Inode), 1, fw);
        fflush(fw);
        
        return 0
    }
    else {
        print("free directory not found, directory create failed\n");
        return -1;
    }
}

void print(char* str) {
    write(1, str, strlen(str));
}

void Login() {
    write(1,"Login Interface\nUserName: ", "Login Interface\nUserName: ");

}
