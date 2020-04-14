#include <iostream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <conio.h>

#define BLOCK_SIZE 512                    //unit is byte B
#define INODE_SIZE 128
#define MAX_NAME_SIZE 28

#define INODE_NUM 640
#define BLOCK_NUM 10240                   //10240 * 512B = 5120KB
#define BLOCKS_PER_GROUP 64               //Free block stack size

#define MODE_DIR 01000                    //octal digit, directory identifier
#define MODE_FILE 00000                   //file identifier

#define OWNER_R 4<<6                      //this user read authority
#define OWNER_W 2<<6                      //this user write authority
#define OWNRT_X 1<<6                      //execute authority
#define GROUP_R 4<<3                      //group users read authority
#define GROUP_W 2<<3
#define GROUP_X 1<<3
#define OTHERS_R 4
#define OTHERS_W 2
#define OTHERS_X 1
#define FILE_DFT_PERMISSION 0664
#define DIR_DFT_PERMISSION 0775

#define FILESYSNAME "mono.sys"

//super block define
struct SuperBlock {
    unsigned short s_inode_num;
    unsigned int s_block_num;
    
    unsigned short s_free_inode_num;
    unsigned int s_free_inode_num;
    
    int s_free_addr; //free block stack address
    int s_free[BLCOKS_PER_GROUP]; //free block stack
    
    unsigned short s_block_size;
    unsigned short s_inode_size;
    unsigned short s_superblock_size;
    unsigned short s_blocks_per_group; //size of blcok group
    
    //disk distribution
    int s_superblock_startaddr;
    int s_inodebitmap_startaddr;
    int s_blockbitmap_startaddr;
    int s_inode_startaddr;
    int s_block_startaddr;
};


struct Inode {
    unsigned short i_ino; //inode identifier
    unsigned short i_mode; //access permission

    unsigned short i_cnt; //the number of file names that point to this inode

    char i_uname[20]; //file's owner's name
    char i_gname[20];

    unsigned int i_size;
    time_t i_ctime; //the last time the file changed
    time_t i_mtime; //the last time the file content changed
    time_t i_atime; //the last time the file opened
    int i_dirBlock[10]; //10 direct blocks, 10 * 512B = 5120B
    int i_indirBlock_1; //first level indirect block, 512B / 4 * 512B = 64KB
};

struct Dir {
    char dirName[MAX_NAME_SIZE];
    int inodeAddr;
};




















