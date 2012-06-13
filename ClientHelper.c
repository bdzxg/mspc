/*
 * =====================================================================================
 *
 *       Filename:  ClientHelper.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/02/2012 05:37:58 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <time.h>
#include "proxy.h"

char* GetClientType(char* client)
{
	if(!strcasecmp(client,"WV"))
		return "WVWV";
	else if(!strcasecmp(client, "POC"))
		return "POCC";
	else if(!strcasecmp(client, "SYMBIANR1"))
		return "SYBA";
	else if(!strcasecmp(client, "J2MER1"))
		return "JTEA";
	else if(!strcasecmp(client, "LINUXMOTO"))
		return "LXMT";
	else if(!strcasecmp(client, "SMS"))
		return "SMSS";
	else if(!strcasecmp(client, "OMS"))
		return "OMSS";
	else if(!strcasecmp(client, "IPHONE"))
		return "IPHO";
	else if(!strcasecmp(client, "ANDROID"))
		return "ANDR";
	else if(!strcasecmp(client, "MTKVRE"))
		return "MKVR";
	else if(!strcasecmp(client, "MTKMYTHROAD"))
		return "MKMY";
	else if(!strcasecmp(client, "MTKDALOADER"))
		return "MKDA";
	else if(!strcasecmp(client, "MTKMORE"))
		return "MKMO";
	else if(!strcasecmp(client, "ROBOT"))
		return "ROBT";
	else if(!strcasecmp(client, "BLACKBERRY"))
		return "BKBR";
	else if(!strcasecmp(client, "PCGAME"))
		return "PCGM";
	else if(!strcasecmp(client, "WEBGAME"))
		return "WBGM";
	else if(!strcasecmp(client, "PCMUSIC"))
		return "PCMS";
	else if(!strcasecmp(client, "PCSMART"))
		return "PCSM";
	else if(!strcasecmp(client, "WEBBAR"))
		return "WBAR";
	else if(!strcasecmp(client, "IPAD"))
		return "IPAD";
	else if(!strcasecmp(client, "WEBUNION"))
		return "WUNION";
	else if(!strcasecmp(client, "MTKMEX"))
		return "MKMX";
	else if(!strcasecmp(client, "MTKSXM"))
		return "MKSM";
	else if(!strcasecmp(client, "MTK"))
		return "MTKK";
	else if(!strcasecmp(client, "EANDROID"))
		return "EAND";
	else if(!strcasecmp(client, "ESYMBIAN"))
		return "ESYM";
	else if(!strcasecmp(client, "EIPHONE"))
		return "EIPH";
	else if(!strcasecmp(client, "WINPHONE"))
		return "WPHO";
	else if(!strcasecmp(client, "WIN8"))
		return "WIN8";
	else if(!strcasecmp(client, "PCMAC"))
		return "PCMC";
	else if(!strcasecmp(client, "SMSDEP"))
		return "SMSD";
	else if(!strcasecmp(client, "SMSBP"))
		return "SMSB";
	else
	    return NULL;
}

char* GenerateClientEpid(char* type, char* version)
{
	struct timeval tv;
	time_t timesec;
	struct tm *p;
	time(&timesec);
	p = gmtime(&timesec);
    //	D("utc_now min %d, sec %d", p->tm_min, p->tm_sec);
	gettimeofday(&tv, NULL);
    //	D("tv.tv_usec %d",tv.tv_usec);
	char tep[7] = {0,0,0,0,0,0,0};
	sprintf(tep, "%d", tv.tv_usec);
	char tep_t[5] = {0,0,0,0,0};
    
	strncpy(tep_t, tep, 4);
	char t[15];
	int i = 0;
	for(i = 0; i < 15; i++)
		t[i] = 0;
	sprintf(t, "%02d%02d%04s", p->tm_min, p->tm_sec, tep_t);
	D("epid time part %s", t);
	char* epid = malloc(20);
	if(version == NULL)
	{
		sprintf(epid, "%s%s%s", GetClientType(type), "0000", t);
	}
	else
	{
		// to do add version process
	}
	return epid;
}
