#include "mono.h"

int create(int parinoAddr, char name[], char buf[]) {
    if (strlen(name) >= MAX_NAME_SIZE) {
        printf("file's name too long, not allowed to exceed to 28 chars");
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
                printf("file create failed: file already exists\n");
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
            printf("create file failed: no enough space under this directory\n");
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
            printf("create function: inode allocate failed\n");
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
                printf("create function: block allocate failed\n");
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
                printf("function create(): block allocate failed\n");
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
        printf("file not exists\n");
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
        printf("no authority to delete\n");
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
                    printf("cannot remove a directory\n");
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
    printf("file not found\n");
    return -1;
}

int list(int parinoAddr) {
    if (curDirAddr == rootDirAddr) {
        //printf("you are in root dir\n");
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

int vim(int parinoAddr, char name[]) {
    char cmd[100] = "vim ";
    strcat(cmd, "tmpFile/");
    strcat(cmd, name);
    int res = system(cmd);
    if (res == -1) {
        printf("system command vim failed\n");
        return -1
    }
    res = readFile(parinoAddr, name);
    if (res == -1) {
        printf("vim read file failed\n");
        memset(cmd, 0, sizeof(cmd));
        strcpy(cmd, "rm ");
        strcat(cmd, name);
        res = system(cmd);
        if (res == -1) {
            printf("system command rm failed\n");
            return -1;
        }
        return -1;
    }
    return 0;
}

//read the tmp file made by vim
int readFile(int parinoAddr, char name[]) {
    //find the target file inode
    struct Inode parInode;
    fseek(fr, parinoAddr, SEEK_SET);
    fread(&parInode, sizeof(parInode), 1, fr);
    
    int i;
    struct Dir dirlist[16] = {
        0
    };
    struct Inode tarInode;
    int flag = 1;
    for (i = 0; i < 10 && flag; i++) {
        if (parInode.i_dirBlock[i] == -1) {
            continue;
        }
        
        fseek(fr, parInode.i_dirBlock[i], SEEK_SET);
        fread(&dirlist, sizeof(dirlist), 1, fr);
        
        int j;
        for (j = 0; j < 16; j++) {
            if (strcmp(dirlist[j].dirName, name) == 0) {
                fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
                fread(&tarInode, sizeof(tarInode), 1, fr);
                flag = 0;
                break;
            }
        }
    }
    
    //if file not found, need to create a new file
    if (flag) {
        
    }

    char fileName[100] = {
        0
    };
    strcpy(fileName, "tmpFile/");
    strcat(fileName, name);
    FILE* fr = fopen(fileName, "rb");
    if (fr == NULL) {
        printf("tmp file open failed\n");
        return -1;
    }

    char buffer[512] = {
        0
    };
    size_t size;
    while (1) {
        size = fread(buffer, sizeof(buffer), 1, fr);
        if (size <= 0) {
            break;
        }

    }
}
