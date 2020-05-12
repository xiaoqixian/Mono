### **Mono Filesystem-English Version**

The original project address: https://github.com/elliotxx/os_filesystem

Note in the front row:  This English version of REMAME.md is translated by machine, so if you have any problem with it, post it in the issues.

And because the author is lazy, the vim command has not been implemented. The documentation doesn't cover all the functions in detail, but the essence of the file system is covered.

**Loading process**

1. The first is to determine the location of the disk.

   The super block occupies the first block of the virtual disk.

   Then there are the iNode bitmap and blcok bitmap, which occupy two disk blocks and 20 disk blocks respectively. One disk block is 512Byte, so 1024 inodes and 10240 blocks can be monitored, respectively. It is important to note that although a disk is 512byte, only 512 inodes or blocks can be monitored, because only one byte or more can be used to store data in a virtual file, not a real bit.

   Then there is the iNode region and the block region.

2. Indirect block

   When you're trying to store a large file, you may not have enough inodes in the iNode to store the block Numbers. In the iNode structure, there is a plastic variable called i_indirBlock_1, which stores the number of the indirection block and, in the indirection it points to, the number of the disk block of the file. Because a block has a size of 512B, you can store more block Numbers.

   In fact, there can be more indirect blocks below the level 1 indirect block to support larger file storage, but this system only supports level 1 indirect block for the time being.

**Formatting process **

The file system is formatted the first time it is installed and when the user needs it.

1. Super block formatting

   The formatting of the super block is mainly to create a super block structure and assign values.

2. INode bitmap and block bitmap formatting

3. Data area formatting

   The data area is organized by group linking, with 64 blocks in each group. Position 0 of the array of each group marks the starting address of the header of the next group. The starting address of the first group is not marked, because it is the starting address of the block area, and the zero of the last group is not marked.

   In the formatting process, the s_free stack of the super block is used as a temporary storage place, and the next set of starting addresses is first placed at position 0. You then put the address of that block in each of the other cells. Finally, write this s_free array into the starting block of the group block. That's right, the address data for the group block is placed in the first block of the group. However, don't worry about overwriting this data, because when the last block of the previous block is used, the address data of this group of blocks will be read into s_free of the super block, and then this group of blocks can be written at will.

**Block allocation function `balloc()`**

The `balloc()` function is used to allocate blocks to the storage of directories and files, returning the address of the allocated block.

Since the disk blocks are organized as group links, the super block is first accessed to see if the s_free_addr property of the super block is 0, which indicates that there are no disk blocks left (this property is set to zero after the last disk block is allocated). It then USES the method of redundancy to get the first free disk block which is located in the free disk block stack.

If the free disk block points to the bottom of the stack, it indicates that the free disk block stack is used up and a new disk block group needs to be read into the free disk block group. First of all, the value that the function needs to return is the one at the bottom of the stack of this group of blocks, but the address of the block at the bottom of the stack is not placed in s_free[0], but in s_free_addr of the super block. This property is assigned when the superblock reads a new set of block groups, and is not used until the group is changed.

So the address of the new block you're taking, if it's not the block at the bottom of the stack, just return the address in s_free, otherwise return the address in s_free_addr.

**iNode allocation function `ialloc()`**

`ialloc ()` is used to allocate inodes, returning the address of inodes.

`ialloc ()` has a much simpler procedure than` balloc()`, which is to look up a free iNode linearly, find it, write the corresponding location as 1, which means not free, update the super block, and return the address.

**Block release function `bfree()`**

Blocks are more difficult to release than they are to allocate. Since the allocation can be linear and the release can occur in all block regions, it is a problem to string the freed block into the distributable block.

The algorithm used in this project is to calculate the block number according to the address of the block to be released. (the middle part of the address rationality judgment is omitted). Then, the corresponding block content will be erased, if it is used to erase an array full of 0, remember to release the memory of the array after use, the original author's code did not release this section of memory, to develop a good habit of releasing memory. Determine if the current idle block stack is full. If so, create a new block group and use it first. To do this, change s_free[0] to s_free_addr, which is used to store the address of the block at the bottom of the current block stack. The current block group is removed from the superblock's s_free free stack by setting the address of the rest of the stack to -1. Finally, the data of the removed block is written to the block to be released.

But in principle, the original author wants this block to be the bottom of the latest idle block stack, so s_free_addr should point to the address of the block to be released, but I didn't see this line in the original author's code, so I ventured to add this line. If the stack is full, it is reasonable to say that the number of idle blocks should be an integer multiple of the block group size, so the block to be released is the only one that is left, so according to the algorithm of block allocate, after using this block, it will directly jump to the original group of blocks. The block to be released is then used first.

It's easier if the stack is not full, just put a space on the stack and the address points to the block to be released.

**Create directory function `mkdir()`**

