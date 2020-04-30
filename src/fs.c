#include "fs.h"

struct SuperBlock superBlock;

static char p1[100];
static char p2[100];
static char p3[100];
static char buf[1000];

void test();

int main() {
    int tes = 1;
    if (tes) {
        test();
        return 0;
    }
    //Init the virtual disk
    int res;
    if ((res = Init()) == -1) {
        print("file system init failed\n");
        return -1;
    }


    char str[100];
    system("reset");
    while (1) {
        char *p;
        if ((p = strstr(curDirName, curUserDirName)) != NULL) {
            printf("[%s@%s %s]# ", curUserName, curHostName, curDirName);
        }
        else {
            printf("[%s@%s ~%s]# ", curUserName, curHostName, curDirName + strlen(curUserName));
        }
        fgets(str, sizeof(str), stdin);
        cmd(str);
    }
    fclose(fw);
    fclose(fr);
    return 0;

}

void test() {
    struct Dir dirlist[16];
    printf("size of dirlist: %zx\n",sizeof(dirlist));
}

int Init() {
    superBlockStartAddr = 0;
    inodeBitmapStartAddr = superBlockStartAddr + 1 * BLOCK_SIZE;
    blockBitmapStartAddr = inodeBitmapStartAddr + 2 * BLOCK_SIZE;
    inodeStartAddr = blockBitmapStartAddr + 20 * BLOCK_SIZE;
    blockStartAddr = inodeStartAddr + INODE_NUM * INODE_SIZE;
    sumSize = blockStartAddr + BLOCK_NUM * BLOCK_SIZE;
    maxFileSize = 10 * BLOCK_SIZE + BLOCK_SIZE / sizeof(int) * BLOCK_SIZE;


    int res;

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
        strcpy(curHostName, "ubuntu");
        
        rootDirAddr = inodeStartAddr;
        curDirAddr = rootDirAddr;
        strcpy(curDirName, "/");
        
        printf("file system formating\n");
        sleep(2);
        if ((res = Format()) == -1) {
            printf("disk format failed\n");
            return -1;
        }
        printf("file system format done\n");
        printf("file system installing\n");
        sleep(2);
        if ((res = Install()) == -1) {
            printf("file system install failed\n");
            return -1;
        }
    }
    else {
        fread(buffer, sumSize, 1, fr);
        fw = fopen(FILESYSNAME, "wb");
        if (fw == NULL) {
            print("virtual disk open failed\n");
            return -1;
        }
        fwrite(buffer, sumSize, 1, fw);
        
        nextUID = 0;
        nextGID = 0;
        isLogin = false;
        strcpy(curUserName, "root");
        strcpy(curGroupName, "root");
        
        strcpy(curHostName, "ubuntu");
        
        rootDirAddr = inodeStartAddr;
        curDirAddr = rootDirAddr;
        strcpy(curDirName, "/");
        if ((res = Install()) == -1) {
            print("file system install failed\n");
            return -1;
        }
    }
    return 0;    
}

int Format() { //When you first time install the file system, some format need to be done.
    //format super block
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
    
    //create directory and config file
    /*mkDir(rootDirAddr, "home");
    chDir(rootDirAddr, "home");
    mkDir(curDirAddr, "root");
    
    chDir(curDirAddr, "..");
    mkDir(curDirAddr, "etc");
    chDir(curDirAddr, "etc");
    
    char buf[1000] = {0};
    sprintf(buf, "root:x:%d:%d\n", nextUID++, nextGID++); //format user:passwd:userID:groupID
    create(curDirAddr, "passwd", buf);
    sprintf(buf, "root:root\n"); 
    create(curDirAddr, "shadow", buf); //create user password file
    chMod(curDirAddr, "shadow", 0660);
    
    sprintf(buf, "root::0:root\n");
    sprintf(buf + strlen(buf), "user::1:\n");
    create(curDirAddr, "group", buf);
    
    chDir(curDirAddr, "..");*/
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
    
    return 0;
}

