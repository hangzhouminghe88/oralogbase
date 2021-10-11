#ifndef LGC_STRUCTURE
#define LGC_STRUCTURE
#include "lgc_define.h"

#ifdef DEF_ORACLE_9I
struct log_block_header {
    unsigned int	log_file_sequence;            /* 0x00 */
    unsigned int	block_id;					  /* 0x04 */
    unsigned int	epoch;						  /* 0x08 */
    unsigned short	first_new_record_offset;      /* 0x0c */
    unsigned short	crc;						  /* 0x0e */

	unsigned short firstNewRecordOffset(){
		return first_new_record_offset;
	}
};
typedef struct log_block_header log_block_header;
#else
struct log_block_header
{
    unsigned int   block_type;  //unknow
    unsigned int   block_id;            //the block index
    unsigned int   log_file_sequence;   //the logfile sequence number
                                            //In Oracle 10+ the ms bit of the this field is set to 1. Possibly indicating block validity.
    unsigned short  first_new_record_offset;          // the byte offset of the first new record header in the block
    unsigned short crc;     
	
	unsigned short firstNewRecordOffset(){
		return (first_new_record_offset^0x8000);
	}
};
typedef struct log_block_header log_block_header;
#endif

struct log_file_header
{
	unsigned char      unknow[16];
    unsigned char      zero0;
    unsigned char      type; //不准确，在windows下和aix下不一样
    unsigned short     zero1;
    unsigned int       block_size; //一个block的大小
    unsigned int       block_count; //后续的block的个数
    unsigned char      reserved[484]; //不用解析
};
typedef struct log_file_header log_file_header;


struct log_redo_header 
{ 
    log_block_header    block_head;
    unsigned int        db_version;         /* 0x10 (block offset including block header) */
    unsigned int        compatibility_version;    /* 0x14 */
    unsigned int        db_id;                    /* 0x18 */
    char                db_name[8];             /* 0x1c */
    unsigned int        control_sequence;         /* 0x24 */
    unsigned int        file_size;                /* 0x28 */
    unsigned int        block_size;               /* 0x2c */
    unsigned short      file_number;            /* 0x30 */
    unsigned short      file_type;              /* 0x32 */
    unsigned int        activation_id;              /* 0x34 */ //后面的字段先不关心
    unsigned char       empty_38h[0x24];         /* 0x38 */
    char                description[0x40];         /* 0x5c */
    unsigned int        nab;                      /* 0x9c - final block count */
    unsigned int        reset_log_count;          /* 0xa0 */
    unsigned int        scn_minor;                /* 0xa4 */ 
    // The following two fields may be a single 32 bit field, 
    //this looks like the case on a bigendian dump.
    unsigned short      scn_major;              /* 0xa8 */
    unsigned char       empty_aah[2];            /* 0xaa */
    unsigned int        hws;                      /* 0xac */
    unsigned short      thread;                 /* 0xb0 */
    unsigned char       empty_b2h[2];            /* 0xb2 */
    unsigned int        low_scn_minor;            /* 0xb4 */
    unsigned short      low_scn_major;          /* 0xb8 */
    unsigned char       empty_bah[2];            /* 0xba */
    unsigned int        low_scn_epoch;            /* 0xbc */
    unsigned int        next_scn_minor;           /* 0xc0 */
    unsigned short      next_scn_major;         /* 0xc4 */
    unsigned char       empty_c6h[2];            /* 0xc6 */
    unsigned int        next_scn_epoch;           /* 0xc8 */
    unsigned char       eot;                     /* 0xcc */
    unsigned char       dis;                     /* 0xcd */
    unsigned char       empty_ceh[2];            /* 0xce */
    unsigned int        enabled_scn_minor;        /* 0xd0 */
    unsigned short      enabled_scn_major;      /* 0xd4 */
    unsigned char       empty_d6h[2];            /* 0xd6 */
    unsigned int        enabled_scn_epoch;        /* 0xd8 */
    unsigned int        thread_closed_scn_minor;  /* 0xdc */
    unsigned short      thread_closed_scn_major;/* 0xe0 */
    unsigned char       empty_e2h[2];            /* 0xe2 */
    unsigned int        thread_closed_scn_epoch;  /* 0xe4 */
    unsigned int        log_format_version;       /* 0xe8 */
    unsigned int        flags;                    /* 0xec */
    unsigned int        terminal_scn_minor;       /* 0xf0 */
    unsigned short      terminal_scn_major;     /* 0xf4 */
    unsigned char       empty_f6h[2];            /* 0xf6 */
    unsigned int        terminal_epoch;           /* 0xf8 */
	unsigned char       unkown_tw[260];
};
typedef struct log_redo_header log_redo_header;



