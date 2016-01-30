//
// Coor.cpp
//
#include "common.h"

#define PORT_NUM		10275
#define MAX_CMDTOKEN	6
#define BAD_IP			0xFFFFFFFF

typedef map<const char*, long>	nodeMap;	// AS->id;IPname->IP
FILE *input;

class coordinator : public commObject
{
public:
	coordinator();
	virtual ~coordinator() {;}
	
	virtual void init(int coorPort);
	void run();
	void run_console();
	virtual size_t readMsg(char* pBuff, size_t sz);

protected:
	void printMenu();
	void procUserInput(char* pBuf);
	unsigned long getIP(const char* pIP);
	unsigned long getIP(const char* pAS, const char* pIP);
	unsigned long getAS(const char* pAS);
	void handleQuery(unsigned long dest);
	void handleRegister(unsigned long* pBuf);

	void cmdChange();
	void cmdDownLink();
	void cmdDownUPASLink();
	void cmdSendMsg();
	void cmdQuit();

private:
	bool	_bQuit;
	nodeMap	_nodeIPs;
	nodeMap	_nodeASs;
	char	_buffMsg[MSG_SIZE];
	char*	_pTokens[MAX_CMDTOKEN];
};
void start_s(void* pObj)			// for send
{
	((coordinator*) pObj)->run_console();
}
coordinator::coordinator() 
	:_bQuit(false)
{
#ifdef WIN32
	_ptime = new struct timeval;
	_ptime->tv_usec = 500;		// 500 usec 
	_ptime->tv_sec = 0;
#endif
}	

void coordinator::init(int coorPort)
{
#ifdef WIN32 
    //set up winsock stuff
    WSADATA wsa;
    if (WSAStartup(0x0101, &wsa) == SOCKET_ERROR)
		fprintf(stderr, "Error in WSAStartup.\n");
#endif
    memset(&cli_sin, 0, sizeof(cli_sin));
    memset(&srv_sin, 0, sizeof(srv_sin));
    srv_sin.sin_family = AF_INET;
	srv_sin.sin_addr.s_addr = htonl(INADDR_ANY);
	srv_sin.sin_port = htons(coorPort);

	if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		errexit("Coordinator can't create UDP socket: %s\n", strerror(errno));
	if (bind(sd, (struct sockaddr* )&srv_sin, sizeof(srv_sin)) < 0)
		errexit("Coordinator can't bind local address: %s\n", strerror(errno));
}

void coordinator::run()
{
	size_t nRead;
	while (!_bQuit)
	{
		nRead = readMsg(_buffMsg, MSG_SIZE);
		if (nRead > 0)
		{
			if (_buffMsg[0] == MSGTYPE_QUERYIP)
			{
				if (nRead >= sizeof(long)+1)
					handleQuery(*(unsigned long*)&_buffMsg[1]);
				else
					printf("coordinator::run: incorrect query msg size.\n");
			}
			else if (_buffMsg[0] == MSGTYPE_REG)
			{
				if (nRead < (4*sizeof(long)))
					printf("coordinator::run: incorrect register msg size.\n");
				else
					handleRegister((unsigned long*)&_buffMsg[1]);
			}
			else
				printf("Coordinator: unknown command %d.\n", _buffMsg[0]);
		}
	}
}

void coordinator::run_console()
{
	if (input == NULL)
	{
		while (!_bQuit)
		{
			printMenu();
			for (;;)
			{
//				if (gets(_buffMsg) != NULL)
				scanf("%127s", _buffMsg);
				if (strlen(_buffMsg) == 0)
						break;
			}
			procUserInput(_buffMsg);
		}
	}
	else
	{
#ifdef WIN32
		Sleep(1000*60);     // 1 minute
#endif
		while (!_bQuit)
		{
			printMenu();
			if (fgets(_buffMsg, MSG_SIZE, input) == NULL)
				_bQuit = true;
			else
			{
				if (_buffMsg[0] != '#')
					procUserInput(_buffMsg);
			}
#ifdef WIN32
			Sleep(5000);
#endif
		}
	}
}

size_t coordinator::readMsg(char* pBuff, size_t sz)
{
	int ns;
	size_t ret = 0;
	fd_set	readfds;
	FD_ZERO(&readfds);				// reset the FD, because NT.
#ifndef WIN32
	FD_SET(0, &readfds);			// stdin
	printMenu();
#endif
	FD_SET(sd, &readfds);
	ns = select(sd+1, &readfds, NULL, NULL, _ptime);
	if (ns < 0)
		printf("commObject/readMsg: error in select sockets: %s\n",strerror(errno));
	else if (ns > 0)
	{
#ifdef WIN32
		int sin_len;
#else
		unsigned int sin_len;
		if (FD_ISSET(0, &readfds))
		{
			printMenu();
			size_t nread = read(0, _buffMsg, MSG_SIZE);
			_buffMsg[nread] = '\0';
		}
		else
		{
#endif
			sin_len = sizeof(peer_sin);
			ret = recvfrom(sd, pBuff, sz, 0, (struct sockaddr* ) &peer_sin, &sin_len);
			if (sz < 0)
				printf("commObject/readMsg: error in recv message: %s\n", strerror(errno));
#ifndef WIN32
		}
#endif
	}
	return ret;
}

