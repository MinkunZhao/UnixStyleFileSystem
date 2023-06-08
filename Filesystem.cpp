#include "Filesystem.h"
using namespace std;

char buffer[SYSTEM_SIZE];

Filesystem::Filesystem() {
    strcpy(curpath, root_dir);
    flag = true;
}

Filesystem::~Filesystem() {
    fclose(fp);
}

void Filesystem::welcome() {
    cout << "\nHere is our Unix file system!\n";
    cout << "Our group members is as followed: \n";
    cout << "\tZhao Minkun\t\t202030430394\n";
    cout << "\tLuo Yinyi\t\t202064350408\n\n";
    help();
}

void Filesystem::initialize() {
    fstream file;
    file.open(HOME, ios::in);
    if(!file) {
        cout << "The Operating System is initializing...\n";
        fp = fopen(HOME, "wb+");
        if(fp == NULL) {
            cout << "Error when creating the Operating System...\n";
            flag = false;
            return;
        }
        fwrite(buffer, SYSTEM_SIZE, 1, fp); // create a 16MB space for OS

        fseek(fp, 0, SEEK_SET);
        superblock.systemsize = SYSTEM_SIZE;
        superblock.blocksize = BLOCK_SIZE;
        superblock.blocknum = BLOCK_NUM;
        superblock.address_length = ADDRESS_LENGTH;
        superblock.max_filename_size = MAX_FILENAME_SIZE;
        superblock.superblock_size = SUPER_BLOCK_SIZE;
        superblock.inode_size = INODE_SIZE;
        superblock.inode_bitmap_size = INODE_BITMAP_SIZE;
        superblock.block_bitmap_size = BLOCK_BITMAP_SIZE;
        superblock.inode_table_size = INODE_TABLE_SIZE;
        fwrite(&superblock, sizeof(Superblock), 1, fp);

        fseek(fp, INODE_BITMAP_START, SEEK_SET);
        for(int i = 0; i < INODE_BITMAP_SIZE; i++) {
            unsigned char byte = 0;
            fwrite(&byte, sizeof(unsigned char), 1, fp);
        }

        fseek(fp, BLOCK_BITMAP_START, SEEK_SET);
        for(int i = 0; i < BLOCK_BITMAP_SIZE; i++) {
            unsigned char byte = 0;
            fwrite(&byte, sizeof(unsigned char), 1, fp);
        }

        modifyBlockBitmap(0); // set block of superblock busy
        
        for(int i = 0; i < INODE_BITMAP_SIZE/1024; i++) {
            modifyBlockBitmap(i+1);
        } // set block of inode bitmap busy

        for(int i = 0; i < BLOCK_BITMAP_SIZE/1024; i++) {
            modifyBlockBitmap(i+INODE_BITMAP_SIZE/1024+1);
        } // set block of block bitmap busy

        for(int i = 0; i < INODE_TABLE_SIZE/1024; i++) {
            modifyBlockBitmap(i+INODE_BITMAP_SIZE/1024+BLOCK_BITMAP_SIZE/1024+1);
        } // set block of inode table busy

        root_Inode.clear();
        root_Inode.count = 0;
        root_Inode.mcount = 0;
        root_Inode.ctime = time(NULL);
        root_Inode.fmode = DENTRY_MODE;
        root_Inode.id = 0;
        modifyInodeBitmap(0);
        writeInode(0, root_Inode);
        cur_Inode = root_Inode;

        printInfo();
    }
    else {
        cout << "The Operating System is loading...\n";
        fp = fopen(HOME, "rb+");
        if(fp == NULL) {
            cout << "Error when loading the Operating System...\n";
            flag = false;
            return;
        }

        fseek(fp, 0, SEEK_SET);
        fread(&superblock, sizeof(Superblock), 1, fp);

        cur_Inode = root_Inode = readInode(0);

        printInfo();
    }
}

void Filesystem::tip() {
    cout << "[ root@os ]: " << curpath << "# ";
}

