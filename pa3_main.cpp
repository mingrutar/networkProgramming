//
// pa3_main.cpp
//
#include "routing.h"

#include "data.cpp"

char			*pmyAS;
long			ASid;
netPrifixMap	netPrefixTable;

static int		rcount;
static void*	routers[MAX_NODES];
static router*	pBGP;			//BGP, we not implemented it yet
static int		hcount;
static void*	hosts[MAX_NODES];
#ifdef WIN32
static HANDLE* threads;
void start_s(void* pObj)			// for send
{
	((base*) pObj)->run();
}
#else
static pthread_t**	threads;
void *start(void* pObj)
{
	((base*) pObj)->run();
}
#endif

FILE *output;

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		printf("Usage: pa3 n coorHost coorIP - where n is AS id.\n");
		exit(0);
	}
	ASid = atol(argv[1]);
	int coorPort = atoi(argv[3]);
	int				ii;
	st_netPrefix*	pprefix;
	st_router*		prouter;
	st_host*		phost;
	unsigned long	ipass;

	char temp[20];
	sprintf(temp, "AS%d.txt", ASid);
   if ((output  = fopen( temp, "w" )) == NULL)
		errexit("error in creating output file");

	pmyAS = AS_root[ASid]._name;
	pprefix	= AS_root[ASid]._nps;
	for (ii = 0; ii < AS_root[ASid]._cnps; ii++)
	{
		ipass = inet_addr((pprefix+ii)->_ips);
		netPrefixTable.insert(netPrifixMap::value_type(ipass,(pprefix+ii)->_name));
	}
	rcount = AS_root[ASid]._crouters;
	prouter	= AS_root[ASid]._routers;
	router*	pr;
	int	rid;
	for (ii = 0; ii < rcount; ii++)
	{
		ipass = inet_addr((prouter+ii)->_ips);
		if (((prouter+ii)->_type) == 0)
		{
			pr = new router((prouter+ii)->_name, ipass);
			if (strlen((prouter+ii)->_ips1) > 0) 
			{
				ipass = inet_addr((prouter+ii)->_ips1);
				pr->addExtraIP(ipass);
			}
		}
		else
		{
			ipass = inet_addr((prouter+ii)->_ips);
			pr = new router((prouter+ii)->_name, ipass);
// more will be later. May be.
		}
		for (int j = 0; j < MAX_NBR; j++)
		{
			rid = (prouter+ii)->_nbrs[j];
			if	(rid != -1)
			{
				ipass = inet_addr((prouter+rid)->_ips);
				pr->setNbr(ipass, (prouter+ii)->_cost[j]);
			}
		}
		pr->init(argv[2], coorPort);
		if (((prouter+ii)->_type) == 0)
			routers[ii] = pr;
		else
//			pBGP = pr;
			routers[ii] = pr;
	}
	hcount	= AS_root[ASid]._chosts;
	phost	= AS_root[ASid]._hosts;
	unsigned long ipR;
	host*	ph;
	for (ii = 0; ii < hcount; ii++)
	{
		ipass = inet_addr((phost+ii)->_ips);
		rid = (phost+ii)->_router;
		ipR = inet_addr((prouter+rid)->_ips);
		ph = new host((phost+ii)->_name, ipass, ipR);
		ph->init(argv[2], coorPort);
		hosts[ii] = ph;
	}
	int it = 0;
#ifdef WIN32
	threads = new HANDLE[rcount+hcount];
	unsigned long threadID;      // pointer to receive thread ID
/*	threads[it] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)&start_s,pBGP,0,&threadID);
	if (threads[it++] == NULL)
		errexit("error in creating router thread");
	for (ii = 0; ii < rcount; ii++, it++) */
	for (ii = 0; ii < rcount; ii++, it++)
	{
		threads[it] = CreateThread(NULL,0,
			(LPTHREAD_START_ROUTINE)&start_s,*(routers+ii),0,&threadID);
		if (threads[it] == NULL)
			errexit("error in creating router thread");
	}
	for (ii = 0; ii < hcount; ii++, it++)
	{
		threads[it] = CreateThread(NULL,0,
			(LPTHREAD_START_ROUTINE)&start_s,*(hosts+ii),0,&threadID);
		if (threads[it] == NULL)
			errexit("error in creating host thread");
	}
	WaitForMultipleObjects((rcount+hcount),threads,TRUE,INFINITE);	
#else
	threads = new pthread_t*[rcount+hcount];
//	if (pthread_create(threads[0], NULL, start, pBGP)) 
//		errexit("error in creating router thread");
	for (ii = 0; ii < rcount; ii++, it++)
	{
		if (pthread_create(threads[it], NULL, start, *(routers+ii))) 
			errexit("error in creating router thread");
	}
	for (; ii < hcount; ii++, it++)
	{
		if (pthread_create(threads[it], NULL, start, *(hosts+ii))) 
			errexit("error in creating host thread");
	}
	for (ii = 0; ii < (rcount+hcount); ii++)
		pthread_join(*threads[ii], 0);
#endif
	fclose(output);
	printf("Exit...");
	exit(0);
}