void coordinator::handleQuery(unsigned long dest)
{
	IPmap::iterator it = _IP2IP.find(dest);
	unsigned long* lp = (unsigned long*)_buff;
	if (it == _IP2IP.end())
	{
		*lp++ = BAD_IP;
		*lp++ = BAD_IP;
		printf("Coordinator: sent queried node 0x%x back. nodeis not available\n");
	}
	else
	{
		hostInfo *pHost = (hostInfo*) (*it).second;
#ifdef WIN32
		*lp++ = (pHost->_IPAddr).S_un.S_addr;
#else
		*lp++ = (pHost->_IPAddr).s_addr;
#endif
		*lp++ = pHost->_port;
		printf("Coordinator: sent queried node 0x%x back IP=%s,port=%d.\n"
			, dest, inet_ntoa(pHost->_IPAddr), pHost->_port);
	}
	size_t sz =2*sizeof(long);
#ifdef WIN32
	int sin_len;
#else
	unsigned int sin_len;
#endif
	sin_len = sizeof(cli_sin);
	if (sendto (sd, _buff, sz, 0, (struct sockaddr* )+ &peer_sin, sin_len) <0)
		printf("Coordinator: send query node 0x%x back failed..\n",dest);
}
void coordinator::handleRegister(unsigned long* pBuf)
{
	unsigned long* lp = pBuf;
	size_t sz = *lp++;
	unsigned long _ASid = *lp++;
	unsigned long _IPid = *lp++;
	sz -= 3*sizeof(long);
	char* p = (char*) lp;
	*(p+sz) = '\0';
	char* temp = strstr(p, ":");
	char *pStr;
	if (temp != NULL)
	{
		sz = temp - p;
		char *pStr = new char[sz+1];
		memcpy(pStr, p, sz);
		*(pStr+sz) = '\0';
		_nodeASs.insert(nodeMap::value_type(pStr, _ASid));
/*		sz = strlen(p) - sz -1;
		pStr = new char[sz+1];
		memcpy(pStr, temp+1, sz);
		*(pStr+sz) = '\0';			*/
	}
	pStr = new char[strlen(p)+1];
	strcpy(pStr, p);
	_nodeIPs.insert(nodeMap::value_type(pStr, _IPid));
	hostInfo* pHost = new hostInfo(_IPid, peer_sin.sin_addr, ntohs(peer_sin.sin_port));
	_IP2IP.insert(IPmap::value_type(_IPid, pHost));
	printf("Coordinator: handleRegister insert AS=0x%x,IP=0x%x,%s - realIP=%s,port=%d.\n"
		, _ASid, _IPid, p, inet_ntoa(pHost->_IPAddr), pHost->_port);
}
void coordinator::procUserInput(char* pBuf)
{
	char seps[]   = " ,\t";
	char *token;
	int i;
	char* p = pBuf;
	try
	{
		while (*p != '\0')
		{
			*p = toupper(*p);
			p++;
		}
		token = strtok(pBuf, seps);
		for (i = 0; i < MAX_CMDTOKEN; i++)
		{
			if( token != NULL )
			{
				_pTokens[i] = token;
				token = strtok( NULL, seps );
			}
			else
				break;
		}
		switch (*_pTokens[0])
		{
			case 'C':
				_buff[0] = MSGTYPE_CCHNG;
				cmdChange();
				break;
			case 'D':
				_buff[0] = MSGTYPE_CDN;
				cmdDownLink();
				break;
			case 'F':
				_buff[0] = MSGTYPE_CASDN;
				cmdDownUPASLink();
				break;
			case 'U':
				_buff[0] = MSGTYPE_CASUP;
				cmdDownUPASLink();
				break;
			case 'S':
				_buff[0] = MSGTYPE_CMSG;
				cmdSendMsg();
				break;
			case 'P':
				_buff[0] = MSGTYPE_CPRNT;
				sendMsg(getIP(_pTokens[1],_pTokens[2]), _buff, 1);
				break;
			case 'V':
				_buff[0] = MSGTYPE_CPRNT_DV;
				sendMsg(getIP(_pTokens[1],_pTokens[2]), _buff, 1);
				break;
			case 'Z':
				_buff[0] = MSGTYPE_CPRNT_NT;
				sendMsg(getIP(_pTokens[1],_pTokens[2]), _buff, 1);
				break;
			case 'Q':
				_buff[0] = MSGTYPE_CQUIT;
				cmdQuit();
				break;
			default:
				printf("Invalid command %c.\n", *_pTokens[0]);
				*_pTokens[0] = -1;
		}
	}
	catch (...)
	{
		*_pTokens[0] = -1;
	}
	if (*_pTokens[0] > 0)
		printf("Command has been executed.\n");
	else
		printf("Wrong command.\n");
}
void coordinator::printMenu()
{
	printf("Please enter a command:\n");
	printf("\tC <AS id> <router id1> <router id2> <new const> - for changing cost.\n");
	printf("\tD <AS id> <router id1> <router id2> - for down link in an AS.\n");
	printf("\tF <AS id1> <router id1> <AS id2> <router id2> - for down link between ASs.\n");
	printf("\tU <AS id1> <router id1> <AS id2> <router id2> - for Up link between ASs.\n");
	printf("\tS <msg> <host1> <host2> - for sending message from host1 to host 2.\n");
	printf("\tP <AS id> <router id>- for asking a router to print its forward table.\n");
	printf("\tV <AS id> <router id>- for asking a router to print its distance vector table.\n");
	printf("\tZ <AS id> <router id>- for asking a router to print its neighbour table.\n");
	printf("\tQ - for quit entire system. The coordinator notifies all nodes.\n");
}

