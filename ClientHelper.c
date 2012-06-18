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
	sprintf(t, "%02d%02d%4s", p->tm_min, p->tm_sec, tep_t);
	//D("epid time part %s", t);
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
 
char* get_cmd_func_name(int cmd)
{
	switch(cmd)
	{
		case 101:
			return "mcore-Reg1V5";
		case 102:
			return "mcore-Reg2V5";
		case 103:
			return "mcore-UnRegV5";
		case 104:
			return "mcore-KeepAliveV5";
		case 105:
			return "mcore-FlowV5";
		case 165:
			return "muser-HandleContactRequestV5";
		case 161:
		    return "muser-AddToBlacklistV5";
		case 162:
		    return "muser-RemoveFromBlacklistV5";
		case 163:
			return "muser-AddBuddyV5";
		case 164:
		    return "muser-DeleteBuddyV5";
		case 166:
		    return "muser-CreateBuddyListV5";
		case 167:
			return "muser-DeleteBuddyListV5";
		case 168:
			return "muser-SetBuddyListInfoV5";
		case 169:
			return "muser-AddChatFriendV5";
		case 170:
			return "muser-DeleteChatFriendV5";
		case 171:
			return "muser-GetContactInfoV5";
		case 172:
			return "muser-InviteToDownloadMCV5"; 
		case 173:
			return "muser-InviteUserV5";
		case 174:
			return "muser-PermissionRequestV5";
		case 175:
		    return "muser-PermissionResponseV5";
		case 176:
			return "muser-PresenceV5";
		case 180:
			return "muser-SmsForwardInfoV5";
		case 179:
			return "muser-SetUserInfoV5";
		case 177:
			return "muser-SetContactInfoV5";
		case 142:
			return "mgroup-PGGetGroupListV5";
		case 141:
		    return "mgroup-PGGetGroupInfoV5";
		case 149:
		    return "mgroup-PGSearchGroupV5";
		case 136:
		    return "mgroup-PGApplyGroupV5";
		case 137:
			return "mgroup-PGCancelApplicationV5";
		case 148:
		   	return"mgroup-PGQuitGroupV5";
		case 147:
		   	return"mgroup-PGInviteJoinGroupV5";
		case 146:
		   	return"mgroup-PGHandleInviteJoinGroupV5";
		case 143:
		   	return"mgroup-PGGetGroupMembersV5";
		case 150:
		   	return"mgroup-PGUpdatePersonalInfoV5";
		case 144:
		   	return"mgroup-PGGetPersonalInfoV5";
		case 145:
		   	return"mgroup-PGHandleApplicationV5";
		case 138:
		   	return"mgroup-PGCreateGroupV5";
		case 139:
		   	return"mgroup-PGDeleteGroupV5";
		case 140:
		   	return"mgroup-PGDeleteGroupMemberV5";
		case 135:
		   	return"mgroup-PGGroupRegV5";
		case 132:
		   	return"mgroup-PGUACMsgV5";
		case 155:
		   	return"mgroup-PGKeepAlive";
		case 156:
		   	return"mgroup-PGPresence";
		case 181:
		   	return"mgroup-SubPGPresenceV5";
		case 182:
		   	return"mgroup-PGSetPresenceV5";
		case 183:
		   	return"mgroup-PGApproveInviteJoinV5";
		case 185:
		   	return"mgroup-PGSendSmsV5";
		case 151:
		   	return"mmessage-SendOfflineV5";
		case 152:
		   	return"mmessage-SendGroupSMSV5";
		case 153:
		   	return"mmessage-SendSMSV5";
		case 154:
		   	return"mmessage-ShareContentV5";
		case 191:
		   	return"mmultimedia-CreateMMConversationV5";
		case 192:
		   	return"mmultimedia-FinishMMDownload";
		case 193:
		   	return"mmultimedia-RequireMMData";
		case 194:
		   	return"mmultimedia-SendMMDataV5McpApp";
		case 195:
		   	return"mmultimedia-SendMMDataFinishV5";
		default:
			return NULL;
	}
	return NULL;
}
