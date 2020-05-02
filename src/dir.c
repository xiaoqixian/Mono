#include "mono.h"

int mkDir(int parinoAddr, char name[]) {
    if (strlen(name) >= MAX_NAME_SIZE) {
        printf("function mkDir(): directory name too long\n");
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
    fflush(fr);
    
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
            if (strcmp(dirlist[j].dirName, name) == 0) {
                printf("cannot create directory: File exists\n");
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
            printf("the number of directories has readched to the maximum\n");
            return -1;
        }
        curInode.i_dirBlock[dno] = balloc();
        if (curInode.i_dirBlock[dno] == -1) {
            printf("function mkdir(): create block failed\n");
            return -1;
        }
        posi = dno;
        posj = 0;
    }
    if (posi != -1) {
        fseek(fr, cur->i_dirBlock[posi], SEEK_SET);
        fread(&dirlist, sizeof(dirlist), 1, fr);
        fflush(fr);
        
        int chiinoAddr = ialloc();
        if (chiinoAddr == -1) {
            printf("function mkdir(): inode allocate failed\n");
            //if the block is in a new block group, it need be freed
            return -1;
        }
        //printf("new inode address: %d\n", chiinoAddr);
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
        if (curBlockAddr == -1) {
            printf("block allocate failed\n");
            ifree(chiinoAddr);
            return -1;
        }
        //printf("new block address: %d\n", curBlockAddr);
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
        printf("free inode not found, directory create failed\n");
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
            printf("function rmall(): free block failed\n");
            return -1;
        }
        res = ifree(parinoAddr);
        if (res == -1) {
            printf("function rmall(): free inode failed\n");
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
                printf("function rmall(): block free failed\n");
                return -1;
            }
        }
    }
    res = ifree(parinoAddr);
    if (res == -1) {
        printf("function rmall(): block free failed\n");
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
                    printf("error operation: not a directory\n");
                    return -1;
                }
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
    printf("directory not found\n");
    return -1;
}

//change directory to root directory
void chRoot() {
    memset(curDirName, 0, sizeof(curDirName));
    curDirAddr = rootDirAddr;
    strcpy(curDirName, "/");
}

//change    
int chChiDir(int parinoAddr, char name[]) {
    if (strlen(name) > MAX_NAME_SIZE) {
        printf("dirctory not found\n");
        return -1;
    }
    if (strcmp(name, "") == 0) {
        printf("empty name not accepted\n");
        return -1;
    }
    
    if (strcmp(name, ".") == 0) {
        return 0;
    }

    struct Inode curInode;
    fseek(fr, parinoAddr, SEEK_SET);
    fread(&curInode, sizeof(curInode), 1, fr);
    fflush(fr);
    
    int i;
    struct Dir dirlist[16] = {
        0
    };
    for (i = 0; i < DIR_DIRECT_BLOCKS; i++) {
        if (curInode.i_dirBlock[i] == -1) {
            continue;
        }
        
        fseek(fr, curInode.i_dirBlock[i], SEEK_SET);
        fread(&dirlist, sizeof(dirlist), 1, fr);
        fflush(fr);
        
        int j;
        for (j = 0; j < 16; j++) {
            if (strcmp(name, dirlist[j].dirName) == 0) {
                struct Inode tarInode;
                fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
                fread(&tarInode, sizeof(tarInode), 1, fr);
                fflush(fr);
                
                if (((tarInode.i_mode >> 9) & 1) != 1) {
                    printf("%s is not a directory\n", name);
                    return -1;
                }
                
                curDirAddr = dirlist[j].inodeAddr;
                if (strcmp(name, "..") == 0) {
                    int k;
                    for (k = strlen(curDirName); k >= 0; k--) {
                        if (curDirName[k] == '/') {
                            break;
                        }
                    }
                    curDirName[k] = '\0';
                    if (strlen(curDirName) == 0) {
                        curDirName[0] = '/';
                        curDirName[1] = '\0';
                    }
                }
                else {
                    if (curDirName[strlen(curDirName) - 1] != '/') {
                        strcat(curDirName, "/");
                    }
                    strcat(curDirName, dirlist[j].dirName);
                }
                return 0;
            }
        }
    }
    printf("directory not found\n");
    return -1;
}

//change to a directory according to the path
int chDir(int parinoAddr, char path[]) {
    int res, index = 0, before = 0;
    char tmp[MAX_NAME_SIZE];
    if (path[0] == '/') {
        chRoot();
        index++;
        before++;
    }
    while (index < strlen(path)) {
        if (path[index] == '/') {
            break;
        }
        index++;
    }
    memset(tmp, 0, sizeof(tmp));
    int i, j = 0;
    for (i = before; i < index; i++) {
        tmp[j++] = path[i];
    }
    if (strcmp(tmp, "") == 0) {
        return 0;
    }
    res = chChiDir(curDirAddr, tmp);
    if (res == -1) {
        printf("change directory failed\n");
        return -1;
    }
    if (index == strlen(path)) {
        return 0;
    }
    char* subPath = &path[index + 1];
    chDir(curDirAddr, subPath);

    return 0;
}
