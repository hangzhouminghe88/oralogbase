#include "ChuckSum.h"
#include <string.h>
#include <stdio.h>

int do_16_byte_xor(BYTE *block1, BYTE *block2, BYTE *out)
{
	int c = 0;
	while (c<16)
	{
		out[c] = block1[c] ^ block2[c];
		c++;
	}
	return 0;
}

WORD do_checksum(int block_size, BYTE *buffer)
{
	BYTE block1[16]="";
	BYTE block2[16]="";
	BYTE block3[16]="";
	BYTE block4[16]="";
	BYTE out1[16]="";
	BYTE out2[16]="";
	BYTE res[16]="";
	BYTE nul[16]="";
	int count = 0;
	unsigned int r0=0,r1=0,r2=0,r3=0,r4=0;
	while(count < block_size)
	{
		memmove(block1,&buffer[count],16);
		memmove(block2,&buffer[count+16],16);
		memmove(block3,&buffer[count+32],16);
		memmove(block4,&buffer[count+48],16);
		do_16_byte_xor(block1,block2,out1);
		do_16_byte_xor(block3,block4,out2);
		do_16_byte_xor(nul,out1,res);
		memmove(nul,res,16);
		do_16_byte_xor(nul,out2,res);
		memmove(nul,res,16);
		count = count + 64;
	}
	memmove(&r1,&res[0],4);
	memmove(&r2,&res[4],4);
	memmove(&r3,&res[8],4);
	memmove(&r4,&res[12],4);
	r0 = r0 ^ r1;
	r0 = r0 ^ r2;
	r0 = r0 ^ r3;
	r0 = r0 ^ r4;
	r1 = r0;
	r0 = r0 >> 16;
	r0 = r0 ^ r1;
	r0 = r0 & 0xFFFF;
	return r0;
}

// µÃµ½CheckSum
WORD GetCheckSum(BYTE *buffer)
{
	WORD RetCheckSum = 0x00;
	memcpy(&RetCheckSum,buffer+DatafileCheckSumOffset,sizeof(RetCheckSum));
	return RetCheckSum;	
}

bool FixedCheckSum(int block_size,BYTE *buffer,int XORvalue)
{
	if(XORvalue == 0x00)
		return true;
	
	WORD RetCheckSum = GetCheckSum(buffer);
	WORD NewCheckSum = RetCheckSum ^ XORvalue;

	memcpy(buffer+DatafileCheckSumOffset,&NewCheckSum,sizeof(NewCheckSum));

	if (do_checksum(block_size,buffer)){
		return false;
	}
	return true;
}
