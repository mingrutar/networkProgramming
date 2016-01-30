//
// routing.cpp 
//
#include "routing.h"

extern FILE *output;

/*****************
 *  base
 *****************/
base::base(const char* pName, unsigned long myIP)
{
	_myIP = myIP;
	_pcomm = new commObject(ROUTER_INTERVAL);
	_name = pName;
}
base::~base()
{
	delete _pcomm;
}
unsigned long base::toPrefix(unsigned long ip)
{
	struct in_addr ia;
#ifdef WIN32
	ia.S_un.S_addr = ip;
#else
	ia.s_addr = ip;
#endif	
	char *pDest =  inet_ntoa (ia);
	char *p = strrchr(pDest, '.');
	*++p = '0';
	*++p = '\0';
	return inet_addr(pDest);
}
char* base::toIPString(unsigned long ip)
{
	struct in_addr ia;
#ifdef WIN32
	ia.S_un.S_addr = ip;
#else
	ia.s_addr = ip;
#endif	
	return inet_ntoa (ia);
}
/*****************
 *  host
 *****************/
host::host(const char* pName, unsigned long myIP, unsigned long myRouter)
	: base(pName, myIP)
{
	_router = myRouter;
}
void host::init(const char* coor, int coorPort)
{
	_pcomm->init(coor, coorPort);
	_outbuf[0] = MSGTYPE_REG;
	unsigned long* lp = (unsigned long*)(&_outbuf[1]);
	size_t sz = strlen(_name)+(3*sizeof(long))+strlen(pmyAS)+1;
	*lp++ = sz;
	*lp++ = ASid;
	*lp++ = _myIP;
	char* p = (char*) lp;
	sprintf(p, "%s", _name);
	_pcomm->tellGod(_outbuf, (sz+2));
}
void host::run()
{
	printf("From %s: host::run called.\n",_name);
	bool bRun = true;
	size_t sz;
	while (bRun)
	{
		if ((sz = _pcomm->readMsg(_inbuf, BUFF_SIZE)) > 0)
		{
			switch (_inbuf[0])
			{
			case MSGTYPE_CMSG:
				sendMsg(&_inbuf[1], (sz-1));
				break;
			case MSGTYPE_HMSG:
				recvMsg(&_inbuf[1], (sz-1));
				break;
			case MSGTYPE_CQUIT:
				bRun = false;
				break;
			default:
				printf("From %s: unknown message type %d. The message is dropped.\n",_name,_inbuf[0]);
			}
		}
	}
}
void host::sendMsg(const char* pMsg, size_t msgSz)
{
	unsigned long* ulp = (unsigned long*) pMsg;
	_outbuf[0] = MSGTYPE_HMSG;	// msg type
	unsigned long* ulp1 = (unsigned long*)&_outbuf[1];
	unsigned long dest = *ulp++;			//dest
	*ulp1++ = dest;			//dest
	*ulp1++ = _myIP;
	*ulp1++ = 0;
	memset(ulp1, 0, sizeof(unsigned long)*HOP_SIZE);
	size_t sz = *ulp++;
	if (msgSz >= (sz+sizeof(long)*2))
	{
		char *p = (char*) ulp;
		*(p+sz) = '\0';				// make sure proper terminated
		char *p1 = (char*) (ulp1+HOP_SIZE);
		strcpy(p1, p);
		_pcomm->sendMsg(_router, _outbuf, (sizeof(_header)+sz+2));
		printf("From %s: send to 0x%x msg %s.\n",_name,dest,p);
	}
	else
		printf("From %s: error - message from coordinator size mismatch, message dropped.\n",_name);
}
void host::recvMsg(const void* pMsg, size_t msgSz)
{
	unsigned long* ulp = (unsigned long*) pMsg;
	struct in_addr ia;
	_header.destIP = *ulp++;
#ifdef WIN32
	ia.S_un.S_addr = _header.destIP;
#else
	ia.s_addr = _header.destIP;
#endif
	char *pDest =  inet_ntoa (ia);
#ifdef WIN32
	ia.S_un.S_addr = *ulp++;		// src
#else
	ia.s_addr = *ulp++;		// src
#endif
	char *pSrc =  inet_ntoa (ia);
#ifdef WIN32
	ia.S_un.S_addr = _myIP;
#else
	ia.s_addr = _myIP;
#endif
	if (_header.destIP != _myIP)
	{
		printf("From %s: error - the message from %s is destinated for %s. My address is %s.\n"
			, pSrc, pDest, inet_ntoa(ia));
		fprintf(output,"From %s: error - the message from %s is destinated for %s. My address is %s.\n"
			, pSrc, pDest, inet_ntoa(ia));
	}
	else
	{
		_header.length = *ulp++;
		char* p = (char*) (ulp+HOP_SIZE);
		printf("From %s: the message is from %s through %d routers:\n"
			,_name, pSrc, _header.length);
		fprintf(output,"From %s: the message is from %s through %d routers:\n"
			,_name, pSrc, _header.length);
		char rtr[20];
		for (unsigned int i = 0; i < _header.length; i++)
		{
#ifdef WIN32
			ia.S_un.S_addr = *ulp++;
#else
			ia.s_addr = *ulp++;
#endif
			strcpy(rtr, inet_ntoa (ia));
			if (i == 0)
			{
				printf(" **[hop %d, %s]", i, rtr);
				fprintf(output," **[hop %d, %s]", i, rtr);
			}
			else
			{
				printf(" -> [hop %d, %s]", i, rtr);
				fprintf(output," ->[hop %d, %s]", i, rtr);
			}
		}
		printf(".\n ++ msg: %s.\n",p);
		fprintf(output,".\n ++ msg: %s.\n",p);
	}
	fflush(output);
}
/*****************
 *  router
 *****************/
