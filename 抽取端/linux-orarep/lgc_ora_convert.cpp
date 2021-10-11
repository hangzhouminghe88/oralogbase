#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lgc_ora_convert.h"

CONFIG_DATA config_data;

void raw_to_date(ub1 *buffer,ub4 len,PCOL_DATA col_data){
     ub1 *data;
     if ((len!=7) || (! check_date(buffer))){
         col_data->mem_alloc=TRUE;
         col_data->common_data.data=col_data->common_data.str_data=(char *)malloc(1);
         col_data->common_data.data[0]=0;
         return;
     }     
     col_data->mem_alloc=TRUE;
     col_data->common_data.data=col_data->common_data.str_data=(char *)malloc(DATE_STR_LEN+1);
     data=(ub1*)col_data->common_data.data;

     fill_2_num((char*)data,buffer[0]-100);
     data+=2;

     fill_2_num((char*)data,buffer[1]-100);
     data+=2;
     *(data++)='-';
     fill_2_num((char*)data,buffer[2]);
     data+=2;
     *(data++)='-';
     fill_2_num((char*)data,buffer[3]);
     data+=2;
     *(data++)=' ';
     fill_2_num((char*)data,buffer[4]-1);
     data+=2;
     *(data++)=':';
     fill_2_num((char*)data,buffer[5]-1);
     data+=2;
     *(data++)=':';
     fill_2_num((char*)data,buffer[6]-1);
     data+=2;
     *data=0;
}          
 

void raw_to_timestamp(ub1 *buffer,ub4 len,PCOL_DATA col_data,ub2 scale){
     ub1 *data,val;
     char temp[50];
     char fmt[15];
	 ub2 i;
     ub2 frac_len=0;
     ub8 frac=0;
     if (len==7) {
        raw_to_date(buffer,len,col_data);
        return;
     }   
     for (i=7 ; i<len;i++){
         val=buffer[i];
         frac=(frac<<8)+val;
     }    
#ifdef _WIN32_
     sprintf(temp,".%09I64u",frac);
#else
     sprintf(temp,".%09llu",frac);
#endif     
     
     if (scale>0) 
       temp[scale+1]='\0';
     else
       temp[scale]='\0';  
     
     frac_len=strlen(temp);
          
     col_data->mem_alloc=TRUE;
     col_data->common_data.data=col_data->common_data.str_data=(char *)malloc(DATE_STR_LEN+1+frac_len);
     data=(ub1*)col_data->common_data.data;
     fill_2_num((char*)data,buffer[0]-100);
     data+=2;
     fill_2_num((char*)data,buffer[1]-100);
     data+=2;
     *(data++)='-';
     fill_2_num((char*)data,buffer[2]);
     data+=2;
     *(data++)='-';
     fill_2_num((char*)data,buffer[3]);
     data+=2;
     *(data++)=' ';
     fill_2_num((char*)data,buffer[4]-1);
     data+=2;
     *(data++)=':';
     fill_2_num((char*)data,buffer[5]-1);
     data+=2;
     *(data++)=':';
     fill_2_num((char*)data,buffer[6]-1);
     data+=2;
     memcpy(data,temp,frac_len);
     data+=frac_len;
     *data=0;
}    


