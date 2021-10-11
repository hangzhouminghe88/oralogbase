#ifndef _LGC_ORA_CONVERT_H
#define _LGC_ORA_CONVERT_H
#include "oratypes.h"

#define DATE_STR_LEN 20
#define EXT_ROWID_STR_LEN 19
#define MAX_TS_COUNT 4


//////////////////////////////some define//////////////////

typedef signed char sb1;

#define TRUE 1
#define FALSE 0

struct COMMON_DATA
{
	char* data;
	char* str_data;
};
typedef struct COMMON_DATA COMMON_DATA;

struct COL_DATA
{
	ub1 mem_alloc;
	COMMON_DATA common_data;
	ub4 data_len;
	ub1 is_null;

	COL_DATA(){
		memset(this, 0, sizeof(COL_DATA));
	}
	~COL_DATA(){
		if(mem_alloc == TRUE){
			free(this->common_data.data);
			this->common_data.data = NULL;
			this->common_data.str_data = NULL;
		}
	}
};
typedef struct COL_DATA COL_DATA;

typedef COL_DATA* PCOL_DATA;

//////////////////////////////////////////////////////////


//////////////////////////////trace info///////////////////
#define TL_MIN_TRACE 0
struct CONFIG_DATA
{
        ub4 trace_level;
};
typedef struct CONFIG_DATA  CONFIG_DATA;


extern CONFIG_DATA config_data;
///////////////////////////////////////////////////////////








void raw_to_date(ub1 *buffer,ub4 len,PCOL_DATA col_data);
void raw_to_timestamp(ub1 *buffer,ub4 len,PCOL_DATA col_data,ub2 scale);
void raw_to_timestamp_tz(ub1 *buffer,ub4 len,PCOL_DATA col_data,ub2 scale);
void raw_to_timestamp_ltz(ub1 *buffer,ub4 len,PCOL_DATA col_data,ub2 scale);


void raw_to_number(ub1 *buffer,ub4 len,PCOL_DATA col_data);
void raw_to_binary_float(ub1 *buffer,ub4 len,PCOL_DATA col_data);
void raw_to_binary_double(ub1 *buffer,ub4 len,PCOL_DATA col_data);

void raw_to_char(ub1 *buffer,ub4 len,PCOL_DATA col_data);

void raw_to_raw(ub1 *buffer,ub4 len,PCOL_DATA col_data);
void raw_to_hex(ub1 *buffer,ub4 len,PCOL_DATA col_data);

void raw_to_interval_ym(ub1 *buffer, ub4 len, PCOL_DATA col_data);
void raw_to_interval_ds(ub1 *buffer, ub4 len, PCOL_DATA col_data);

void fill_2_hex(char* buffer, ub1 c);

void raw_to_rowid(ub1 *buffer,ub4 len,PCOL_DATA col_data);
int decode_base64(char *buf,int len,ub4 *result);
void encode_rowid(char *rowid,ub4 obj_no,ub2 file_no,ub4 block_no,ub2 row_no);
void encode_base64(char *buf,int len,ub4 value);
void encode_short_rowid(char *rowid,ub2 file_no,ub4 block_no,ub2 row_no);


int check_date(ub1 *buffer);




//////////////////////////

void fill_2_num(char* buffer, ub1 c);

int sp_printf(const char *format, ...);

//ub4 get_ub4_big(ub1* buffer, ub1 isBig);

 void get_ub8_big(ub1* _buffer,ub1 offset, ub8* pResult);
ub4 get_ub4_big(ub1* buffer, ub1 offset);
ub2 get_ub2_big(ub1* buffer, ub1 offset);
 
ub2 rdba_file_no(ub4 rdba,ub4 max);
ub4 rdba_block_no(ub4 rdba,ub4 max);
 /////////
int strcmp_igNum(const char* str1, const char* str2);
ub2 getScaleInTimestamp(const char* colDataType);
#endif