/* Used when valid == 0x01, 0x02... maybe anything else not used above. */
//struct log_record_header_minor 
//{
//   unsigned int    record_length;   /* 00h */
//    unsigned char   valid;           /* 04h */
//    unsigned char   _foo[3];         /* 05h */
//    unsigned int    scn_minor;       /* 08h */
//    unsigned short  subscn;          /* 0ch */
//    unsigned short  unknown_1;       /* 0eh */
//    unsigned short  scn_major;       /* 10h */
//    unsigned short  _padding_1;      /* 12h */
//    unsigned int    _padding_2;      /* 14h */
//};
//typedef struct log_record_header_minor log_record_header_minor;

/* Used when valid == 0x04, 0x05, 0x06, 0x09, 0x0d */
//struct log_record_header_major 
//{
 //   unsigned int    record_length;   /* 00h */
//    unsigned char   valid;           /* 04h */
//    unsigned char   _foo[3];         /* 05h */
//    unsigned int    scn_minor;       /* 08h */
//    unsigned short  subscn;          /* 0ch */
//    unsigned short  unknown_1;       /* 0eh */
//    unsigned short  scn_major;       /* 10h */
//    unsigned short  foo;             /* 12h */
//    int             unknown_2[10];   /* 14h */
//    int             thread;          /* 3ch */
//    unsigned int    epoch;           /* 40h */
//};
//typedef struct log_record_header_major  log_record_header_major;

#ifdef DEF_ORACLE_9I
struct log_record_header_minor {
    unsigned int	 record_length;  /* 0x00 */
    unsigned char	valid;           /* 0x04 */
    unsigned char	subscn;          /* 0x05 */
    unsigned short	scn_major;       /* 0x06 */
    unsigned int	scn_minor;       /* 0x08 */
};
typedef struct log_record_header_minor log_record_header_minor;

struct log_record_header_major {
    unsigned int	record_length;   /* 0x00 */
    unsigned char	valid;           /* 0x04 */
    unsigned char	subscn;          /* 0x05 */
    unsigned short	scn_major;       /* 0x06 */
    unsigned int	scn_minor;       /* 0x08 */
};
typedef struct log_record_header_major log_record_header_major;

#else
/* Used when valid == 0x01, 0x02... maybe anything else not used above. */
struct log_record_header_minor 
{
    unsigned int    record_length;   /* 00h */
    unsigned char   valid;           /* 04h */
    unsigned char   _foo[1];         /* 05h */
	unsigned short  scn_major;		 /* 06h */			//change  by tw,lzg 20150531
    unsigned int    scn_minor;       /* 08h */
    unsigned short  subscn;          /* 0ch */
    unsigned short  unknown_1;       /* 0eh */
    unsigned short  unknown_3;       /* 10h */			//change by tw.lzg		
    unsigned short  _padding_1;      /* 12h */
    unsigned int    _padding_2;      /* 14h */
};
typedef struct log_record_header_minor log_record_header_minor;

/* Used when valid == 0x04, 0x05, 0x06, 0x09, 0x0d */
struct log_record_header_major 
{
    unsigned int    record_length;   /* 00h */
    unsigned char   valid;           /* 04h */
    unsigned char   _foo[1];         /* 05h */
	unsigned short  scn_major;		 /* 06h */			//change  by tw,lzg 20150531
    unsigned int    scn_minor;       /* 08h */
    unsigned short  subscn;          /* 0ch */
    unsigned short  unknown_1;       /* 0eh */
    unsigned short  unknown_3;       /* 10h */			//change  by tw,lzg 20150531
    unsigned short  foo;             /* 12h */
    int             unknown_2[10];   /* 14h */
    int             thread;          /* 3ch */
    unsigned int    epoch;           /* 40h */
};
typedef struct log_record_header_major  log_record_header_major;
#endif