void Filesystem::help() {
    cout << "Following commands are supported in our system: \n";

    cout << "createFile <fileName> <fileSize>\n";
    cout << "\te.g. createFile /dir1/myFile 10 (in KB)\n";

    cout << "deleteFile <fileName>\n";
    cout << "\te.g. deleteFile /dir1/myFile\n";

    cout << "createDir <dirPath>\n";
    cout << "\te.g. createDir /dir1/sub1\n";

    cout << "deleteDir <dirPath>\n";
    cout << "\te.g. deleteDir /dir1/sub1\n";

    cout << "changeDir <dirPath>\n";
    cout << "\te.g. changeDir /dir2\n";

    cout << "dir\n";

    cout << "cp <fileName1> <fileName2>\n";
    cout << "\te.g. cp file1 file2\n";

    cout << "sum\n";

    cout << "cat <fileName>\n";
    cout << "\te.g. cat /dir1/file1\n";

    cout << "help\n";

    cout << "exit\n";
}

int Filesystem::findAvailableInode() {
    fseek(fp, INODE_BITMAP_START, SEEK_SET);
    int pos = -1;
    for(int i = 0; i < INODE_BITMAP_SIZE; i++) {
        unsigned char byte;
        fread(&byte, sizeof(unsigned char), 1, fp);
        for(int j = 0; j < 8; j++) {
            if(((byte >> j) & 1) == 0) {
                pos = i * 8 + j;
                break;
            }
        }
        if(pos != -1)
            break;
    }
    return pos;
}

int Filesystem::findAvailableBlock() {
    fseek(fp, BLOCK_BITMAP_START, SEEK_SET);
    int pos = -1;
    for(int i = 0; i < BLOCK_BITMAP_SIZE; i++) {
        unsigned char byte;
        fread(&byte, sizeof(unsigned char), 1, fp);
        for(int j = 0; j < 8; j++) {
            if(((byte >> j) & 1) == 0) {
                pos = i * 8 +j;
                break;
            }
        }
        if(pos != -1)
            break;
    }
    return pos;
}

int Filesystem::numberOfAvailableBlock() {
    int unused = 0;
    fseek(fp, BLOCK_BITMAP_START, SEEK_SET);
    for(int i = 0; i < BLOCK_BITMAP_SIZE; i++) {
        unsigned char byte;
        fread(&byte, sizeof(unsigned char), 1, fp);
        for(int j = 0; j < 8; j++) {
            if(((byte >> j) & 1) == 0)
                unused++;
        }
    }
    return unused;
}

void Filesystem::printInfo() {
    int unused = numberOfAvailableBlock();
    int used = BLOCK_NUM - unused;
    cout << "System Size: " << superblock.systemsize << " Bytes\n";
    cout << "Block Size: " << superblock.blocksize << " Bytes\n";
    cout << "INode Bitmap Size: " << superblock.inode_bitmap_size << " Bytes\n";
    cout << "Block Bitmap Size: " << superblock.inode_bitmap_size << " Bytes\n";
    cout << "INode Table Size: " << superblock.inode_table_size << " Bytes\n";
    cout << "Number of Blocks: " << superblock.blocknum << "\n";
    cout << "Number of Blocks that have been used: " << used << "\n";
    cout << "Number of Blocks that are available: " << unused << "\n";
}

void Filesystem::modifyInodeBitmap(int pos) {
    int origin = pos / 8;
    int offset = pos % 8;
    unsigned char byte;
    fseek(fp, INODE_BITMAP_START + origin, SEEK_SET);
    fread(&byte, sizeof(unsigned char), 1, fp);
    byte = byte ^ (1 << offset);
    fseek(fp, INODE_BITMAP_START + origin, SEEK_SET);
    fwrite(&byte, sizeof(unsigned char), 1, fp);
}

void Filesystem::modifyBlockBitmap(int pos) {
    int origin = pos / 8;
    int offset = pos % 8;
    unsigned char byte;
    fseek(fp, BLOCK_BITMAP_START + origin, SEEK_SET);
    fread(&byte, sizeof(unsigned char), 1, fp);
    byte = byte ^ (1 << offset);
    fseek(fp, BLOCK_BITMAP_START + origin, SEEK_SET);
    fwrite(&byte, sizeof(unsigned char), 1, fp);
}

void Filesystem::writeInode(int pos, INode inode) {
    fseek(fp, INODE_TABLE_START + INODE_SIZE * pos, SEEK_SET);
    fwrite(&inode, sizeof(INode), 1, fp);
}

INode Filesystem::readInode(int pos) {
    INode inode;
    fseek(fp, INODE_TABLE_START + INODE_SIZE * pos, SEEK_SET);
    fread(&inode, sizeof(INode), 1, fp);
    return inode;
}

