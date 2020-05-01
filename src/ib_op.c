#include "mono.h"

int balloc() { //block allocate funciton
    int top;
    if (superBlock.s_free_addr == -1) {
        printf("there is no enough block\n");
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

int bfree(int addr) {
    if (addr < blockStartAddr || (addr - blockStartAddr) % superBlock.s_block_size != 0) {
        printf("function bfree(): invalid block address\n");
        return -1;
    }
    unsigned int bno = (addr - blockStartAddr) / superBlock.s_block_size; //block number
    if (blockBitmap[bno] == 0) {
        printf("function bfree(): this is an unused block, not allowed to free\n");
        return -1;
    }
    
    int top;
    if (superBlock.s_free_block_num == superBlock.s_block_num) {
        printf("function bfree(): no non-idle blocks, not allowed to free\n");
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
        printf("no enough inode\n");
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
        printf("function ifree(): invalid inode address\n");
        return -1;
    }
    unsigned short ino = (addr - inodeStartAddr) / superBlock.s_inode_size;
    if (inodeBitmap[ino] == 0) {
        printf("function ifree(): the inode is unused, not allowed to free\n");
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