int balloc() { //block allocate funciton
    int top;
    if (superBlock.s_free_addr == -1) {
        print("there is no enough block\n");
        return -1;
    }
    else {
        top = (superBlock.s_free_block_num - 1) % superBlock.s_blocks_per_group;
    }
    int retAddr;
    
    if (top == 0) {
        retAddr = superBlock.s_free_addr;
        superBlock.s_free_addr = superBlock.s_free[0];
        fseek(fr, superBlock.s_free_addr, SEEK_SET);
        fread(&superBlock.s_free, sizeof(superBlock.s_free), 1, fr);
        fflush(fr);
        
        superBlock.s_free_block_num--;
    }
    else {
        retAddr = superBlock.s_free[top];
        superBlock.s_free[top] = -1;
        //top--;
        superBlock.s_free_block_num--;
    }
    
    //update super block
    fseek(fw, superBlockStartAddr, SEEK_SET);
    fwrite(&superBlock, sizeof(superBlock), 1, fw);
    fflush(fw);
    
    //update block bitmap
    blockBitmap[(retAddr - blockStartAddr) / BLOCK_SIZE] = 1;
    fseek(fw, (retAddr - blockStartAddr) / BLOCK_SIZE + blockBitmapStartAddr, SEEK_SET);
    fwrite(&blockBitmap[(retAddr - blockStartAddr) / BLOCK_SIZE], sizeof(bool), 1, fw);
    fflush(fw);

    return retAddr;
}

//block free function
int bfree(int addr) {
    if (addr < blockStartAddr || (addr - blockStartAddr) % superBlock.s_block_size != 0) {
        printf("function bfree(): invalid block address\n");
        return -1;
    }
    unsigned int bno = (addr - blockStartAddr) / superBlock.s_block_size; //block number
    if (blockBitmap[bno] == 0) {
        print("function bfree(): this is an unused block, not allowed to free\n");
        return -1;
    }
    
    int top;
    if (superBlock.s_free_block_num == superBlock.s_block_num) {
        print("function bfree(): no non-idle blocks, not allowed to free\n");
        return -1;
    }
    else {
        top = (superBlock.s_free_block_num - 1) % superBlock.s_blocks_per_group;
        char tmp[BLOCK_SIZE] = {0}; 
        fseek(fw, addr, SEEK_SET);
        fwrite(tmp, sizeof(tmp), 1, fw);

        if (top == superBlock.s_blocks_per_group - 1) {
            superBlock.s_free[0] = superBlock.s_free_addr;
            superBlock.s_free_addr = addr; // I add this line
            int i;
            for (i = 1;i < superBlock.s_blocks_per_group; i++) {
                superBlock.s_free[i] = -1;
            }
            fseek(fw, addr, SEEK_SET);
            fwrite(&superBlock.s_free, sizeof(superBlock.s_free), 1, fw);
        }
        else {
            top++;
            superBlock.s_free[top] = addr;
        }
    }
    
    superBlock.s_free_block_num++;
    fseek(fw, superBlockStartAddr, SEEK_SET);
    fwrite(&superBlock, sizeof(superBlock), 1, fw);
    
    //update block bitmap
    blockBitmap[bno] = 0;
    fseek(fw, bno + blockBitmapStartAddr, SEEK_SET);
    fwrite(&blockBitmap[bno], sizeof(bool), 1, fw);
    fflush(fw);
    
    return 0;

}

int ialloc() { //inode allocate function
    if (superBlock.s_free_inode_num == 0) {
        print("no enough inode\n");
        return -1;
    }
    else {
        int i;
        for (i = 0; i < superBlock.s_inode_num;i++) {
            if (inodeBitmap[i] == 0) {
                break;
            }
        }
        
        //update super block
        superBlock.s_free_inode_num--;
        fseek(fw, superBlockStartAddr, SEEK_SET);
        fwrite(&superBlock, sizeof(superBlock), 1 ,fw);
        
        //update inode bitmap
        inodeBitmap[i] = 1;
        fseek(fw, inodeBitmapStartAddr + i, SEEK_SET);
        fwrite(&inodeBitmap[i], sizeof(bool), 1, fw);
        fflush(fw);
        
        return inodeStartAddr + i * superBlock.s_inode_size;
    }
}

//inode free function
int ifree(int addr) {
    if (addr < inodeStartAddr || (addr - inodeStartAddr) % superBlock.s_inode_size != 0) {
        print("function ifree(): invalid inode address\n");
        return -1;
    }
    unsigned short ino = (addr - inodeStartAddr) % superBlock.s_inode_size;
    if (inodeBitmap[ino] == 0) {
        print("function ifree(): the inode is unused, not allowed to free\n");
        return -1;
    }
    
    //clear inode
    char tmp[INODE_SIZE] = {0}; 
    fseek(fw, addr, SEEK_SET);
    fwrite(tmp, sizeof(tmp), 1, fw);
    
    //update super block
    superBlock.s_free_inode_num++;
    fseek(fw, superBlockStartAddr, SEEK_SET);
    fwrite(&superBlock, sizeof(superBlock), 1, fw);
    
    //update inode bitmap
    inodeBitmap[ino] = 0;
    fseek(fw, inodeBitmapStartAddr + ino, SEEK_SET);
    fwrite(&inodeBitmap[ino], sizeof(bool), 1, fw);
    fflush(fw);

    return 0;
}