void Filesystem::dir() {
    cur_Inode = readInode(cur_Inode.id);
    int cnt = cur_Inode.mcount;
    int FILE_PER_BLOCK = superblock.blocksize / sizeof(File);
    
    cout << left << setw(7) << "Mode";
    cout << right << setw(25) << "Created Time";
    cout << right << setw(17) << "Length(Bytes)";
    cout << " ";
    cout << left << setw(25) << "Name";
    cout << endl;
    cout << left << setw(7) << "----";
    cout << right << setw(25) << "-----------";
    cout << right << setw(17) << "-------------";
    cout << " ";
    cout << left << setw(25) << "----";
    cout << "\n";

    for(int i = 0; i < cur_Inode.NUM_DIRECT_ADDRESS; i++) {
        if(cnt == 0)
            break;
        for(int j = 0; j < FILE_PER_BLOCK; j++) {
            if(cnt == 0)
                break;
            cnt--;
            File file;
            fseek(fp, BLOCK_SIZE * cur_Inode.dir_address[i] + sizeof(File) * j, SEEK_SET);
            fread(&file, sizeof(File), 1, fp);
            if(file.inode_id == -1)
                continue;
            INode inode = readInode(file.inode_id);
            if(inode.fmode == DENTRY_MODE)
                cout << left << setw(7) << "Dentry";
            else
                cout << left << setw(7) << "File";
            char buffer[50];
            strftime(buffer, 40, "%a %b %d %T %G", localtime(&inode.ctime));
            cout << right << setw(25) << buffer;
            cout << right << setw(17) << inode.fsize * 1024;
            cout << " ";
            cout << left << setw(25) << file.filename;
            cout << "\n";
        }
    }
}

State Filesystem::createFile(std::string fileName, int fileSize) {
    int len = (int)fileName.size();
    INode inode;

    if(fileName[0] == '/') {
        inode = root_Inode;
    }
    else {
        inode = cur_Inode;
    }
    vector<string> v;
    string temp = "";
    for(int i = 0; i < len; i++) {
        if(fileName[i] == '/') {
            if(temp != "") {
                v.push_back(temp);
                temp = "";
            }
            continue;
        }
        temp += fileName[i];
    }
    if(temp == "")
        return NO_FILENAME;
    if((int)temp.size() >= MAX_FILENAME_SIZE)
        return LENGTH_EXCEED;

    v.push_back(temp);
    
    int cnt = (int)v.size();

    for(int i = 0; i < cnt - 1; i++) {
        bool ok = true;
        INode nxtInode = findNextInode(inode, v[i], ok);
        if(nxtInode.fmode != DENTRY_MODE)
            ok = false;
        if(!ok)
            return DIR_NOT_EXIST;
        inode = nxtInode;
    }
    bool ok = true;
    INode nxtInode = findNextInode(inode, v[cnt-1], ok);
    if(ok)
        return FILE_EXISTS;

    int SUM_OF_DIRECTORY = superblock.blocksize / sizeof(File) * INode::NUM_DIRECT_ADDRESS;
    if(inode.count >= SUM_OF_DIRECTORY)
        return DIRECTORY_EXCEED;

    int unused = numberOfAvailableBlock();
    if(unused < fileSize)
        return NO_ENOUGH_SPACE;
    if(fileSize > MAX_FILE_SIZE)
        return NO_ENOUGH_SPACE;

    File file;
    file.inode_id = findAvailableInode();
    modifyInodeBitmap(file.inode_id);
    strcpy(file.filename, v[cnt-1].c_str());

    INode newInode;
    newInode.clear();
    newInode.fsize = fileSize;
    newInode.ctime = time(NULL);
    newInode.fmode = FILE_MODE;
    newInode.id = file.inode_id;

    writeFileToDentry(file, inode);

    int temp_fileSize = fileSize;

    for(int i = 0; i < INode::NUM_DIRECT_ADDRESS; i++) {
        if(temp_fileSize == 0)
            break;
        temp_fileSize--;
        newInode.dir_address[i] = findAvailableBlock();
        modifyBlockBitmap(newInode.dir_address[i]);
        writeRandomStringToBlock(newInode.dir_address[i]);
    }
    if(temp_fileSize > 0) {
        newInode.indir_address[0] = findAvailableBlock();
        modifyBlockBitmap(newInode.indir_address[0]); // indirect address
        int cnt = 0;
        while(temp_fileSize>0) {
            temp_fileSize--;
            int blockid = findAvailableBlock();
            modifyBlockBitmap(blockid);
            Address address;
            address.setblockID(blockid);
            address.setOffset(0);
            writeRandomStringToBlock(blockid);
            writeAddressToBlock(address, newInode.indir_address[0], cnt);
            cnt++;
        }
    }

    writeInode(newInode.id, newInode);
    return SUCCESS;
}