router::router(const char* pName, unsigned long myIP)
	: base(pName, myIP)
{
	_myIP2 = 0xFFFFFFFF;
	_cprop = 0;
}
void router::init(const char* coor, int coorPort)
{
	_pcomm->init(coor, coorPort);
	_outbuf[0] = MSGTYPE_REG;
	unsigned long* lp = (unsigned long*)(&_outbuf[1]);
	size_t sz = strlen(_name)+(3*sizeof(long))+strlen(pmyAS)+1;
	*lp++ = sz;
	*lp++ = ASid;
	*lp++ = _myIP;
	char* p = (char*) lp;
	sprintf(p, "%s:%s", pmyAS, _name);
	_pcomm->tellGod(_outbuf, (sz+2));
}
void router::addExtraIP(unsigned long ip)
{
	_myIP2 = ip;
}
void router::setNbr(unsigned long nbr, long cost)
{
	_nbr_table.insert(pa3Map::value_type(nbr, cost));
#ifdef _TRACE
	printf("From %s: setNbr insert nbr %lu, cost %ld\n",_name, nbr,cost);
#endif
}
void router::run()
{
	printf("From %s: router::run called.\n",_name);
	pa3Map::iterator nt_it;
	unsigned long nbrIPprf;
	_dv_table.insert(pa3Map::value_type(toPrefix(_myIP), 0));
	for (nt_it = _nbr_table.begin(); nt_it != _nbr_table.end(); nt_it++)
	{
		nbrIPprf = toPrefix((*nt_it).first);
/*		if ((nbrIP.S_un.S_addr>>8) != (_myIP.S_un.S_addr>>8))// from other AS?
		{
//			nbrIP.S_un.S_addr != 0x80000000; this may not work
		}	*/
		_dv_table.insert(pa3Map::value_type(nbrIPprf, (*nt_it).second));
		_fwd_table.insert(IFWMap::value_type(nbrIPprf, (*nt_it).first));
	}
	printf("initial DV..");
	printDV();
	printFT("run");
	propaganda_DV();

	bool bRun = true;
	while (bRun)
	{
		size_t sz = _pcomm->readMsg(_inbuf, BUFF_SIZE);
		if (sz == 0)
			propaganda_DV();	 
		else
		{
			switch (_inbuf[0])
			{
			case MSGTYPE_HMSG:
				forwrdMsg(&_inbuf[1], sz);
				break;
			case MSGTYPE_RDV:
				updateDV(&_inbuf[1], (sz-1));
				break;
			case MSGTYPE_CCHNG:
				changeCost(&_inbuf[1], (sz-1));
				break;
			case MSGTYPE_CDN:
				downLink(&_inbuf[1], (sz-1));
				break;
			case MSGTYPE_CASDN:
				ASdownLink(&_inbuf[1], (sz-1));
				break;
			case MSGTYPE_CASUP:
				ASupink(&_inbuf[1], (sz-1));
				break;
			case MSGTYPE_CPRNT:
				printFT("MSGTYPE_CPRNT");
				break;
			case MSGTYPE_CQUIT:
				bRun = false;
				break;
			case MSGTYPE_CPRNT_DV:
				printDV();
				break;
			case MSGTYPE_CPRNT_NT:
				printNT();
				break;
			default:
				printf("From %s: unknown message type %d. The message is dropped.\n",_name,_inbuf[0]);
			}
		}
	}
}
void router::forwrdMsg(const char* pMsg, size_t sz)
{
	unsigned long* ulp = (unsigned long*) pMsg;
	unsigned long dest;
	dest = *ulp++;
	ulp++;					//srcIP
	unsigned long len = *ulp;
	(*ulp)++;
	ulp++;
	*(ulp+len) = _myIP;
	if (toPrefix(dest) == toPrefix(_myIP))
	{
		_pcomm->sendMsg(dest, _inbuf, sz);
		printf("From %s: forwrdMsg - delivered message to host 0x%x.\n"
			,_name,dest);
	}
	else
	{
		struct in_addr	ia;
#ifdef WIN32
		ia.S_un.S_addr = dest;
#else
		ia.s_addr = dest;
#endif	
		IFWMap::iterator ft_it = _fwd_table.find(toPrefix(dest));
		if (ft_it == _fwd_table.end())
		{ //??ming look BGP table
			printf("From %s: Cannot forwar to %s. Messag is dropped\n",_name, inet_ntoa(ia));
			return;
		}
		_pcomm->sendMsg((*ft_it).second, _inbuf, sz);
		printf("From %s: forwrdMsg - forward message for 0x%x to router 0x%x.\n",
			_name,dest,(*ft_it).second);
	}
}
void router::updateDV(const char* pBuf, size_t sz)
{
	unsigned long*	lp = (unsigned long*)pBuf;
	unsigned long	fromAddr= *lp++;		// source Router
	struct in_addr ia;
#ifdef WIN32
	ia.S_un.S_addr = fromAddr;
#else
	ia.s_addr = fromAddr;
#endif	
	size_t			numDV = *lp++;			// numberDV
#ifdef _TRACE
	printf("From %s/updateDV: called fromAddr %s, msgSize=%d,numEnter=%d.\n",
		_name, inet_ntoa(ia), sz, numDV);
#endif
	if ((sz - sizeof(long) - sizeof(long)) < numDV*sizeof(long))
	{
		printf("From %s-updateDV: number entry is %d, msgSize=%d, not match\n"
			,_name, numDV, sz);
		return;
	}
	pa3Map::iterator nt_it = _nbr_table.find(fromAddr);	// get the neighbor
	if (nt_it == _nbr_table.end())
	{
		printf("From %s: Unknown neighour %s.\n", _name, inet_ntoa(ia));
		return;
	}
	pa3Map::iterator dv_it;
	unsigned long	dv_IP;
	long			dv_cost;
	bool			bMod = false;
	struct in_addr ria;
	IFWMap::iterator ft_it;
	for (size_t i = 0 ; i < numDV; i++)
	{
		dv_IP = (unsigned long) *lp++;
		dv_cost = *lp++;

		if (dv_IP == toPrefix(_myIP))
			continue;
#ifdef WIN32
		ria.S_un.S_addr = dv_IP;
#else
		ria.s_addr = dv_IP;
#endif
		dv_cost	+= (*nt_it).second;
		dv_it = _dv_table.find(dv_IP);
		if (dv_it != _dv_table.end())			// is it in our dv
		{
			if ((*dv_it).second == -1)			// was down
			{
				nt_it = _nbr_table.find(dv_IP);	// if link is down, will
				if (nt_it != _nbr_table.end())	// not accept from nbr
					bMod = true;
			}
			else if ((*dv_it).second > dv_cost)
			{
				printf("--From %s: updateDV  %d: change for %s new_cost=%d, my_dv=%d\n", 
					_name, i, inet_ntoa(ria), dv_cost,(*dv_it).second);
				bMod = true;
			} 
			else
			{
				ft_it = _fwd_table.find(dv_IP);
				printFT("updateDV 3");
				char temp[30];
				strcpy(temp, inet_ntoa(ia));
				if ((ft_it != _fwd_table.end())&&((*ft_it).first==dv_IP)&&
					((*ft_it).second==fromAddr)&&((*dv_it).second!=dv_cost))
				{
					printf("--From %s: updateDV nbr=%s, prfx=%s change: new_cost=%d, my_dv=%d\n", 
						_name, temp, inet_ntoa(ria), dv_cost,(*dv_it).second);
					bMod = true;
				}
			}
			if (bMod)
			{
				_dv_table.erase(dv_it);		// delete old one
				_dv_table.insert(pa3Map::value_type(dv_IP, dv_cost));
				ft_it = _fwd_table.find(dv_IP);
				if (ft_it != _fwd_table.end())
				{
					_fwd_table.erase(ft_it);
					if (dv_cost != -1)
						_fwd_table.insert(IFWMap::value_type(dv_IP, fromAddr));
				}
				else
					printf("From %s-updateDV: FT couldn't find dv_IP=%s, cost=%d, from 0x%x",
						_name, inet_ntoa(ria), dv_cost, fromAddr);
			}
		}
		else
		{
			bMod = true;
			_dv_table.insert(pa3Map::value_type(dv_IP, dv_cost));
			_fwd_table.insert(IFWMap::value_type(dv_IP, fromAddr));
			printf("--From %s: updateDV %d: insert new_dv=(%s,%d))\n",
				_name,i, inet_ntoa(ria), dv_cost);
		}
	}
	if (bMod)
		propaganda_DV();
}
void router::changeCost(const char* pBuf, size_t sz)
{
	unsigned long*	lp = (unsigned long*)pBuf;
	lp++;							// skip AS
	unsigned long	rIP;
	rIP = *lp++;
	if (rIP == _myIP)
		rIP = *lp++;				// source Router
	else
		lp++;
	long	dCost = *lp++;	// new cost
	pa3Map::iterator nt_it = _nbr_table.find(rIP);	// get the neighbor
	if (nt_it == _nbr_table.end())
	{
		printf("From %s: changeCost - wrong link nbrIP=0x%x, newCost=%d.\n"
			,_name, rIP, dCost);
		return;
	}
	long nbrCost;
	long oldCost = (*nt_it).second;
	_nbr_table.erase(nt_it);			//delete old one
	_nbr_table.insert(pa3Map::value_type(rIP, dCost));
	dCost -= oldCost;
	pa3Map::iterator	dv_it;
	IFWMap::iterator	ft_it;
	bool				bMod = false;
	unsigned long prf;
	unsigned long hop;
	for (ft_it = _fwd_table.begin(); ft_it != _fwd_table.end(); ft_it++)
	{
		hop = (*ft_it).second;
		printf("From %s: changeCost - prf=%s,fw=%s.\n",_name,toIPString(prf),toIPString(hop));
		if (hop == rIP)
		{
			prf = (*ft_it).first;
			dv_it = _dv_table.find(prf);
			oldCost = (*dv_it).second;
			_dv_table.erase(dv_it);				//delete old one
			//if current path cost more then we have in neibour table, use neibour
			for (nt_it = _nbr_table.begin(); nt_it != _nbr_table.end(); nt_it++)
			{
				if ((toPrefix((*nt_it).first)==prf)&&((*nt_it).second<(oldCost+dCost)))
				{
					_dv_table.insert(pa3Map::value_type(prf,(*nt_it).second));
					_fwd_table.erase(ft_it);
					_fwd_table.insert(IFWMap::value_type(prf, (*nt_it).first));
					bMod = true;
					break;
				}
			}
			if (!bMod)
			{
				_dv_table.insert(pa3Map::value_type(prf, (oldCost+dCost)));
				bMod = true;
			}
		}
	}
	if (bMod)
		propaganda_DV();
}	
bool router::updateTables(unsigned long rIP, bool bAS)
{
	bool ret;
	pa3Map::iterator dv_it, nt_it;
	IFWMap::iterator ft_it;
	unsigned long prf = toPrefix(rIP);
	nt_it = _nbr_table.find(rIP);	// if it is our nbr 
	bool			bRep = false;
	if ((!bAS) && (nt_it != _nbr_table.end()))
		bRep = true;
	unsigned long oldPfx;
	for (ft_it = _fwd_table.begin(); ft_it != _fwd_table.end(); ft_it++)
	{
		if ((*ft_it).second == rIP)
		{
			oldPfx = (*ft_it).first;
			dv_it = _dv_table.find(oldPfx);
			_dv_table.erase(dv_it);			//delete old one
			_fwd_table.erase(ft_it);
			if ((bRep) && (oldPfx != prf))			// change to nbr
			{
				_dv_table.insert(pa3Map::value_type((*nt_it).first, (*nt_it).second));
				_fwd_table.insert(IFWMap::value_type(oldPfx, (*nt_it).first));
			}
			else
				_dv_table.insert(pa3Map::value_type(oldPfx, -1));
			ret = true;
			if (_fwd_table.empty())
				break;
		}
	}
#ifdef _TRACE
	printf("From %s: updateTables rIP = %lu\n",_name, rIP);
#endif
	return ret;
}
void router::downLink(const char* pBuf, size_t sz)
{
	unsigned long* lp = (unsigned long*)pBuf;
	lp++;							// skip AS
	unsigned long	rIP;
	rIP = *lp++;
	if (rIP == _myIP)
		rIP = *lp++;				// source Router
	else
		lp++;

	if (updateTables(rIP, false))
		propaganda_DV();
}
// ??? ming not done yet
void router::ASdownLink(const char* pBuf, size_t sz)
{
	unsigned long* lp = (unsigned long*)pBuf;
	lp++;							//skip AS id1
	unsigned long rIP;
	rIP = *lp++;
	if (rIP == _myIP)
	{
		lp++;						// skip AS id2
		rIP = *lp++;				// source Router
	}
	else
	{
		lp++;
		lp++;
	}
	if (updateTables(rIP, true))
		propaganda_DV();
}
void router::ASupink(const char* pBuf, size_t sz)
{
	unsigned long* lp = (unsigned long*)pBuf;
	lp++;							//skip AS id1
	unsigned long	rIP;
	rIP = *lp++;
	if (rIP == _myIP)
	{
		lp++;						// skip AS id2
		rIP = *lp++;				// source Router
	}
	else
	{
		lp++;
		lp++;
	}
	bool	bMod = false;
	pa3Map::iterator nt_it = _nbr_table.find(rIP);
	struct in_addr	ia;
#ifdef WIN32
	ia.S_un.S_addr = rIP;
#else
	ia.s_addr = rIP;
#endif	
	if (nt_it == _nbr_table.end())
	{
		printf("From %s: unknown link to router %s. The message is ignored.\n"
			,_name,inet_ntoa(ia));
		return;
	}
	unsigned long prf = toPrefix(rIP);
	pa3Map::iterator dv_it = _dv_table.find(prf);
	IFWMap::iterator ft_it = _fwd_table.find(prf);
	if (dv_it != _dv_table.end())
		_dv_table.erase(dv_it);
	if (ft_it != _fwd_table.end())
		_fwd_table.erase(ft_it);
	_dv_table.insert(pa3Map::value_type(prf, (*nt_it).second));
	_fwd_table.insert(IFWMap::value_type(prf, rIP));
	if (bMod)
		propaganda_DV();
}
void router::printFT(const char *p)
{
	IFWMap::iterator ft_it;
	printf("Forward table for router %s:%s, called from %s:\n",pmyAS, _name, p);
	fprintf(output,"Forward table for router %s:%s:\n",pmyAS,_name);
	struct in_addr	pfx;
	struct in_addr	hop;
	char	pPfx[50];
	char	pHop[50];
	for (ft_it = _fwd_table.begin(); ft_it != _fwd_table.end(); ft_it++)
	{
#ifdef WIN32
		pfx.S_un.S_addr = (*ft_it).first;
		hop.S_un.S_addr = (*ft_it).second;
#else
		pfx.s_addr = (*ft_it).first;
		hop.s_addr = (*ft_it).second;
#endif
		strcpy(pPfx, inet_ntoa(pfx));
		strcpy(pHop, inet_ntoa(hop));
		printf("\t%s\t %s\n",pPfx,pHop);
		fprintf(output,"\t%s\t %s\n",pPfx,pHop);
	}
	fflush(output);
}
void router::printDV()
{
	pa3Map::iterator dv_it;
	printf(" -- Dump DV table  for router %s:\n",_name);
	struct in_addr	pfx;
	for (dv_it = _dv_table.begin(); dv_it != _dv_table.end(); dv_it++)
	{
#ifdef WIN32
		pfx.S_un.S_addr = (*dv_it).first;
#else
		pfx.s_addr = (*dv_it).first;
#endif
		printf("\t%s\t %d\n",inet_ntoa(pfx),(*dv_it).second);
	}
}
void router::printNT()
{
	pa3Map::iterator nt_it;
	printf(" -- Dump Nbr table  for router %s:\n",_name);
	struct in_addr	pfx;
	for (nt_it = _nbr_table.begin(); nt_it != _nbr_table.end(); nt_it++)
	{
#ifdef WIN32
		pfx.S_un.S_addr = (*nt_it).first;
#else
		pfx.s_addr = (*nt_it).first;
#endif
		printf("\t%s\t %d\n",inet_ntoa(pfx),(*nt_it).second);
	}
}
void router::propaganda_DV()
{
#ifdef WIN32
	Sleep(20);
#else
	usleep(20);
#endif
	pa3Map::iterator dv_it;
	_outbuf[0] = MSGTYPE_RDV;
	unsigned long* lp = (unsigned long*)(&_outbuf[1]);
	size_t i = 0;
	*lp++ = _myIP;
	i++;
	*lp++ = _dv_table.size() - 1;		// subtract myself
	i++;
#ifdef _TRACE
	printf("++From %s: propaganda_DV - ", _name);
#endif
	for (dv_it = _dv_table.begin(); dv_it != _dv_table.end(); dv_it++)
	{
		if ((*dv_it).second != 0)	// myself
		{
			*lp++ = (*dv_it).first;
			i++;
			*lp++ = (*dv_it).second;
			i++;
#ifdef _TRACE
			printf("%d=(0x%x,%d), ", i, (*dv_it).first, (*dv_it).second);
#endif
		}
	}
	pa3Map::iterator nt_it;
	for (nt_it = _nbr_table.begin(); nt_it != _nbr_table.end(); nt_it++)
	{
// need to kick out if not in my AS
		_pcomm->sendMsg((*nt_it).first, _outbuf, i*sizeof(long)+1);
	}
#ifdef _TRACE
	printf("From %s: propaganda_DV called %d #tableE=%d, i=%d.\n", _name, _cprop++,_dv_table.size(),i);
#endif
}