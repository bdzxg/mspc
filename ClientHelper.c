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

//TODO hard code

static char* get_inner_cltype(char* client)
{
	D("get_inner_cltype putin %s", client);
	if(!strcasecmp(client,"PC")) return "PCCL";
	if(!strcasecmp(client,"SMARTPHONE")) return "SMTP";
	if(!strcasecmp(client,"POCKETPC")) return "PCKT";
	if(!strcasecmp(client,"SYMBIAN")) return "SYMB";
	if(!strcasecmp(client,"J2ME")) return "JTME";
	if(!strcasecmp(client,"WAP")) return "WAPP";
	if(!strcasecmp(client,"WEB")) return "WEBB";
	if(!strcasecmp(client,"WV")) return "WVWV";
   	if(!strcasecmp(client, "POC")) return "POCC"; 
	if(!strcasecmp(client, "SYMBIANR1")) return "SYBA";
	if(!strcasecmp(client, "J2MER1")) return "JTEA";
	if(!strcasecmp(client, "LINUXMOTO")) return "LXMT";
	if(!strcasecmp(client, "SMS")) return "SMSS";
	if(!strcasecmp(client, "OMS")) return "OMSS";
	if(!strcasecmp(client, "IPHONE")) return "IPHO";
	if(!strcasecmp(client, "ANDROID")) return "ANDR";
	if(!strcasecmp(client, "MTKVRE")) return "MKVR";
	if(!strcasecmp(client, "MTKMYTHROAD")) return "MKMY";
	if(!strcasecmp(client, "MTKDALOADER")) return "MKDA";
	if(!strcasecmp(client, "MTKMORE")) return "MKMO";
	if(!strcasecmp(client, "ROBOT")) return "ROBT";
	if(!strcasecmp(client, "BLACKBERRY")) return "BKBR";
	if(!strcasecmp(client, "PCGAME")) return "PCGM";
	if(!strcasecmp(client, "WEBGAME")) return "WBGM";
	if(!strcasecmp(client, "PCMUSIC")) return "PCMS";
	if(!strcasecmp(client, "PCSMART")) return "PCSM";
	if(!strcasecmp(client, "WEBBAR")) return "WBAR";
	if(!strcasecmp(client, "IPAD")) return "IPAD";
	if(!strcasecmp(client, "WEBUNION")) return "WUNION";
	if(!strcasecmp(client, "MTKMEX")) return "MKMX";
	if(!strcasecmp(client, "MTKSXM")) return "MKSM";
	if(!strcasecmp(client, "MTK")) return "MTKK";
	if(!strcasecmp(client, "EANDROID")) return "EAND";
	if(!strcasecmp(client, "ESYMBIAN")) return "ESYM";
	if(!strcasecmp(client, "EIPHONE")) return "EIPH";
	if(!strcasecmp(client, "WINPHONE")) return "WPHO";
	if(!strcasecmp(client, "WIN8")) return "WIN8";
	if(!strcasecmp(client, "PCMAC")) return "PCMC";
	if(!strcasecmp(client, "SMSDEP")) return "SMSD";
	if(!strcasecmp(client, "SMSBP")) return "SMSB";

	return "ANDR";
}