int mkDir(int parinoAddr, char name[]) {
    printf("making dir...\n");
    if (strlen(name) >= MAX_NAME_SIZE) {
        print("function mkDir(): directory name too long\n");
        return -1;
    }
    int dirlist_per_group = BLOCK_SIZE / (4 + MAX_NAME_SIZE);
    struct Dir dirlist[16] = {
        0
    };
    
    struct Inode curInode;
    struct Inode* cur = &curInode;
    fseek(fr, parinoAddr, SEEK_SET);
    fread(cur, sizeof(struct Inode), 1, fr);
    
    int i = 0, count = curInode.i_count + 1, posi = -1, posj = -1, dno = 0;
    while (i < 160) {
        //in 160 directories, directly search in direct blocks
        dno = i / 16;
        
        if (curInode.i_dirBlock[dno] == -1) {
            i += 16;
            continue;
        }
        
        fseek(fr, curInode.i_dirBlock[dno], SEEK_SET);
        fread(&dirlist, sizeof(dirlist), 1, fr);
        fflush(fr);
        
        int j;
        for (j = 0; j < 16; j++) {
            printf("files under the directory: %s\n", dirlist[j].dirName);
            if (strcmp(dirlist[j].dirName, name) == 0) {
                print("cannot create directory: File exists\n");
                return -1;
            }
            else if (posi == -1 && strcmp(dirlist[j].dirName, "") == 0) {
                posi = dno;
                posj = j;
            }
            i++;
        }
    }
    if (posi == -1) { //can't find free dirItem in the blocks already exist, have to create a new block
        dno = 0;
        while (dno < DIR_DIRECT_BLOCKS && curInode.i_dirBlock[dno] != -1) {
            dno++;
        }
        if (dno == DIR_DIRECT_BLOCKS) {
            print("the number of directories has readched to the maximum\n");
            return -1;
        }
        curInode.i_dirBlock[dno] = balloc();
        if (curInode.i_dirBlock[dno] == -1) {
            print("function mkdir(): create block failed\n");
            return -1;
        }
        posi = dno;
        posj = 0;
    }
    if (posi != -1) {
        fseek(fr, cur->i_dirBlock[posi], SEEK_SET);
        fread(dirlist, sizeof(dirlist), 1, fr);
        fflush(fr);
        
        int chiinoAddr = ialloc();
        printf("new inode address: %d\n", chiinoAddr);
        if (chiinoAddr == -1) {
            print("function mkdir(): inode allocate failed\n");
            //if the block is in a new block group, it need be freed
            return -1;
        }
        dirlist[posj].inodeAddr = chiinoAddr;
        strcpy(dirlist[posj].dirName, name);
        
        struct Inode newInode;
        struct Inode* inode = &newInode;
        inode->i_inode = (chiinoAddr - inodeStartAddr) / superBlock.s_inode_size;
        inode->i_atime = time(NULL);
        inode->i_ctime = time(NULL);
        inode->i_mtime = time(NULL);
        strcpy(inode->i_uname, curUserName);
        strcpy(inode->i_gname, curGroupName);
        inode->i_count = 2;
        
        int curBlockAddr = balloc();
        printf("new block address: %d\n", curBlockAddr);
        if (curBlockAddr == -1) {
            print("block allocate failed\n");
            return -1;
        }
        struct Dir dirlist2[16] = {0};
        strcpy(dirlist2[0].dirName, ".");
        strcpy(dirlist2[1].dirName, "..");
        dirlist2[0].inodeAddr = chiinoAddr; //current directory address
        dirlist2[1].inodeAddr = parinoAddr; //parent directory address
        
        fseek(fw, curBlockAddr, SEEK_SET);
        fwrite(&dirlist2, sizeof(dirlist2), 1, fw);
        
        inode->i_dirBlock[0] = curBlockAddr;
        int k;
        for (k = 1; k < 10; k++) {
            inode->i_dirBlock[k] = -1;
        }
        inode->i_size = superBlock.s_block_size;
        inode->i_indirBlock_1 = -1;
        inode->i_mode = MODE_DIR | DIR_DFT_PERMISSION;

        fseek(fw, chiinoAddr, SEEK_SET);
        fwrite(inode, sizeof(struct Inode), 1, fw);
        
        fseek(fw, curInode.i_dirBlock[posi], SEEK_SET);
        fwrite(&dirlist, sizeof(dirlist), 1, fw);
        
        curInode.i_count++;
        fseek(fw, parinoAddr, SEEK_SET);
        fwrite(cur, sizeof(struct Inode), 1, fw);
        fflush(fw);
        
        return 0;
    }
    else {
        print("free directory not found, directory create failed\n");
        return -1;
    }
}