//struct log_changeVector_header//24byte
//{
//    unsigned char  op_major;           /* Opcode主码*/
//    unsigned char  op_minor;           /* Opcode子码*/
//    unsigned short block_class;                /* 0x02 */
//    unsigned int   file_id;                /* 0x04 */
//    unsigned int   dba;                /* 0x08 */
//    unsigned int   low_scn;          /* 0x0c */
//    unsigned short high_scn;          /* 0x10 */ //后续字段先不关注
//    unsigned char  unknown_12h[2];     /* 0x12 padding */
//    unsigned char  seq;                /* 0x14 */
//    unsigned char  typ;                /* 0x15 0-normal;1-multi block or begin trans;2-'block cleanout record'*/
//    unsigned short unknown_16h;        /* 0x16 padding? */
//};


//tw change 2014-12-11
struct log_changeVector_header//24byte
{
    unsigned char  op_major;           /* Opcode主码*/
    unsigned char  op_minor;           /* Opcode子码*/
    unsigned short block_class;        /* 0x02 */
    unsigned short file_id;            /* 0x04 */
    unsigned short unknown_tw;
	unsigned int   dba;                /* 0x08 */
    unsigned int   low_scn;            /* 0x0c */
    unsigned short high_scn;           /* 0x10 */ //后续字段先不关注
    unsigned char  unknown_12h[2];     /* 0x12 padding */
    unsigned char  seq;                /* 0x14 */
    unsigned char  typ;                /* 0x15 0-normal;1-multi block or begin trans;2-'block cleanout record'*/
    unsigned short unknown_16h;        /* 0x16 padding? */
};
typedef struct log_changeVector_header log_changeVector_header;

struct log_opcode_51       // DML UNDO statement 1
{
    unsigned short size;		/*0x00*/
    unsigned short space;		/*0x02*/
    unsigned short flag;		/*0x04*/
    unsigned short unknown1;	/*0x06*/
    unsigned short xid_high;	/*0x08*/
    unsigned short xid_mid;		/*0x0a*/
    unsigned int   xid_low;		/*0x0c*/
    unsigned short seq;			/*0x10*/
    unsigned char  rec;			/*0x12*/
};
typedef struct log_opcode_51 log_opcode_51;

struct log_opcode_51_second //DML UNDO statement 2
{
    unsigned int   objn;		/*0x00*/
    unsigned int   objd;		/*0x04*/
    unsigned int   tsn;			/*0x08*/
    unsigned int   unknown1;	/*0x0c*/
    unsigned char  op_major;	/*0x10*/
    unsigned char  op_minor;	/*0x11*/
    unsigned char  slt;			/*0x12*/
    unsigned char  rci;			/*0x13*/
    unsigned char  is_ktubl;	/*0x14*/
};
typedef struct log_opcode_51_second log_opcode_51_second;

struct log_opcode_kdo       //DML UNDO statement 4
{
    unsigned int   bdba;		/*0x00*/
    unsigned int   hdba;		/*0x04*/
    unsigned short maxfr;		/*0x08*/
    unsigned char  opcode;		/*0x0a*/
    unsigned char  xtype;		/*0x0b*/
    unsigned char  itli;		/*0x0c*/
    unsigned char  ispac;		/*0x0d*/
};
typedef struct log_opcode_kdo log_opcode_kdo;


struct log_opcode_kdoirp  //DML INSERT STATEMENT 2
{
    unsigned int   bdba;               /* 00h */
    unsigned int   hdba;               /* 04h */
    unsigned short maxfr;              /* 08h */
    unsigned char  opcode;             /* 0ah */
    unsigned char  xtype;              /* 0bh */
    unsigned char  itli;               /* 0ch */
    unsigned char  ispac;              /* 0dh */
    unsigned short unknown1;           /* 0eh */
    unsigned char  flag;               /* 10h  标志位*/
    unsigned char  lb;                 /* 11h */
	unsigned char  cc;                 /* 12h 列数，用这个列数来循环存储所有列的值*/
    unsigned char  cki;                /* 13h */
    unsigned int   hrid;               /* 14h */
    unsigned short hrid_minor;         /* 18h */
    unsigned short unknown2;           /* 1ah */
    unsigned int   nrid;               /* 1ch  行链接或迁移时指向下一个rowid*/
	unsigned short nrid_minor;         /* 20h行链接或迁移时指向下一个rowid */
    unsigned short unknown3;           /* 22h */
    unsigned int   unknown4;           /* 24h */
    unsigned short size;               /* 28h */
    unsigned short slot;               /* 2ah */
    unsigned char  tabn;               /* 2bh */
    unsigned char  null_bitmap[1];     /* 2ch */
};
typedef struct log_opcode_kdoirp log_opcode_kdoirp;


