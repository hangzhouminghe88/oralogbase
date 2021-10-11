#include <fstream>
#include <stdarg.h> 

using namespace std;

#define COMMLIB_DBG_FILE "MyLog.log"
#ifndef _MYLOG_FILE_
#define _MYLOG_FILE_


void MyLogMes(const char *str, ...);

void MyLogMes2File(char *FileName,const char *str, ...);

class LogClass
{
private:
	std::fstream m_logfile;//(FileName,mymode);
public:
	LogClass(string FileName,int Mode)
	{
		m_logfile.open(FileName.c_str(),Mode);
	};
	~LogClass(){
		m_logfile.close();
	};

	bool OpenFile(string FileName,int Mode)
	{
		m_logfile.open(FileName.c_str(),Mode);
		return is_open();
	}
	
	void LogData(string &strData)
	{
		m_logfile<<strData.c_str();
	};

	void LogData(char *strData)
	{
		m_logfile<<strData;
	};

	bool is_open(){
		return m_logfile.is_open();
	}

};

#endif