void Filesystem::writeRandomStringToBlock(int blockid) {
    srand(time(0));
    fseek(fp, blockid*superblock.blocksize, SEEK_SET);
    for(int i = 0; i < superblock.blocksize; i++) {
        char randomChar = (rand() % 26) + 'a';
        fwrite(&randomChar, sizeof(char), 1, fp);
    }
}

void Filesystem::writeAddressToBlock(Address address, int blockid, int offset) {
    fseek(fp, blockid * superblock.blocksize + offset * sizeof(Address), SEEK_SET);
    fwrite(&address, sizeof(Address), 1, fp);
}

void Filesystem::writeFileToDentry(File file, INode inode) {
    int cnt = inode.count;
    int FILE_PER_BLOCK = superblock.blocksize / sizeof(File);

    if(inode.mcount == cnt) {
        if(cnt % FILE_PER_BLOCK == 0) {
            inode.dir_address[cnt / FILE_PER_BLOCK] = findAvailableBlock();
            modifyBlockBitmap(inode.dir_address[cnt / FILE_PER_BLOCK]);
            fseek(fp, BLOCK_SIZE * inode.dir_address[cnt / FILE_PER_BLOCK], SEEK_SET);
        }
        else {
            fseek(fp, BLOCK_SIZE * inode.dir_address[cnt / FILE_PER_BLOCK] + sizeof(File) * (cnt % FILE_PER_BLOCK), SEEK_SET);
        }

        fwrite(&file, sizeof(File), 1, fp);

        inode.count++;
        inode.mcount++;
        writeInode(inode.id, inode);
    }
    else {
        bool ok = false;
        int temp = inode.mcount;
        for(int i = 0; i < inode.NUM_DIRECT_ADDRESS; i++) {
            if(temp == 0)
                break;
            for(int j = 0; j < FILE_PER_BLOCK; j++) {
                if(temp == 0)
                    break;
                temp--;
                File tempfile;
                fseek(fp, BLOCK_SIZE * inode.dir_address[i] + sizeof(File) * j, SEEK_SET);
                fread(&tempfile, sizeof(File), 1, fp);
                if(tempfile.inode_id == -1) {
                    ok = true;
                    fseek(fp, BLOCK_SIZE * inode.dir_address[i] + sizeof(File) * j, SEEK_SET);
                    fwrite(&file, sizeof(File), 1, fp);
                }
                if(ok)
                    break;
            }
            if(ok)
                break;
        }
        
        inode.count++;
        writeInode(inode.id, inode);
    }
    if(inode.id == cur_Inode.id)
        cur_Inode = readInode(cur_Inode.id);
    if(inode.id == root_Inode.id)
        root_Inode = readInode(root_Inode.id);
}

INode Filesystem::findNextInode(INode inode, std::string fileName, bool &canFind) {
    int cnt = inode.mcount;
    int FILE_PER_BLOCK = superblock.blocksize / sizeof(File);
    for(int i = 0; i < inode.NUM_DIRECT_ADDRESS; i++) {
        if(cnt == 0)
            break;
        fseek(fp, BLOCK_SIZE * inode.dir_address[i], SEEK_SET);
        for(int j = 0; j < FILE_PER_BLOCK; j++) {
            if(cnt == 0)
                break;
            cnt--;
            File file;
            fread(&file, sizeof(File), 1, fp);
            if(file.inode_id == -1)
                continue;
            if(strcmp(file.filename, fileName.c_str()) == 0) {
                return readInode(file.inode_id);
            }
        }
    }
    canFind = false;
    return inode;
}

