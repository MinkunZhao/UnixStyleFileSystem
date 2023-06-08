#ifndef INODE_H
#define INODE_H

#include <time.h>

class INode {
public:
    static const int NUM_DIRECT_ADDRESS = 10;
    static const int NUM_INDIRECT_ADDRESS = 1;

public:
    int id;
    int fsize;  // in KB
    int fmode;  // 1表示dentry，0表示file
    int count;  // 如果是dentry，那它拥有的文件数目
    int mcount; // history maximum number
    time_t ctime;
    int dir_address[NUM_DIRECT_ADDRESS];
    int indir_address[NUM_INDIRECT_ADDRESS];

public:
    INode();
    ~INode();
    void clear();
};

#endif