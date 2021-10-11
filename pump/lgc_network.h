#ifndef LGC_NETWORK_H
#define LGC_NETWORK_H
class LGC_NetWork
{
private:
	char* m_host;
	int m_port;
	int m_fd;
	int m_listenFd;
public:
	LGC_NetWork();
	LGC_NetWork(int acceptFd);

	~LGC_NetWork();

	bool connect(const char* host, int port);
	bool listen(int port);
	LGC_NetWork* accept();
	int send(char* buf, int size);
	int recv(char* buf, int size);

private:
	void setConnectInfo(const char* host, int port);
	void setListenInfo(int port);
};

#endif