void Filesystem::deleteFileFromDentry(INode inode, std::string fileName) {
    int cnt = inode.mcount;
    int FILE_PER_BLOCK = superblock.blocksize / sizeof(File);
    bool ok = false;
    for(int i = 0; i < inode.NUM_DIRECT_ADDRESS; i++) {
        if(cnt == 0)
            break;
        fseek(fp, BLOCK_SIZE*inode.dir_address[i], SEEK_SET);
        for(int j = 0; j < FILE_PER_BLOCK; j++) {
            if(cnt == 0)
                break;
            cnt--;
            File file;
            fread(&file, sizeof(File), 1, fp);
            if(file.inode_id == -1)
                continue;
            if(strcmp(file.filename, fileName.c_str()) == 0) {
                fseek(fp, -sizeof(File), SEEK_CUR);
                file.inode_id = -1;
                fwrite(&file, sizeof(File), 1, fp);
                ok = true;
            }
            if(ok)
                break;
        }
        if(ok)
            break;
    }
    inode.count--;
    writeInode(inode.id, inode);
}

State Filesystem::deleteFile(std::string fileName) {
    int len = (int)fileName.size();
    INode inode;
    if(fileName[0] == '/') {
        inode = root_Inode;
    }
    else {
        inode = cur_Inode;
    }
    vector<string> v;
    string temp = "";
    for(int i = 0; i < len; i++) {
        if(fileName[i] == '/') {
            if(temp != "") {
                v.push_back(temp);
                temp = "";
            }
            continue;
        }
        temp += fileName[i];
    }
    if(temp == "")
        return NO_SUCH_FILE;
    v.push_back(temp);

    int cnt = (int)v.size();

    for(int i = 0; i < cnt - 1; i++) {
        bool ok = true;
        INode nxtInode = findNextInode(inode, v[i], ok);
        if(nxtInode.fmode != DENTRY_MODE)
            ok = false;
        if(!ok)
            return NO_SUCH_FILE;
        inode = nxtInode;
    }

    bool ok = true;
    INode nextInode = findNextInode(inode, v[cnt - 1], ok);
    if(nextInode.fmode == DENTRY_MODE)
        ok = false;
    if(!ok)
        return NO_SUCH_FILE;

    deleteFileFromDentry(inode, v[cnt - 1]);

    inode = nextInode;
    modifyInodeBitmap(inode.id);
    int filesize = inode.fsize;
    for(int i = 0; i < inode.NUM_DIRECT_ADDRESS; i++) {
        if(filesize == 0)
            break;
        filesize--;
        modifyBlockBitmap(inode.dir_address[i]);
    }
    if(filesize > 0) {
        modifyBlockBitmap(inode.indir_address[0]);
        int offset = 0;
        while(filesize > 0) {
            filesize--;
            Address address;
            fseek(fp, inode.indir_address[0] * superblock.blocksize + offset * sizeof(Address), SEEK_SET);
            fread(&address, sizeof(Address), 1, fp);
            modifyBlockBitmap(address.getblockID());
            offset++;
        }
    }
    return SUCCESS;
}

State Filesystem::createDir(std::string dirName) {
    int len = (int)dirName.size();
    INode inode;
    if(dirName[0] == '/') {
        inode = root_Inode;
    }
    else {
        inode = cur_Inode;
    }
    vector<string> v;
    string temp = "";
    for(int i = 0; i < len; i++) {
        if(dirName[i] == '/') {
            if(temp != "") {
                v.push_back(temp);
                temp = "";
            }
            continue;
        }
        temp += dirName[i];
    }
    if(temp == "")
        return NO_DIRNAME;
    if((int)temp.size() >= MAX_FILENAME_SIZE)
        return LENGTH_EXCEED;

    v.push_back(temp);
    
    int cnt = (int)v.size();

    for(int i = 0; i < cnt-1; i++) {
        bool ok = true;
        INode nxtInode = findNextInode(inode, v[i], ok);
        if(nxtInode.fmode != DENTRY_MODE)
            ok = false;
        if(!ok)
            return DIR_NOT_EXIST;
        inode = nxtInode;
    }
    bool ok = true;
    INode nextInode = findNextInode(inode, v[cnt - 1], ok);
    if(ok)
        return DIR_EXISTS;

    int SUM_OF_DIRECTORY = superblock.blocksize / sizeof(File) * INode::NUM_DIRECT_ADDRESS;
    if(inode.count >= SUM_OF_DIRECTORY)
        return DIRECTORY_EXCEED;

    File file;
    file.inode_id = findAvailableInode();
    modifyInodeBitmap(file.inode_id);
    strcpy(file.filename, v[cnt - 1].c_str());

    INode newInode;
    newInode.clear();
    newInode.fsize = 0;
    newInode.ctime = time(NULL);
    newInode.fmode = DENTRY_MODE;
    newInode.id = file.inode_id;

    writeFileToDentry(file, inode);

    writeInode(newInode.id, newInode);
    return SUCCESS;
}