int rmall(int parinoAddr) { //delete everything under this directory
    struct Inode curInode;
    struct Inode* cur = &curInode;
    fseek(fr, parinoAddr, SEEK_SET);
    fread(&curInode, sizeof(struct Inode), 1, fr);
    
    int cnt = curInode.i_count, res = 0;
    if (cnt <= 2) {
        res = bfree(curInode.i_dirBlock[0]);
        if (res == -1) {
            print("function rmall(): free block failed\n");
            return -1;
        }
        res = ifree(parinoAddr);
        if (res == -1) {
            print("function rmall(): free inode failed\n");
            return -1;
        }
        return 0;
    }
    
    int i = 0;
    while (i < 160) {
        if (cur->i_dirBlock[i/16] == -1) {
            i += 16;
            continue;
        }
        struct Dir dirlist[16] = {
            0
        };
        //extract disk block
        int blockAddr = cur->i_dirBlock[i/16];
        fseek(fr, blockAddr, SEEK_SET);
        fread(&dirlist, sizeof(dirlist), 1, fr);
        
        //extract dit item from block, and delete recursively
        int j;
        bool f = false;
        for (j = 0;j < 16; j++) {
            if (!(strcmp(dirlist[j].dirName, "") == 0 || strcmp(dirlist[j].dirName, ".") == 0 || strcmp(dirlist[j].dirName, "..") == 0)) {
            f = true;
            rmall(dirlist[j].inodeAddr);
            }
            cnt = cur->i_count;
            i++;
        }
        if (f) {
            res = bfree(blockAddr);
            if (res == -1) {
                print("function rmall(): block free failed\n");
                return -1;
            }
        }
    }
    res = ifree(parinoAddr);
    if (res == -1) {
        print("function rmall(): block free failed\n");
        return -1;
    }
    return 0;
}

int rmDir(int curAddr, char name[]) {
    if (strlen(name) >= MAX_NAME_SIZE) {
        printf("directory name too long\n");
        return -1;
    }
    
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        printf("wrong operation\n");
        return -1;
    }
    
    struct Inode curInode;
    fseek(fr, curAddr, SEEK_SET);
    fread(&curInode, sizeof(struct Inode), 1, fr);
    
    int cnt = curInode.i_count;
    
    struct Dir dirlist[16] = {
        0
    };
    int i = 0;
    while (i < 160) {
        if (curInode.i_dirBlock[i/16] == -1) {
            i += 16;
            continue;
        }
        
        int blockAddr = curInode.i_dirBlock[i/16];
        fseek(fr, blockAddr, SEEK_SET);
        fread(&dirlist, sizeof(dirlist), 1, fr);
        
        int j;
        for (j = 0;j < 16; j++) {
            if (strcmp(dirlist[j].dirName, name) == 0) {
                struct Inode tmp;
                fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
                fread(&tmp, sizeof(struct Inode), 1, fr);
                if (((tmp.i_mode >> 9) & 1) != 1) {
                    print("error operation: not a directory\n");
                    return -1;
                }
                printf("the name of directory: %s\n", dirlist[j].dirName);
                rmall(dirlist[j].inodeAddr);
                
                strcpy(dirlist[j].dirName, "");
                dirlist[j].inodeAddr = -1;
                fseek(fw, blockAddr, SEEK_SET);
                fwrite(&dirlist, sizeof(dirlist), 1, fw);
                curInode.i_count--;
                fseek(fw, curAddr, SEEK_SET);
                fwrite(&curInode, sizeof(curInode), 1, fw);
                fflush(fw);
                
                return 0;
            }
            i++;
        }
    }
    print("directory not found\n");
    return -1;
}