void raw_to_timestamp_tz(ub1 *buffer,ub4 len,PCOL_DATA col_data,ub2 scale){
     ub1 *data,val;
     ub8 frac;
     char temp[50];
	 struct tm stm;
	 time_t seconds;
	 struct tm *newtm;
     ub2 frac_len=0;

	 stm.tm_isdst=0;
	 stm.tm_year=(buffer[0]-100)*100+buffer[1]-100-1900;
	 stm.tm_mon=buffer[2]-1;/*月份从0-11*/
	 stm.tm_mday=buffer[3];
	 stm.tm_hour=buffer[4]-1;
	 stm.tm_min=buffer[5]-1;
	 stm.tm_sec=buffer[6]-1;
	 
	 seconds=mktime(&stm);
	 if (seconds==-1){
		 if (config_data.trace_level>=TL_MIN_TRACE)
			 sp_printf("raw_to_timestamp_tz: mktime return -1.\n");
		 return;
	 }

     if (buffer[11]>20 || (buffer[11]==20 && buffer[12]>=60))
       seconds=seconds+((ub4)(buffer[11]-20))*3600+((ub4)(buffer[12]-60))*60;
     else 
       seconds=seconds-((ub4)(20-buffer[11]))*3600-((ub4)(60-buffer[12]))*60;
     
	 newtm=localtime(&seconds);
	 if (newtm==NULL){
 		 if (config_data.trace_level>=TL_MIN_TRACE)
			 sp_printf("raw_to_timestamp_tz: localtime return NULL.\n");
		 return;
	 }

     val=buffer[7];
     frac=val;
     val=buffer[8];
     frac=(frac<<8)+val;
    
     val=buffer[9];
     frac=(frac<<8)+val;
     val=buffer[10];
     frac=(frac<<8)+val;
     
#ifdef _WIN32_
     sprintf(temp,".%09I64u",frac);
#else
     sprintf(temp,".%09llu",frac);
#endif     
     
     if (scale>0) 
       temp[scale+1]='\0';
     else
       temp[scale]='\0';   
     
     frac_len=strlen(temp);
          
     col_data->mem_alloc=TRUE;
     col_data->common_data.data=col_data->common_data.str_data=(char *)malloc(DATE_STR_LEN+1+frac_len+7+1);//tw change
     data=(ub1*)col_data->common_data.data;

     /*fill_2_num(data,buffer[0]-100);
     data+=2;
     fill_2_num(data,buffer[1]-100);
     data+=2;
     *(data++)='-';
     fill_2_num(data,buffer[2]);
     data+=2;
     *(data++)='-';
     fill_2_num(data,buffer[3]);
     data+=2;
     *(data++)=' ';    
     fill_2_num(data,buffer[4]-1);
     data+=2;
     *(data++)=':';
     fill_2_num(data,buffer[5]-1);
     data+=2;
     *(data++)=':';
     fill_2_num(data,buffer[6]-1);
     data+=2;*/

	 fill_2_num((char*)data,(newtm->tm_year+1900)/100);
     data+=2;
	 fill_2_num((char*)data,(newtm->tm_year+1900) % 100);
     data+=2;
     *(data++)='-';
	 fill_2_num((char*)data,newtm->tm_mon+1);
     data+=2;
     *(data++)='-';
	 fill_2_num((char*)data,newtm->tm_mday);
     data+=2;
     *(data++)=' ';    
	 fill_2_num((char*)data,newtm->tm_hour);
     data+=2;
     *(data++)=':';
	 fill_2_num((char*)data,newtm->tm_min);
     data+=2;
     *(data++)=':';
	 fill_2_num((char*)data,newtm->tm_sec);
     data+=2;

     memcpy(data,temp,frac_len);
     data+=frac_len;
     *(data++)=' ';     
     if (buffer[11]>20 || (buffer[11]==20 && buffer[12]>=60)){
       *(data++)='+';
       fill_2_num((char*)data,buffer[11]-20);
	   data+=2;
       *(data++)=':';  
	   if(buffer[12] < 60) *(data++) = '-';//change tw
	   fill_2_num((char*)data,abs(buffer[12]-60));//change tw
	   data+=2;
     } 
     else {
       *(data++)='-';
       fill_2_num((char*)data,20-buffer[11]);
	   data+=2;
       *(data++)=':';  
	   fill_2_num((char*)data,60-buffer[12]);
	   data+=2;
     }  
     *data=0;
}     