State Filesystem::deleteDir(std::string dirName) {
    int len = (int)dirName.size();
    INode inode;
    if(dirName[0] == '/') {
        inode = root_Inode;
    }
    else {
        inode = cur_Inode;
    }
    vector<string> v;
    string temp = "";
    for(int i = 0; i < len; i++) {
        if(dirName[i] == '/') {
            if(temp != "") {
                v.push_back(temp);
                temp = "";
            }
            continue;
        }
        temp += dirName[i];
    }
    if(temp == "")
        return NO_SUCH_DIR;
    v.push_back(temp);

    int cnt = (int)v.size();

    for(int i = 0; i < cnt - 1; i++) {
        bool ok = true;
        INode nextInode = findNextInode(inode, v[i], ok);
        if(nextInode.fmode != DENTRY_MODE)
            ok = false;
        if(!ok)
            return NO_SUCH_DIR;
        inode = nextInode;
    }

    bool ok = true;
    INode nextInode = findNextInode(inode, v[cnt - 1], ok);
    if(nextInode.fmode == FILE_MODE)
        ok = false;
    if(!ok)
        return NO_SUCH_DIR;

    if(dirName[0] == '/') {
        vector<string> cur;
        string tempdir = "";
        int sz = strlen(curpath);
        for(int i = 1; i < sz; i++) {
            if(curpath[i] == '/') {
                if(tempdir != "") {
                    cur.push_back(tempdir);
                    tempdir = "";
                }
                continue;
            }
            tempdir += curpath[i];
        }
        if(tempdir != "")
            cur.push_back(tempdir);

        int curlen = (int)cur.size();
        if(cnt <= curlen) {
            bool ok = true;
            for(int i = 0; i < cnt; i++) {
                if(v[i] != cur[i]) {
                    ok = false;
                    break;
                }
            }
            if(ok)
                return CAN_NOT_DELETE_TEMP_DIR;
        }
    }

    if(nextInode.count > 0)
        return DIR_NOT_EMPTY;

    deleteFileFromDentry(inode, v[cnt - 1]);

    inode = nextInode;
    modifyInodeBitmap(inode.id);
    int count = inode.mcount;
    int FILE_PER_BLOCK = superblock.blocksize / sizeof(File);
    for(int i = 0; i < inode.NUM_DIRECT_ADDRESS; i++) {
        if(count <= 0)
            break;
        count -= FILE_PER_BLOCK;
        modifyBlockBitmap(inode.dir_address[i]);
    }
    return SUCCESS;
}

State Filesystem::changeDir(std::string path) {
    int len = (int)path.size();

    if(path == "/") {
        cur_Inode = root_Inode;
        strcpy(curpath, root_dir);
        return SUCCESS;
    }

    INode inode;
    if(path[0] == '/') {
        inode = root_Inode;
    }
    else {
        inode = cur_Inode;
    }
    vector<string> v;
    string temp = "";
    for(int i = 0; i < len; i++) {
        if(path[i] == '/') {
            if(temp != "") {
                v.push_back(temp);
                temp = "";
            }
            continue;
        }
        temp += path[i];
    }
    if(temp == "")
        return NO_DIRNAME;
    if((int)temp.size() >= MAX_FILENAME_SIZE)
        return LENGTH_EXCEED;

    v.push_back(temp);

    int cnt = (int)v.size();

    for(int i = 0; i < cnt - 1; i++) {
        bool ok = true;
        INode nextInode = findNextInode(inode, v[i], ok);
        if(nextInode.fmode != DENTRY_MODE)
            ok = false;
        if(!ok)
            return DIR_NOT_EXIST;
        inode = nextInode;
    }
    bool ok = true;
    INode nextInode = findNextInode(inode, v[cnt - 1], ok);
    if(nextInode.fmode != DENTRY_MODE)
        ok = false;
    if(!ok)
        return DIR_NOT_EXIST;
    
    cur_Inode = nextInode;

    if(path[0] == '/') {
        strcpy(curpath, root_dir);
        for(int i = 0; i < cnt; i++) {
            strcat(curpath, "/");
            strcat(curpath, v[i].c_str());
        }
    }
    else {
        for(int i = 0; i < cnt; i++) {
            strcat(curpath, "/");
            strcat(curpath, v[i].c_str());
        }
    }
    return SUCCESS;
}