Starting with the Linux file system directory creation process, Linux creates a directory with at least one iNode and one block. The iNode is used to record the permissions and properties of the directory, and the block is used to record the address of the file name and directory iNode.

For the argument to the mkdir function, you pass in parinoAddr, the iNode address of the parent directory, and the file name you want to create. However, this parent directory is not the parent directory of the current directory, but the parent directory for the created directory, that is, the current directory.

First, the inodes of the current address are read out, and then every 16 directory entries are searched for the iNode dirlist property. Why 16? As mentioned earlier, block is mainly used to record the directory name and iNode address, and the address type is int, which is 4 bytes. Plus the maximum allowed length of the name is 28 bytes, so the total is 32 bytes (** the two together are also known as a directory entry **). A block is 512 bytes in size, so a block can hold 16 items. Without using indirect blocks, the iNode dirlist can hold up to 10 block addresses, meaning 160 directories can be created in one directory.

The next in order to find a suitable place to put down the need to create the directory entry, you need to traverse the entire 160 entries, the cause of the need to traverse all the directory entry is prevent the nuptial directory, the author only banned two same directory directory name repetition, but Linux with directory files and directories are no name repetition, so here I am in accordance with the standards of Linux.

To traverse the first ten blocks directly find the block can deposit directory entry, read it out into the dirlist after find out, and then traverse dirlist, if you have a moniker, create failure, if there are any white space (way is to check whether the name of the directory entry is equal to "", not only empty directory name), find a place to write down the block and the block after, you then need to continue to traverse, in order to prevent the nuptial directory in the next block.

Once you find the right block, you can create the directory. First read the dirlist of the selected block, then create an iNode using the `ialloc()` function, and write the relevant information, such as i_inode represents the iNode identifier and uniquely identifies an iNode. In this case, the simple value is the number of iNode. After writing the information, a block is allocated to store the dirlist that created the directory. The directory was created with two names "." and ".." Directory to this directory and to the parent directory, respectively. Finally, the block is written, with the 0 bit of the iNode i_dirBlock pointing to the original block, and everything else set to -1. Then the iNode is also written. Finally the current directory modifies something to also write.

Here need to tell me the mkdir function of the authorship of a bug, he does not take into account when a block is full, just consider the block have try to put the entries in the directory, I understand is to try to a directory of the block, use less as far as possible, because although a directory can store 10 block, but not created 10 block from the start, but there is demand to create. However, the original author did not add the code to create a new block if a block is full, resulting in the creation of more than 15 directories under a directory will appear after the directory creation failure.

So I changed some of the code based on him, leaving the previous part unchanged, and allowing the automatic creation of a block to hold new directory entries when the appropriate dirlist was not found.

**Deletes all files in the directory with the directory function `rmall()` **

The `rmall()` function deletes everything in the directory, but unlike the rm-rf./* command, the `rmall()` function keeps the current directory, and `rmall()` deletes inodes and blocks in the current directory.

The `rmall()` function is implemented recursively, first determining if there is nothing in the current directory except two connections ("." and "..") ) means there are no files or directories, and inodes and blocks are released directly.

Otherwise, all the blocks under i_dirBlock are traversed, each block traverses all the directory entries, and for the iNode address in each directory entry, this function is called recursively as input. This is done until you encounter a directory that has nothing, and then you do the first case. When all this is done, the inodes and blocks of the current directory are released.

**Not first read file problem **

Since the file pointer in C language will clear the existing contents of the file when reading the file in the way of writing, the information of the file system after the previous login cannot be saved.

The solution for the original author is to prepare a very large buffer, read the file into the buffer with the read pointer, and then initialize the write pointer, so that the contents of the file can be set to zero. The write pointer is then used to write all the contents of the buffer.

But this is a waste of memory, and the original author defined the buffer as a global variable, which is in stack memory and cannot be manually released. The first time I tried to define such a large buffer in a function, the stack burst.

So my solution was to create a temporary file, read the contents of the virtual disk file into a smaller buffer with a read pointer (I set it to 1024 bytes), and then write to the temporary file with a write pointer. If you can, there is no problem with setting this file to disk number two, so you can use it directly. But I didn't like the two disk files, so I wrote an operation to write back the contents of the temporary file. This is fine when the disk file is small, but it is very inefficient when it is large.

Finally, remember to release the buffer memory after reading and writing the file. I deliberately defined the memory of the buffer in the heap, which can be released immediately after using it.

**Editor **

The original author wrote on the Windows platform, so he called some Windows interfaces directly and wrote a simple class vim text editor, although it got stuck when he pressed ESC.

But I wrote it on Linux, so I couldn't borrow this code directly, so I decided to invoke the terminal's vim editor by executing shell commands through system functions. Vim then writes the contents to a temporary file and reads the contents of the temporary file to the file system.

The editor does not support read-write of directory files.