#ifndef _CHUCK_SUM_
#define _CHUCK_SUM_

#include "Defines.h"

#define CheckSumOffset 0x0E

#define DatafileCheckSumOffset 0x10

int do_16_byte_xor(BYTE *block1, BYTE *block2, BYTE *out);

//��Ϊ0ʱ���˿鲻��ȷ
WORD do_checksum(int block_size, BYTE *buffer);

WORD GetCheckSum(BYTE *buffer);

bool FixedCheckSum(int block_size,BYTE *buffer,int XORvalue);

#endif

