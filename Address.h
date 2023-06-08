#ifndef ADDRESS_H
#define ADDRESS_H

#define BLOCKID_SIZE 14
#define BLOCKOFFSET_SIZE 10

class Address{
public:
	unsigned char Address_info[3];  // 地址占3个字节，每个字节8 bits，即24 bits

public:
	Address();
	~Address();
	int getblockID();
	void setblockID(int id);
	int getOffset();
	void setOffset(int offset);
};

#endif