State Filesystem::cat(std::string fileName) {
    int len = (int)fileName.size();
    INode inode;

    if(fileName[0] == '/') {
        inode = root_Inode;
    }
    else {
        inode = cur_Inode;
    }
    vector<string> v;
    string temp = "";
    for(int i = 0; i < len; i++) {
        if(fileName[i] == '/') {
            if(temp != "") {
                v.push_back(temp);
                temp = "";
            }
            continue;
        }
        temp += fileName[i];
    }
    if(temp == "")
        return NO_FILENAME;

    v.push_back(temp);
    
    int cnt = (int)v.size();

    for(int i = 0; i < cnt-1; i++) {
        bool ok = true;
        INode nxtInode = findNextInode(inode, v[i], ok);
        if(nxtInode.fmode != DENTRY_MODE)
            ok = false;
        if(!ok)
            return DIR_NOT_EXIST;
        inode = nxtInode;
    }
    bool ok = true;
    INode nxtInode = findNextInode(inode, v[cnt-1], ok);
    if(nxtInode.fmode == DENTRY_MODE)
        ok = false;
    if(!ok)
        return NO_SUCH_FILE;
    
    inode = nxtInode;

    int fileSize = inode.fsize;

    for(int i = 0; i < inode.NUM_DIRECT_ADDRESS; i++) {
        if(fileSize == 0)
            break;
        fileSize--;
        int blockid = inode.dir_address[i];
        fseek(fp, blockid * superblock.blocksize, SEEK_SET);
        for(int j = 0; j < BLOCK_SIZE; j++) {
            char buffer;
            fread(&buffer, sizeof(char), 1, fp);
            cout << buffer;
        }
    }
    if(fileSize > 0) {
        int offset = 0;
        while(fileSize > 0) {
            fileSize--;
            Address address;
            fseek(fp, inode.indir_address[0] * superblock.blocksize + offset * sizeof(Address), SEEK_SET);
            fread(&address, sizeof(Address), 1, fp);
            int blockid = address.getblockID();
            fseek(fp, blockid * superblock.blocksize, SEEK_SET);
            for(int j = 0; j < BLOCK_SIZE; j++) {
                char buffer;
                fread(&buffer, sizeof(char), 1, fp);
                cout << buffer;
            }
            offset++;
        }
    }
    cout << "\n";
    return SUCCESS;
}