//create file function
int create(int parinoAddr, char name[], char buf[]) {
    if (strlen(name) >= MAX_NAME_SIZE) {
        print("file's name too long, not allowed to exceed to 28 chars");
        return -1;
    }

    //first compare the name with names of files under the current directory
    struct Dir dirlist[16] = {
        0
    };
    
    struct Inode curInode;
    fseek(fr, parinoAddr, SEEK_SET);
    fread(&curInode, sizeof(struct Inode), 1, fr);
    
    int i = 0, posi = -1, posj = -1;
    int count = curInode.i_count + 1;
    while (i < 160) {
        int dno = i / 16;
        if (curInode.i_dirBlock[dno] == -1) {
            i += 16;
            continue;
        }
        
        fseek(fr, curInode.i_dirBlock[dno], SEEK_SET);
        fread(&dirlist, sizeof(dirlist), 1, fr);

        int j;
        for (j = 0; j < 16; j++) {
            if (strcmp(dirlist[j].dirName, name) == 0) {
                print("file create failed: file already exists\n");
                buf[0] = '\0'; 
                return -1;
            }
            
            if (posi == -1 && strcmp(dirlist[j].dirName, "") == 0) {
                posi = dno;
                posj = j;
            }
            i++;
        }
    }
    
    if (posi == -1) {
        i = 0;
        while (i < 16 && curInode.i_dirBlock[i] != -1) {
            i++;
        }
        if (i == 16) {
            print("create file failed: no enough space under this directory\n");
            return -1;
        }
        posi = i;
        posj = 0;
    }
    
    if (posi != -1) {
        fseek(fr, curInode.i_dirBlock[posi], SEEK_SET);
        fread(&dirlist, sizeof(dirlist), 1, fr);
        fflush(fr);
        
        strcpy(dirlist[posj].dirName, name);
        int newInodeAddr = ialloc();
        if (newInodeAddr == -1) {
            print("create function: inode allocate failed\n");
            return -1;
        }
        dirlist[posj].inodeAddr = newInodeAddr;
        fseek(fw, curInode.i_dirBlock[posi], SEEK_SET);
        fwrite(&dirlist, sizeof(dirlist), 1, fw);
        fflush(fw);
        
        struct Inode newInode;
        fseek(fr, newInodeAddr, SEEK_SET);
        fread(&newInode, sizeof(struct Inode), 1, fr);
        newInode.i_inode = (newInodeAddr - inodeStartAddr)/superBlock.s_inode_size;
        newInode.i_atime = time(NULL);
        newInode.i_ctime = time(NULL);
        newInode.i_mtime = time(NULL);
        strcpy(newInode.i_uname, curUserName);
        strcpy(newInode.i_gname, curGroupName);
        newInode.i_count = 1;

        int fileSize = strlen(buf);
        int k;
        for (k = 0; k < fileSize; k += superBlock.s_inode_size) {
            int newBlockAddr = balloc();
            if (newBlockAddr == -1) {
                print("create function: block allocate failed\n");
                return -1;
            }
            newInode.i_dirBlock[k] = newBlockAddr;
            fseek(fw, newBlockAddr, SEEK_SET);
            fwrite(buf + k, superBlock.s_block_size, 1 ,fw);
        }
        
        for (k = fileSize / superBlock.s_block_size; k < 10; k++) {
            newInode.i_dirBlock[k] = -1;
        }
        if (fileSize == 0) { //allocate a block even the file size is 0
            int newBlockAddr = balloc();
            if (newBlockAddr == -1) {
                print("function create(): block allocate failed\n");
                return -1;
            }
            
            newInode.i_dirBlock[k / superBlock.s_block_size] = newBlockAddr;
            fseek(fw, newBlockAddr, SEEK_SET);
            fwrite(buf, superBlock.s_block_size, 1, fw);
        }
        newInode.i_size = fileSize;
        newInode.i_indirBlock_1 = -1;
        newInode.i_mode = 0;
        newInode.i_mode = MODE_FILE | FILE_DFT_PERMISSION;
        
        fseek(fw, newInodeAddr, SEEK_SET);
        fwrite(&newInode, sizeof(struct Inode), 1, fw);
        
        curInode.i_count++;
        fseek(fw, parinoAddr, SEEK_SET);
        fwrite(&curInode, sizeof(struct Inode), 1, fw);
        fflush(fw);
        
        return 0;
    }
    else
        return -1;
}

