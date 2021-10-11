#ifndef LGC_DMLROWPARSER_H
#define LGC_DMLROWPARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <algorithm>
#include <list>
using namespace std;

#include "lgc_Defines.h"
#include "lgc_Structure.h"
#include "Mutex.h"

class LGC_DmlRow;
class LGC_MediaFileOutput;
class table_meta;
class LGC_DmlColumn;

class LGC_DmlRowParser
{
private:
	//member variables

	static Mutex s_mutex;
	static LGC_DmlRowParser* s_instance;

private:
	//constructor and desctructor
	LGC_DmlRowParser();
	~LGC_DmlRowParser();

public:
	//public member functions
	int parse(const LGC_DmlRow* pDmlRow, LGC_MediaFileOutput* pMediaFileOutput);

private:
	//private member functions
	int parseInsertDmlRow(const table_meta& tableMeta, 
		                  const dml_header& dmlHeader, 
		                  const list<LGC_DmlColumn*>& redoColList, 
		                  const list<LGC_DmlColumn*>& undoColList,
		                  LGC_MediaFileOutput*        pMediaFileOutput
	                      );

	int parseDeleteDmlRow(const table_meta& tableMeta, 
		                  const dml_header& dmlHeader, 
		                  const list<LGC_DmlColumn*>& redoColList, 
		                  const list<LGC_DmlColumn*>& undoColList,
						  LGC_MediaFileOutput*        pMediaFileOutput
						  );

	int parseUpdateDmlRow(const table_meta& tableMeta, 
		                  const dml_header& dmlHeader, 
		                  const list<LGC_DmlColumn*>& redoColList, 
		                  const list<LGC_DmlColumn*>& undoColList,
		                  LGC_MediaFileOutput*        pMediaFileOutput
		                  );

public:
	//static member functions
	static LGC_DmlRowParser* getInstance();

private:
	//private static member functions
	static const string getTableName(const table_meta& tableMeta);
	static const string getColName(const table_meta& tableMeta, const LGC_DmlColumn& dmlCol);
	static const string getColValue(const table_meta& tableMeta, const LGC_DmlColumn& dmlCol);
};
#endif