struct log_opcode_kdodrp //DML DELETE STATEMENT 2
{
    unsigned int   bdba;		/*0x00*/
    unsigned int   hdba;		/*0x04*/
    unsigned short maxfr;		/*0x08*/
    unsigned char  opcode;		/*0x0a*/
    unsigned char  xtype;		/*0x0b*/
    unsigned char  itli;		/*0x0c*/
    unsigned char  ispac;		/*0x0d*/
    unsigned short unknown1;	/*0x0e*/
    unsigned short slot;		/*0x10*/
    unsigned char  tabn;		/*0x12*/
};
typedef struct log_opcode_kdodrp log_opcode_kdodrp;

struct log_opcode_kdourp //DML UPDATE STATEMENT 2
{
    unsigned int   bdba;               /* 00h */
    unsigned int   hdba;               /* 04h */
    unsigned short maxfr;              /* 08h */
    unsigned char  opcode;             /* 0ah */
    unsigned char  xtype;              /* 0bh */
    unsigned char  itli;               /* 0ch */
    unsigned char  ispac;              /* 0dh */
    unsigned short unknown1;           /* 0eh */
    unsigned char  flag;               /* 10h  标志位*/
    unsigned char  lock;               /* 11h */
    unsigned char  ckix;               /* 12h */
    unsigned char  tabn;               /* 13h */
    unsigned short slot;               /* 14h */
    unsigned char  ncol;               /* 16h 该数据块实际含有的列数*/
    unsigned char  nnew;               /* 17h  更新的列数*/
    unsigned short size;               /* 18h */
    unsigned char  null_bitmap[1];     /* 1ah */
};
typedef struct log_opcode_kdourp log_opcode_kdourp;

struct log_opcode_kdoqm // DML MULTIINSERT STATEMENT 2
{
    unsigned int   bdba;
    unsigned int   hdba;
    unsigned short maxfr;
    unsigned char  opcode;
    unsigned char  xtype;
    unsigned char  itli;
    unsigned char  ispac;
    unsigned short unknown1;
    unsigned char  tabn;
    unsigned char  lock;
    unsigned char  nrow;
    unsigned char  unknown;
    unsigned short slot[1];
};
typedef struct log_opcode_kdoqm log_opcode_kdoqm;

struct log_redo_rid
{
    unsigned int   major;
    unsigned short minor;
};
typedef struct log_redo_rid log_redo_rid;


struct log_opcode_52 // begin transaction 1
{
    unsigned short slot;			/* 0x20 */
    unsigned short unknown5;        /* 0x22 */
    unsigned int   seq;             /* 0x24 */
    unsigned int   uba_high;        /* 0x28 */
    unsigned short uba_mid;         /* 0x2c */
    unsigned char  uba_low;         /* 0x2e */
    unsigned char  unknown6[1];     /* 0x2f */
    unsigned short flag;            /* 0x30 */
    unsigned short size;            /* 0x32 */
    unsigned char  fbi;             /* 0x34 */
    unsigned char  unknown1;		/* 0X35*/
    unsigned short unknown2;		/* 0X36*/
    unsigned short pxid_major;		/* 0X38*/
    unsigned short pxid_minor;		/* 0X3A*/
    unsigned int   pxid_micro;		/* 0X3C*/
};
typedef struct log_opcode_52 log_opcode_52;


struct log_opcode_54 // commit 1
{
    unsigned short slt;
    unsigned short unknown2;
    unsigned int   sqn;
    unsigned char  srt;
    unsigned char  unknown1[3];
    unsigned int   sta;
    unsigned char  flg;
    /*
    ** if (flg & 4) == ROLLBACK
    ** if (flg & 2) following struct is ZOraOpcode_54_part2
    ** if (flg & 1) OR? (flg & 0x10) last struct is commit scn
    */
};
typedef struct log_opcode_54 log_opcode_54;

/*update的一个中间操作，删除一个块中废弃的列信息,脱离与前一列的行连接*/
struct log_opcode_kdomfc {
    unsigned int   bdba;
    unsigned int   hdba;
    unsigned short maxfr;
    unsigned char  opcode;
    unsigned char  xtype;
    unsigned char  itli;
    unsigned char  ispac;
    unsigned char  unknown1[2];
    unsigned short slot;
    unsigned char  tabn;
};
typedef log_opcode_kdomfc log_opcode_kdomfc;




