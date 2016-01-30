//
// common.cpp
//
#include "common.h"

int errexit(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(-1);
}
/*****************
 *  hostInfo
 *****************/
void hostInfo::dump()
{
	printf("hostInfo::dump: IP=0x%x - realIP=%s,port=%d.\n"
		,_hostIP, inet_ntoa(_IPAddr), _port);
}
/*****************
 *  commObject
 *****************/
commObject::commObject(int timeout)
{
//	_ptime = NULL;
	_ptime = new struct timeval;
	_ptime->tv_usec = 0;
	_ptime->tv_sec = timeout;
}
commObject::~commObject()
{
#ifdef WIN32
	closesocket(sd);
#else
	close(sd);
#endif
	delete _ptime;
}
void commObject::init(const char* coor, int coorPort)
{
	struct  hostent  *he; 
#ifdef WIN32 
    //set up winsock stuff
    WSADATA wsa;
    if (WSAStartup(0x0101, &wsa) == SOCKET_ERROR)
		fprintf(stderr, "Error in WSAStartup.\n");
#endif

    memset(&srv_sin, 0, sizeof(srv_sin));
	srv_sin.sin_family = AF_INET;
	srv_sin.sin_port = htons(coorPort);
    he = gethostbyname(coor);
	if (he != NULL)
		memcpy(&srv_sin.sin_addr, he->h_addr, he->h_length);
    else if ( (srv_sin.sin_addr.s_addr = inet_addr(coor)) == INADDR_NONE )
		errexit("Client can't get \"%s\" host entry\n", coor);

    memset(&cli_sin, 0, sizeof(cli_sin));
    cli_sin.sin_family = AF_INET;
    cli_sin.sin_addr.s_addr = htonl ( INADDR_ANY );
	cli_sin.sin_port = htons(0);

    memset(&peer_sin, 0, sizeof(peer_sin));
	peer_sin.sin_family = AF_INET;

#ifdef WIN32
	int sin_len;
#else
	unsigned int sin_len;
#endif
	sin_len = sizeof(srv_sin);
	if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		errexit("creat socket failed:%s.\n", strerror(errno));
	if (bind(sd, (struct sockaddr* )&cli_sin, sizeof(cli_sin)) < 0)
		errexit("bind socket failed:%s.\n", strerror(errno));
	sin_len = sizeof(cli_sin);
	if (getsockname(sd, (struct sockaddr*)&cli_sin, &sin_len) < 0)
		errexit("getsockname failed:%s.\n", strerror(errno));
}
void commObject::tellGod(const char* pMsg, size_t sz)
{
#ifdef WIN32
	int sin_len;
#else
	unsigned int sin_len;
#endif
	sin_len = sizeof(srv_sin);
	if (sendto (sd, pMsg, sz, 0, (struct sockaddr* ) &srv_sin, sin_len) <0)
		errexit("commObject/tellGod: can't send message to coordinator: %s\n", strerror(errno));
}
size_t commObject::sendMsg(unsigned long destIp, const char* pBuff, size_t sz)
{
	size_t ret = 0;
	hostInfo* pHost;
	IPmap::iterator it = _IP2IP.find(destIp);
	if (it == _IP2IP.end())
	{
		printf("commObject::sendMsg: didn't found destIp 0x%x\n", destIp);
		pHost = queryConnection(destIp);
	}
	else
	{
#ifdef _TRACE
		printf("commObject::sendMsg: found destIp 0x%x\n", destIp);
#endif
		pHost = (*it).second;
	}
	if (pHost == NULL)		// dest not available yet. drop msg
		return ret;
    memset(&peer_sin, 0, sizeof(peer_sin));
	peer_sin.sin_family = AF_INET;
	peer_sin.sin_port = htons(pHost->_port);
#ifdef WIN32
	peer_sin.sin_addr.s_addr = (pHost->_IPAddr).S_un.S_addr;
	int sin_len;
#else
	peer_sin.sin_addr.s_addr = (pHost->_IPAddr).s_addr;
	unsigned int sin_len;
#endif
	sin_len = sizeof(peer_sin);
	if ((ret=sendto (sd, pBuff, sz, 0, (struct sockaddr* ) &peer_sin, sin_len)) <0)
		printf("commObject::sendMsg: can't send message to peer: %s\n", strerror(errno));
	return ret;
}

size_t commObject::readMsg(char* pBuff, size_t sz)
{
	int ns;
	size_t ret = 0;
	fd_set	readfds;
	FD_ZERO(&readfds);					// reset the FD, because NT.
	FD_SET(sd, &readfds );
	ns = select(sd+1, &readfds, NULL, NULL, _ptime );
	if (ns < 0)
		printf("commObject/readMsg: error in select sockets: %s\n",strerror(errno));
	else if (ns > 0)
	{
#ifdef WIN32
		int sin_len;
#else
		unsigned int sin_len;
#endif
		sin_len = sizeof(peer_sin);
		ret = recvfrom(sd, pBuff, sz, 0, (struct sockaddr* ) &peer_sin, &sin_len);
		if (sz < 0)
			printf("commObject/readMsg: error in recv message: %s\n", strerror(errno));
	}
	return ret;
}

hostInfo* commObject::queryConnection(unsigned long destIp)
{
	hostInfo* pRet = NULL;

	_buff[0]=MSGTYPE_QUERYIP;
	unsigned long *lp = (unsigned long *) &_buff[1];
	*lp++ = destIp;
	size_t sz = (sizeof(long)+1);
#ifdef WIN32
	int sin_len;
#else
	unsigned int sin_len;
#endif
	sin_len = sizeof(srv_sin);
	if (sendto (sd,_buff,sz,0,(struct sockaddr* )&srv_sin,sin_len)<0)
		printf("commObject/queryConnection: can't send message to coordinator: %s\n", strerror(errno));
	else
	{
//		sz = readMsg(_buff, MSG_SIZE);
		for (;;)
		{
			sz=recvfrom(sd,_buff,MSG_SIZE,0,(struct sockaddr* )&peer_sin,&sin_len);
			if (ntohs(peer_sin.sin_port) == ntohs(srv_sin.sin_port))
				break;
		}
		if (sz < 0)
			printf("commObject/queryConnection: can't recv message from coordinator: %s\n", strerror(errno));
		else
		{
			lp = (unsigned long *)_buff;
			struct in_addr realIP;
#ifdef WIN32
			realIP.S_un.S_addr = *lp++;
#else
			realIP.s_addr = *lp++;
#endif
			long port = (long) *lp;
			pRet = new hostInfo(destIp, realIP, port);
			_IP2IP.insert(IPmap::value_type(destIp, pRet));
#ifdef _TRACE
	printf("commObject::queryConnection: for IP=0x%x, from %d - sz=%d, realIP=%s,port=%d.\n"
		,destIp, ntohs(peer_sin.sin_port), sz, inet_ntoa(pRet->_IPAddr), pRet->_port);
#endif
		}
	}
	return pRet;
}	