void coordinator::cmdChange()
{
	unsigned long* lp = (unsigned long*) &_buff[1];
	unsigned long r1, r2;
	r1 = getIP(_pTokens[1],_pTokens[2]);
	r2 = getIP(_pTokens[1],_pTokens[3]);
	*lp++ = getAS(_pTokens[1]);
	*lp++ = r1;
	*lp++ = r2;
	*lp++ = atol(_pTokens[4]);
	size_t sz = sizeof(long)*4+1;
	sendMsg(r1, _buff, sz);
	sendMsg(r2, _buff, sz);
}
void coordinator::cmdDownLink()
{
	unsigned long* lp = (unsigned long*) &_buff[1];
	unsigned long r1, r2;
	r1 = getIP(_pTokens[1],_pTokens[2]);
	r2 = getIP(_pTokens[1],_pTokens[3]);
	*lp++ = getAS(_pTokens[1]);
	*lp++ = r1;
	*lp++ = r2;
	size_t sz = sizeof(long)*3+1;
	sendMsg(r1, _buff, sz);
	sendMsg(r2, _buff, sz);
}
void coordinator::cmdDownUPASLink()
{
	unsigned long* lp = (unsigned long*) &_buff[1];
	unsigned long r1, r2;
	r1 = getIP(_pTokens[1],_pTokens[2]);
	r2 = getIP(_pTokens[3],_pTokens[4]);
	*lp++ = getAS(_pTokens[1]);
	*lp++ = r1;
	*lp++ = getAS(_pTokens[3]);
	*lp++ = r2;
	size_t sz = sizeof(long)*4+1;
	sendMsg(r1, _buff, sz);
	sendMsg(r2, _buff, sz);
}
void coordinator::cmdSendMsg()
{
	unsigned long* lp = (unsigned long*) &_buff[1];
	unsigned long r1 = getIP(_pTokens[2]);			//src
	*lp++ = getIP(_pTokens[3]);			//dest
	size_t sz = strlen(_pTokens[1]);
	*lp++ = sz;		//msgLeng
	char *p = (char*) lp;
	strcpy(p, _pTokens[1]);
	sendMsg(r1, _buff, sz+(sizeof(long)*2)+2);
}
void coordinator::cmdQuit()
{
	_bQuit = true;
	nodeMap::iterator it;
	for (it = _nodeIPs.begin(); it != _nodeIPs.end(); it++)
		sendMsg((*it).second, _buff, 1);
}

unsigned long coordinator::getIP(const char* pIP)
{
	nodeMap::iterator it;
	for (it = _nodeIPs.begin(); it != _nodeIPs.end(); it++)
	{
		if (strcmp((*it).first, pIP) == 0)
			return (*it).second;
	}
	return BAD_IP;
}

unsigned long coordinator::getIP(const char* pAS, const char* pIP)
{
	char	temp[40];
	sprintf(temp, "%s:%s", pAS, pIP);
	unsigned long ret = getIP(temp);
	return ret;
}
unsigned long coordinator::getAS(const char* pAS)
{
	nodeMap::iterator it;
	for (it = _nodeASs.begin(); it != _nodeASs.end(); it++)
	{
		if (strcmp((*it).first, pAS) == 0)
			return (*it).second;
	}
	return BAD_IP;
}

int main(int argc, char** argv)
{
	int portNum = PORT_NUM;		
	input = NULL;
	if  (argc > 2)
	{
		if ((portNum = (unsigned short) atoi(argv[1])) == 0)
		{
			fprintf(stderr, "Incorrect port number, use default port %d.\n", PORT_NUM);
			portNum = PORT_NUM;
		}
		if (argc > 3)
		{
			if ((input=fopen(argv[2], "r")) == NULL)
				errexit("error in open input file");
		}
	}	
	printf("The coordinator is starting service at port %u.\n",portNum);
	coordinator* pComm = new coordinator();
	pComm->init(portNum);
#ifdef WIN32
	unsigned long threadID;      // pointer to receive thread ID
	HANDLE thread = CreateThread(NULL,0,
				(LPTHREAD_START_ROUTINE)&start_s,pComm,0,&threadID);
	pComm->run();
	WaitForSingleObject(thread, INFINITE);
#endif
	if (input != NULL)
		fclose(input);
	printf("Exit.\n");
	exit(0);
}