// tw change 
//struct log_supplemental
//{
//    unsigned char   unknown;		/*0x00*/
//    unsigned char   flag;			/*0x01*/
//    unsigned short  cols_logged;	/*0x02*/
//    unsigned char   objv;           /*0x04??? */
//	unsigned char   unkown2;         /*0x05 tw change before: unsigned short start_column */
//   unsigned short   start_column;	/*0x06 start colNo of update*/
//	unsigned short   start_column_insert;	/*0x08 start colNo of insert*/
//};
//typedef struct log_supplemental log_supplemental;

// tw change 
struct log_supplemental
{
    unsigned char   unknown;		/*0x00*/
    unsigned char   flag;			/*0x01*/
    unsigned short  cols_logged;	/*0x02*/
    unsigned short   objv;           /*0x04??? */
    unsigned short   start_column;	/*0x06 start colNo of update*/
	unsigned short   start_column_insert;	/*0x08 start colNo of insert*/
};
typedef struct log_supplemental log_supplemental;


#include "Defines.h"

struct dml_oneColumnData
{
	WORD   colNo;
	bool   bDataNeedFree;
	DWORD  colLen;
	char*  colData;
	
	dml_oneColumnData()
	{
		this->clear();
	}
	void clear()
	{
		colNo = 0;
		bDataNeedFree = false;
		colLen = 0;
		colData = NULL;
	}

	void freeData(){
		if(bDataNeedFree){
			delete[] colData;
		}

		clear();
	}
};
typedef struct dml_oneColumnData dml_oneColumnData; 

struct dml_oneColumnData_header
{
	WORD colNo;
	WORD undown;
	DWORD colLen;
};
typedef struct dml_oneColumnData_header dml_oneColumnData_header; 


enum DML_TYPE 
{
	DML_UNKOWN = 0,
	DML_INSERT = 1,
	DML_DELETE =2,
	DML_UPDATE =3
};

struct dml_header
{
	unsigned short xidSlot;		/*0x0a*/
    unsigned int   xidSqn;		/*0x0c*/
	BYTE8          scn;
	unsigned int   objNum;
	unsigned char  dmlType;
	unsigned short dmlNo;
	unsigned short undoCols;
	unsigned int   undoCols_dataLen;
	unsigned short redoCols;
	unsigned int   redoCols_dataLen;

	unsigned short dmlHeaderLen;
	unsigned int   dmlTotalLen;
};
typedef struct dml_header dml_header; 

struct dml_trsct_begin
{
	unsigned short  xidSlot;
	unsigned int    xidSqn;
	BYTE8           scn;
	unsigned char   threadId;
	unsigned short  len;
	unsigned int    dmls;
	unsigned int    trsctLen;
};
typedef struct dml_trsct_begin dml_trsct_begin;

struct dml_trsct_end
{
	unsigned short  xidSlot;
	unsigned int    xidSqn;
	BYTE8           scn;
	BYTE8           commitSCN;
	unsigned char   threadId;
	unsigned short  len;
	unsigned int    dmls;
	unsigned int    trsctLen;
};
typedef struct dml_trsct_end dml_trsct_end;




//////////////lob//////////

//typedef struct ZOra19_1 {
//    u_int   obj_id;             /* 0x00 */
//    struct ZOraLobId lobid;     /* 0x04 */
//    u_short unknown3;           /* 0x0e */
//    u_int   version_major;      /* 0x10 */
//    u_int   version_minor;      /* 0x14 */
//    u_int   pagenumber;         /* 0x18 */
//    u_short pdba0;              /* 0x1a */
//    u_short pdba1;              /* 0x1c */
//    u_short pdba2;              /* 0x1e */
//    u_short pdba4;              /* 0x20 */
//    char    data;               /* 0x22 */
//} ZOra19_1;


/* Lob ID */
//struct ZOraLobId {
//    u_short index;              /* 0x00 */
//    u_char macro0;              /* 0x02 */
//    u_char macro1;              /* 0x03 */
//    u_char unknown2;            /* 0x04 */
//    u_char unknown3;            /* 0x05 */
//    u_char minor0;              /* 0x06 */
//    u_char minor1;              /* 0x07 */
//    u_char major0;              /* 0x08 */
//    u_char major1;              /* 0x09 */
//};

////////////////////////////////////


#endif