static char* get_client_type(int num)
{
	switch(num)
	{
	case 1: return "PC";
	case 2: return "SMARTPHONE"; 
	case 3: return "POCKETPC";
	case 4: return "SYMBIAN";
	case 5: return "J2ME"; 
	case 6: return "WAP"; 
	case 7: return "WEB"; 
	case 8: return "WV";
	case 9: return "POC"; 
	case 10: return "SYMBIANR1"; 
	case 11: return "J2MER1"; 
	case 12: return "LINUXMOTO"; 
	case 13: return "SMS"; 
	case 14: return "OMS"; 
	case 15: return "IPHONE"; 
	case 16: return "ANDROID"; 
	case 17: return "MTKVRE"; 
	case 18: return "MTKMYTHROAD"; 
	case 19: return "MTKDALOADER"; 
	case 20: return "MTKMORE"; 
	case 21: return "ROBOT"; 
	case 22: return "BLACKBERRY"; 
	case 23: return "PCGAME"; 
	case 24: return "WEBGAME"; 
	case 25: return "PCMUSIC"; 
	case 26: return "PCSMART"; 
	case 27: return "WEBBAR"; 
	case 28: return "IPAD"; 
	case 29: return "WEBUNION"; 
	case 30: return "MTKMEX"; 
	case 31: return "MTKSXM"; 
	case 32: return "MTK"; 
	case 33: return "EANDROID"; 
	case 34: return "ESYMBIAN"; 
	case 35: return "EIPHONE"; 
	case 36: return "WINPHONE"; 
	case 37: return "WIN8"; 
	case 38: return "PCMAC"; 
	case 39: return "SMSDEP"; 
	case 40: return "SMSBP"; 
	}
	return "ANDROID";
}

static char* get_client_version(int version)
{
	switch(version) {
	case 1: return "0100";
	case 2: return "0101";
	case 3: return "0102";
	case 4: return "0103";
	case 5: return "0104";
	case 6: return "0105";																	         
	case 7: return "0106";
	case 8: return "0107";
	case 9: return "0108";
	case 10: return "0109";
				 
	case 11: return "0200";
	case 12: return "0201";
	case 13: return "0202";
	case 14: return "0203";
	case 15: return "0204";
	case 16: return "0205";
	case 17: return "0206";
	case 18: return "0207";
	case 19: return "0208";
	case 20: return "0209";
				 
	case 21: return "0300";
	case 22: return "0301";
	case 23: return "0302";
	case 24: return "0303";
	case 25: return "0304";
	case 26: return "0305";
	case 27: return "0306";
	case 28: return "0307";
	case 29: return "0308";
	case 30: return "0309";
				 
	case 31: return "0400";
	case 32: return "0401";
	case 33: return "0402";
	case 34: return "0403";
	case 35: return "0404";
	case 36: return "0405";
	case 37: return "0406";
	case 38: return "0407";
	case 39: return "0408";
	case 40: return "0409";
	}
	return "0100";
}

static char* generate_client_epid(int type, int version)
{
	char* cl_type = get_client_type(type);
	char* cl_version = get_client_version(version);
	struct timeval tv;
	time_t timesec;
	struct tm *p;
	time(&timesec);
	p = gmtime(&timesec);
	gettimeofday(&tv, NULL);
	//char* tep = calloc(7, 1);
	//sprintf(tep, "%lu", tv.tv_usec);
	//char* tep_t = calloc(5, 1);
	//strncpy(tep_t, tep, 4);
	char* t = calloc(20, 1);	
	sprintf(t, "%02d%02d%lu", p->tm_min, p->tm_sec, tv.tv_usec);
	char* cltype = get_inner_cltype(cl_type);
	char* epid = calloc(strlen(t)+ strlen(cltype) + strlen(cl_version) + 1, 1);
	sprintf(epid, "%s%s%s", cltype,  cl_version, t);
	//free(tep);
	//free(tep_t);
	free(t);
	return epid;
}

static char* get_now_time()
{
	time_t tmp_time;
	struct tm *ptime;
	tmp_time = time(NULL);//获取当前时间
	ptime = localtime(&tmp_time);
	char* t = calloc(50, 1);	
	sprintf(t, "%d-%d-%d %d:%d:%d\n",(1900+ptime->tm_year),(1+ptime->tm_mon),ptime->tm_mday,	ptime->tm_hour,ptime->tm_min,ptime->tm_sec);
	W("%d-%d-%d %d:%d:%d\n",(1900+ptime->tm_year),(1+ptime->tm_mon),ptime->tm_mday,	ptime->tm_hour,ptime->tm_min,ptime->tm_sec);
	return t;
}