void raw_to_timestamp_ltz(ub1 *buffer,ub4 len,PCOL_DATA col_data,ub2 scale){
     ub1 *data,val;
     char temp[50];
     ub2 frac_len=0,i;
     ub8 frac=0;
     if (len==7) {
        raw_to_date(buffer,len,col_data);
        return;
     }   
     for (i=7 ; i<len;i++){
         val=buffer[i];
         frac=(frac<<8)+val;
     }    
     
#ifdef _WIN32_
     sprintf(temp,".%09I64u",frac);
#else
     sprintf(temp,".%09llu",frac);
#endif     
     
     if (scale>0) 
       temp[scale+1]='\0';
     else
       temp[scale]='\0';  
     
     frac_len=strlen(temp);
          
     col_data->mem_alloc=TRUE;
     col_data->common_data.data=col_data->common_data.str_data=(char *)malloc(DATE_STR_LEN+1+frac_len);
     data=(ub1*)col_data->common_data.data;
     fill_2_num((char*)data,buffer[0]-100);
     data+=2;
     fill_2_num((char*)data,buffer[1]-100);
     data+=2;
     *(data++)='-';
     fill_2_num((char*)data,buffer[2]);
     data+=2;
     *(data++)='-';
     fill_2_num((char*)data,buffer[3]);
     data+=2;
     *(data++)=' ';
     fill_2_num((char*)data,buffer[4]-1);
     data+=2;
     *(data++)=':';
     fill_2_num((char*)data,buffer[5]-1);
     data+=2;
     *(data++)=':';
     fill_2_num((char*)data,buffer[6]-1);
     data+=2;
     memcpy(data,temp,frac_len);
     data+=frac_len;
     *data=0;
}
 



void raw_to_number(ub1 *buffer,ub4 len,PCOL_DATA col_data){
     ub4 i;
     ub2 j,bytes;
     sb1 cur_exp,exponent;
     ub1 positive=buffer[0] >= 128 ? 1:0;
     ub1 *data,a;
     exponent=buffer[0] > 128 ? buffer[0]-193: 62-buffer[0];

     if (buffer[0]==128){
         col_data->common_data.data=(char *)malloc(2);
         col_data->common_data.str_data=col_data->common_data.data;
         col_data->mem_alloc=TRUE;
         col_data->common_data.data[0]='0';
         col_data->common_data.data[1]='\0';
         return;
     }	 
	 if (exponent > 62 || exponent < -65) {
         col_data->common_data.data=(char *)malloc(2);
         col_data->common_data.str_data=col_data->common_data.data;
         col_data->mem_alloc=TRUE;
         col_data->common_data.data[0]='\0';
         return;
	 }

     len--;
     buffer++;

     if (! positive ) len--; /*忽略负数最后的0x66*/
     col_data->mem_alloc=TRUE;
     bytes=len*2+4;

     if (exponent>0 && exponent>=len)
        bytes+=(exponent-len+1)*2;
	 else if (exponent < 0)
		bytes+=(abs(exponent)-1)*2;

     col_data->common_data.data=(char *)malloc(bytes);
     col_data->common_data.str_data=col_data->common_data.data;
     data=(ub1*)col_data->common_data.data;
     cur_exp=exponent;
     if (! positive)
        *(data++)='-';
     for (i=0;i<len;i++){
         if (cur_exp<0){
             if (exponent>=0 && cur_exp==-1)
                 *(data++)='.';                
             else if (exponent<0 && i==0){
				 *(data++)=0x30;
				 *(data++)='.';
                 
				 for (j=0;j<(abs(exponent)-1)*2;j++)
                   *(data++)=0x30;
            }   
         }
         if (positive)
            a=buffer[i]-1;
         else
            a=101-buffer[i];   
         if (a<10){
             if (i>0 || cur_exp<0)
                *(data++)=0x30;
            *(data++)=a+0x30;    
         }
         else {
            *(data++)=a /10 +0x30;
            *(data++)=a % 10 +0x30;
         }
         cur_exp--;
     }
     if (cur_exp>=0)
        for (j=0;j<(cur_exp+1)*2;j++)
          *(data++)=0x30;
     else if (cur_exp<-1) {
        while (*(--data)==0x30);
        data++;
     }   
     *data=0;     
} 