State Filesystem::cp(std::string file1, std::string file2) {
    int len = (int)file1.size();
    INode inode;

    if(file1[0] == '/') {
        inode = root_Inode;
    }
    else {
        inode = cur_Inode;
    }
    vector<string> v;
    string temp = "";
    for(int i = 0; i < len; i++) {
        if(file1[i] == '/') {
            if(temp != "") {
                v.push_back(temp);
                temp = "";
            }
            continue;
        }
        temp += file1[i];
    }
    if(temp == "")
        return NO_FILENAME;

    v.push_back(temp);
    
    int cnt = (int)v.size();

    for(int i = 0; i < cnt - 1; i++) {
        bool ok = true;
        INode nextInode = findNextInode(inode, v[i], ok);
        if(nextInode.fmode != DENTRY_MODE)
            ok = false;
        if(!ok)
            return DIR_NOT_EXIST;
        inode = nextInode;
    }
    bool ok = true;
    INode nextInode = findNextInode(inode, v[cnt - 1], ok);
    if(nextInode.fmode == DENTRY_MODE)
        ok = false;
    if(!ok)
        return NO_SUCH_FILE;
    
    inode = nextInode;

    vector<char> content;
    int fileSize = inode.fsize;
    for(int i = 0; i < inode.NUM_DIRECT_ADDRESS; i++) {
        if(fileSize == 0) {
            break;
        }
        fileSize--;
        int blockid = inode.dir_address[i];
        fseek(fp, blockid * superblock.blocksize, SEEK_SET);
        for(int j = 0; j < BLOCK_SIZE; j++) {
            char buffer;
            fread(&buffer, sizeof(char), 1, fp);
            content.push_back(buffer);
        }
    }
    if(fileSize > 0) {
        int offset = 0;
        while(fileSize > 0) {
            fileSize--;
            Address address;
            fseek(fp, inode.indir_address[0] * superblock.blocksize + offset * sizeof(Address), SEEK_SET);
            fread(&address, sizeof(Address), 1, fp);
            int blockid = address.getblockID();
            fseek(fp, blockid * superblock.blocksize, SEEK_SET);
            for(int j = 0; j < BLOCK_SIZE; j++) {
                char buffer;
                fread(&buffer, sizeof(char), 1, fp);
                content.push_back(buffer);
            }
            offset++;
        }
    }

    State state = createFile(file2, inode.fsize);
    if(state != SUCCESS) return state;

    len = (int)file2.size();

    if(file2[0] == '/') {
        inode = root_Inode;
    }
    else {
        inode = cur_Inode;
    }
    v.clear();
    temp = "";
    for(int i = 0; i < len; i++) {
        if(file2[i] == '/') {
            if(temp != "") {
                v.push_back(temp);
                temp = "";
            }
            continue;
        }
        temp += file2[i];
    }
    if(temp == "")
        return NO_FILENAME;

    v.push_back(temp);
    
    cnt = (int)v.size();

    for(int i = 0; i < cnt - 1; i++) {
        bool ok = true;
        INode nextInode = findNextInode(inode, v[i], ok);
        if(nextInode.fmode != DENTRY_MODE)
            ok = false;
        if(!ok)
            return DIR_NOT_EXIST;
        inode = nextInode;
    }
    ok = true;
    nextInode = findNextInode(inode, v[cnt - 1], ok);
    if(nextInode.fmode == DENTRY_MODE)
        ok = false;
    if(!ok)
        return NO_SUCH_FILE;

    inode = nextInode;

    fileSize = inode.fsize;
    int index = 0;
    for(int i = 0; i < inode.NUM_DIRECT_ADDRESS; i++) {
        if(fileSize == 0)
            break;
        fileSize--;
        int blockid = inode.dir_address[i];
        fseek(fp, blockid * superblock.blocksize, SEEK_SET);
        for(int j = 0; j < BLOCK_SIZE; j++) {
            char buffer = content[index++];
            fwrite(&buffer, sizeof(char), 1, fp);
        }
    }
    if(fileSize > 0) {
        int offset = 0;
        while(fileSize > 0) {
            fileSize--;
            Address address;
            fseek(fp, inode.indir_address[0] * superblock.blocksize + offset * sizeof(Address), SEEK_SET);
            fread(&address, sizeof(Address), 1, fp);
            int blockid = address.getblockID();
            fseek(fp, blockid * superblock.blocksize, SEEK_SET);
            for(int j = 0; j < BLOCK_SIZE; j++) {
                char buffer = content[index++];
                fwrite(&buffer, sizeof(char), 1, fp);
            }
            offset++;
        }
    }
    return SUCCESS;
}

void Filesystem::giveState(std::string command, State st){
    if(st != SUCCESS)
        cout << "Command \'" << command << "\'";
	if(st == DIR_NOT_EXIST)
		cout << "The directory does not exist!\n";
    else if(st == NO_FILENAME)
		cout << "No file name provided!\n";
    else if(st == FILE_EXISTS)
		cout << "The file has existed!\n";
    else if(st == LENGTH_EXCEED)
		cout << "The length of the file name is exceed!\n";
    else if(st == DIRECTORY_EXCEED)
		cout << "Too many files in the directory!\n";
    else if(st == NO_ENOUGH_SPACE)
		cout << "The system has no enough space for the file!\n";
    else if(st == NO_SUCH_FILE)
		cout << "No such file!\n";
    else if(st == NO_DIRNAME)
		cout << "No directory name provided!\n";
    else if(st == NO_SUCH_DIR)
		cout << "No such directory!\n";
    else if(st == DIR_NOT_EMPTY)
		cout << "The directory is not empty!\n";
    else if(st == CAN_NOT_DELETE_TEMP_DIR)
		cout << "You cannot delete the current directory!\n";
    else if(st == DIR_EXISTS)
		cout << "The directory has existed!\n";
}