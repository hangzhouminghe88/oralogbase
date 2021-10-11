#ifndef LGC_TRSCTORDERLIST_H
#define LGC_TRSCTORDERLIST_H

class LGC_TrsctOrderList
{
private:
	//member variables
private:
	//constructor and desctructor
	LGC_TrsctOrderList();
	~LGC_TrsctOrderList();

public:
	//public member functions
	LGC_Transaction* front();
	void pop_front();
	void push(LGC_Transaction* pTrsct);
};
#endif