void raw_to_binary_float(ub1 *buffer,ub4 len,PCOL_DATA col_data){
     char temp[100];
     float f;
     ub4 *p=(ub4 *)&f;
     ub4 length;
     ub1 buf[4];
     memcpy(buf,buffer,4);

     if (buf[0]>=0x80) 
        buf[0]&=0x7f;
     else{ 
       // buf[0]|= 0x80;
		buf[0] ^=0xff;
		buf[1] ^=0xff;
		buf[2] ^=0xff;
		buf[3] ^=0xff;
	}
        
     *p=get_ub4_big(buf,0);     
     sprintf(temp,"%.10f",f);
     length=strlen(temp);
     col_data->common_data.data=(char *)malloc(length+1);
     col_data->common_data.str_data=col_data->common_data.data;
     col_data->mem_alloc=TRUE;
     strcpy(col_data->common_data.str_data,temp);
}     

void raw_to_binary_double(ub1 *buffer,ub4 len,PCOL_DATA col_data){
     char temp[100];
     double f;
     ub8 *p=(ub8 *)&f;
     ub4 length;
     ub1 buf[8];
     memcpy(buf,buffer,8);

     if (buf[0]>=0x80) 
        buf[0]&=0x7f;
     else{   
      //  buf[0]|= 0x80;
		int i;
		for(i= 0; i<8; i++){
			buf[i] ^=0xff;
		}
	 }

     get_ub8_big(buf,0,p);

     sprintf(temp,"%.10f",f);
     length=strlen(temp);
     col_data->common_data.data=(char *)malloc(length+1);
     col_data->common_data.str_data=col_data->common_data.data;
     col_data->mem_alloc=TRUE;
     strcpy(col_data->common_data.str_data,temp);
} 

void raw_to_char(ub1 *buffer,ub4 len,PCOL_DATA col_data){
     col_data->mem_alloc=TRUE;
     col_data->common_data.data=col_data->common_data.str_data=(char *)malloc(len+1);
     memcpy(col_data->common_data.data,buffer,len);
     col_data->common_data.data[len]=0;
}


void raw_to_raw(ub1 *buffer,ub4 len,PCOL_DATA col_data){
     col_data->mem_alloc=TRUE;
     col_data->common_data.data=col_data->common_data.str_data=(char *)malloc(len);
     memcpy(col_data->common_data.data,buffer,len);     
     col_data->data_len=len;
}  

void raw_to_hex(ub1 *buffer,ub4 len,PCOL_DATA col_data){
     ub4 i;
     ub1 *buf;
     col_data->mem_alloc=TRUE;
     col_data->common_data.data=col_data->common_data.str_data=(char *)malloc(len*2+1);
     buf=(ub1*)col_data->common_data.data;
     for (i=0;i<len;i++){
       fill_2_hex((char*)buf,buffer[i]);
       buf+=2;
     }  
     *buf=0;       
}   

void raw_to_interval_ym(ub1 *buffer, ub4 len, PCOL_DATA col_data)
{
	struct oracle_intervalym {
        u_char year[4];
        u_char month;
    };
	
	struct oracle_intervalym *interval = (struct oracle_intervalym*)buffer;
	int year, month,i;
	char output[100];
	bool negative = false;

	if(len != 5){
		fprintf(stderr, "intervalym invalid length:%d\n",len);
		return;
	}

	year = 0;
	for(i=0;i<4;i++){
		year |= interval->year[i]<<((3-i)*8);
	}

	year -=0x80000000;
	month = interval->month-60;

	if(year < 0){
		year = -year;
		negative = true;
	}
	if(month < 0){
		month = -month;
		negative = true;
	}

	sprintf(output, "%s%d-%d", 
		negative? "-":"",year,month);
	
	col_data->mem_alloc=TRUE;
	col_data->common_data.data=col_data->common_data.str_data = strdup(output);


	return;
}