//delete file function
int del(int parinoAddr, char name[]) {
    if (strlen(name) >= MAX_NAME_SIZE) {
        print("file not exists\n");
        return -1;
    }
    
    struct Inode curInode;
    fseek(fr, parinoAddr, SEEK_SET);
    fread(&curInode, sizeof(struct Inode), 1, fr);
    
    int i = 0, posi = -1, posj = -1;
    
    int filemode;
    if (strcmp(curUserName, curInode.i_uname) == 0) {
        filemode = 6;
    }
    else if (strcmp(curUserName, curInode.i_gname) == 0) {
        filemode = 3;
    }
    else {
        filemode = 0;
    }
    
    if (((curInode.i_mode >> filemode >> 1) & 1) == 0) {
        print("no authority to delete\n");
        return -1;
    }
    
    while (i < 160) {
        struct Dir dirlist[16] = {
            0
        };
        int dno = i / 16;
        if (curInode.i_dirBlock[dno] == -1) {
            i += 16;
            continue;
        }
        
        int blockAddr = curInode.i_dirBlock[dno];
        fseek(fr, blockAddr, SEEK_SET);
        fread(&dirlist, sizeof(dirlist), 1, fr);
        
        int j;
        for (j = 0; j < 16; j++) {
            struct Inode tmp;
            fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
            fread(&tmp, sizeof(struct Inode), 1, fr);

            if (strcmp(dirlist[j].dirName, name) == 0) {
                if (((tmp.i_mode >> 9) & 1) == 1) {
                    print("cannot remove a directory\n");
                    return -1;
                }
                int k;
                for (k = 0; k < 10; k++) {
                    if (tmp.i_dirBlock[k] != -1) {
                        bfree(tmp.i_dirBlock[k]);
                    }
                }
                ifree(dirlist[j].inodeAddr);
                
                strcpy(dirlist[j].dirName, "");
                dirlist[j].inodeAddr = -1;
                fseek(fw, blockAddr, SEEK_SET);
                fwrite(&dirlist, sizeof(dirlist), 1, fw);
                
                curInode.i_count--;
                fseek(fw, parinoAddr, SEEK_SET);
                fwrite(&curInode, sizeof(struct Inode), 1, fw);
                fflush(fw);

                return 0;
            }
            i++;
        }
    }
    print("file not found\n");
    return -1;
}


int list(int parinoAddr) {
    if (parinoAddr == inodeStartAddr) {
        printf("you are in root directory\n");
    }
    struct Inode curInode;
    fseek(fr, parinoAddr, SEEK_SET);
    fread(&curInode, sizeof(struct Inode), 1, fr);
    fflush(fr);
    
    int i, count = 1;
    struct Dir dirlist[16] = {
        0
    };
    int cnt = curInode.i_count;
    
    printf("inode direct blocks address: \n");
    for (i = 0; i < 10; i++) {
        printf("%d, ", curInode.i_dirBlock[i]);
    }
    printf("\n");

    for (i = 0; i < 10; i++) {
        if (curInode.i_dirBlock[i] == -1) {
            continue;
        }
        
        memset(&dirlist, 0, sizeof(dirlist));
        fseek(fr, curInode.i_dirBlock[i], SEEK_SET);
        fread(&dirlist, sizeof(dirlist), 1, fr);
        fflush(fr);
        
        int j;
        for (j = 0; j < 16; j++) {
            if (strcmp(dirlist[j].dirName, "") == 0) {
                continue;
            }
            else if (strcmp(dirlist[j].dirName, ".") == 0) {
                continue;
            }
            else if (strcmp(dirlist[j].dirName, "..") == 0) {
                continue;
            }
            printf("%s\t", dirlist[j].dirName);
            if (count % 5 == 0) {
                printf("\n");
            }
            count++;
        }
    }
    if (count % 5 != 1) {
        printf("\n");
    }
    return 0;
}

int chMod(int parinoAddr, char name[], int pmode) {
    return 0;
}

int chDir(int parinoAddr, char name[]) {
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
            print("touch file failed\n");
        }
    }
    else if (strcmp(p1, "rm") == 0) {
        sscanf(str, "%s%s", p1, p2);
        del(curDirAddr, p2);
    }
    else if (strcmp(p1, "ls") == 0) {
        sscanf(str, "%s%s", p1, p2);
        printf("p2: %s\n", p2);
        if (strcmp(p2, "") == 0) {
            list(curDirAddr);
        }
        else {
            //cd
        }
    }
    else if (strcmp(p1, "") == 0) {
        //do nothing
    }
    else if (strcmp(p1, "exit") == 0) {
        exit(0);
    }
    else {
        print("command not found\n");
    }
}


int print(char* str) {
    write(1, str, strlen(str));
}

int Login() {
    write(1,"Login Interface\nUserName: ", strlen("Login Interface\nUserName: "));

}