void raw_to_interval_ds(ub1 *buffer, ub4 len, PCOL_DATA col_data)
{
	struct oracle_intervalds
	{
		ub1 day[4];
		ub1 hour;
		ub1 minute;
		ub1 second;
		ub1 nanosecond[4];
	};
	struct oracle_intervalds *interval = (struct oracle_intervalds*)buffer;
	int day,nanosecond,i;
	char output[100] = {0};
	bool negative = false;

	if(len != 11){
		fprintf(stderr, "intervalds length: %d\n", len);
		return;
	}

	day = 0;
	for(i=0;i<4;i++){
		day |= interval->day[i]<<((3-i)*8);
	}
	day = day-0x80000000;

	nanosecond = 0;
	for(i=0; i<4;i++){
		nanosecond |= interval->nanosecond[i]<<((3-i)*8);
	}
	nanosecond -=0x80000000;
	
	if( day < 0){
		negative = true;
		day =-day;
	}
	if(nanosecond < 0){
		negative = true;
		nanosecond = -nanosecond;
	}

	interval->hour   = negative? (60 - interval->hour):  (interval->hour   - 60);
	interval->minute = negative? (60 - interval->minute):(interval->minute - 60);
	interval->second = negative? (60 - interval->second):(interval->second - 60);

	sprintf(output, "%s%d %02d:%02d:%02d.%d",
		negative?"-":"",
		day,
		interval->hour,
		interval->minute,
		interval->second,
		nanosecond);
	
	col_data->mem_alloc=TRUE;
	col_data->common_data.data=col_data->common_data.str_data = strdup(output);
	return;
}

void raw_to_rowid(ub1 *buffer,ub4 len,PCOL_DATA col_data){
     ub4 obj_no,rdba;
     ub2 row_no;
     if ( len!=10 && len!=6 ){
         col_data->is_null=TRUE;
         return;
     }
     col_data->mem_alloc=TRUE;
     col_data->common_data.data=col_data->common_data.str_data=(char *)malloc(EXT_ROWID_STR_LEN+1);
     
     if (len==10){
          obj_no=get_ub4_big(buffer,0);
          rdba=get_ub4_big(buffer,4);
          row_no=get_ub2_big(buffer,8);
          encode_rowid(col_data->common_data.str_data,obj_no,rdba_file_no(rdba,MAX_TS_COUNT),rdba_block_no(rdba,MAX_TS_COUNT),row_no);
     }     
     else {
          rdba=get_ub4_big(buffer,0);
          row_no=get_ub2_big(buffer,4);
          encode_short_rowid(col_data->common_data.str_data,rdba_file_no(rdba,MAX_TS_COUNT),rdba_block_no(rdba,MAX_TS_COUNT),row_no);
     }     
} 

void encode_rowid(char *rowid,ub4 obj_no,ub2 file_no,ub4 block_no,ub2 row_no){
    rowid[EXT_ROWID_STR_LEN]='\0';
    encode_base64(rowid,6,obj_no);
    encode_base64(rowid+6,3,file_no);
    encode_base64(rowid+9,6,block_no);
    encode_base64(rowid+15,3,row_no);
}  
void encode_short_rowid(char *rowid,ub2 file_no,ub4 block_no,ub2 row_no){
    rowid[EXT_ROWID_STR_LEN]='\0';
    encode_base64(rowid,8,file_no);
    rowid[8]='.';
    encode_base64(rowid+9,4,block_no);
    rowid[13]='.';
    encode_base64(rowid+15,3,row_no);
}  

int decode_base64(char *buf,int len,ub4 *result){
    int i;
    ub4 letter;    
    *result=0;
    for (i=0;i<len;i++){
        if (buf[i]>='A' && buf[i]<='Z')
          letter=buf[i]-'A';
        else if (buf[i]>='a' && buf[i]<='z')
          letter=buf[i]-'a' + 26;  
        else if (buf[i]>='0' && buf[i]<='9')
          letter=buf[i]-'0'+52;
        else if (buf[i]=='+')
          letter=62;
        else if (buf[i]=='/')
          letter=63;
        else return -1;
        *result=(*result)*64+letter;      
    }
    return 0;
}  

void encode_base64(char *buf,int len,ub4 value){
     ub1 letter;
     int i;
     for (i=len-1;i>=0;i--){
         letter=(ub1)(value & 0x3F);
         if (letter <=25)
            buf[i]=letter+'A';
         else if (letter>=26 && letter<=51)
            buf[i]=letter-26+'a';
         else if (letter>=52 && letter<=61)
            buf[i]=letter-52+'0';   
         else if (letter==62) 
            buf[i]='+';
         else
            buf[i]='/';   
         value=value >> 6;    
     }    
} 

int check_date(ub1 *buffer){
    if ( (buffer[0]>=100) && (buffer[1]>=100) && (buffer[0] <= 199)
       && (buffer[1]<=199) && (buffer[2] > 0) && (buffer[2] <=12)
       && (buffer[3]>0) && (buffer[3]<=31) && (buffer[4]>=1)
       && (buffer[4]<=24) && (buffer[5]>=1) && (buffer[5]<=60)
       && (buffer[6]>=1) && ( buffer[6]<=60))
       return TRUE;
    else
       return FALSE;   
}   


void fill_2_num(char* buffer, ub1 c)
{
	sprintf(buffer, "%.2u",c);
	return;
}

void fill_2_hex(char* buffer, ub1 c)
{
        sprintf(buffer, "%.2x",c);
        return;
}


int sp_printf(const char *format, ...)
{
	return 0;
}

/*
ub4 get_ub4_big(ub1* buffer, ub1 isBig)
{	
	if(isBig){//is big machine
		//return *(ub4*)buffer;
	}else{
		int i;
		for(i=0; i<2; i++){//swap
			ub1 tmp = buffer[i];
			buffer[i] = buffer[4-1-i];
			buffer[4-1-i] = tmp;
		}	
	}
	return *(ub4*)buffer;
}
*/

void get_ub8_big(ub1* _buffer,ub1 offset, ub8* pResult)
{
	ub1 array[8] = {0};
	memcpy(array,_buffer,8);

	ub1* buffer = &array[0];

	if(0){//is big machine
	}else{
		ub1 *p = &buffer[0];
		ub1 *q = &buffer[7];
		while(p < q){
			ub1 tmp = *p;
			*p++ = *q;
			*q-- = tmp;

		}
	}

	*pResult = *(unsigned long long *)buffer;

	return;
}

ub4 get_ub4_big(ub1* buffer, ub1 offset)
{	
	buffer += offset;
	if(0){//is big machine
		//return *(ub4*)buffer;
	}else{
		int i;
		for(i=0; i<2; i++){//swap
			ub1 tmp = buffer[i];
			buffer[i] = buffer[4-1-i];
			buffer[4-1-i] = tmp;
		}	
	}
	return *(unsigned int*)buffer;
}

ub2 get_ub2_big(ub1* buffer, ub1 offset)
{
	buffer += offset;
	if(0){
	}else{
		ub1 tmp = buffer[0];
		buffer[0] = buffer[1];
		buffer[1] = tmp;
	}
	return *(unsigned short*)buffer;

}

ub2 rdba_file_no(ub4 rdba,ub4 max)
{
	ub4 fileNo = rdba&rdba&0xffc00000;
	fileNo >>=22;

	return (ub2)fileNo;
}

ub4 rdba_block_no(ub4 rdba,ub4 max)
{
	ub4 blockNo = rdba&0x003ffff;
	return blockNo;
}



////////////////lib
int strcmp_igNum(const char* str1, const char* str2)
{
	const char* p = str1;
	const char* q = str2;

	while( *p != '\0' && *q != '\0'){
		while( *p >='0' && *p <= '9') p++;
		while( *q >='0' && *q <= '9') q++;

		if(*p == '\0' || *q == '\0') break;

		if(*p > *q ){
			return 1;
		}else if(*p < *q){
			return -1;
		}

		p++;
		q++;
	}//end while
	
	if(*p > *q){
		return 1;
	}else if(*p < *q){
		return -1;
	}else{
		return 0;
	}
}

ub2 getScaleInTimestamp(const char* colDataType)
{
	const char* p = NULL;
	const char* q = NULL;

	p = strchr(colDataType, '(');
	q = strchr(colDataType, ')');
	
	if(q == NULL || q == NULL){
		fprintf(stderr, "getScaleInTimestamp failed\n");
		exit(1);
	}
	p++;

	char buf[126] = {0};
	strncpy(buf,p,q-p);

	return atoi(buf);
}


