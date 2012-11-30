#ifndef __MRPC_H
#define __MRPC_H

#include <string.h>
#include "msp_pb.h"
#include "proxy.h"

#define MRPC_BUF_SIZE 4096
#define UP_CONN_COUNT 3
#define MRPC_CONN_DISCONNECTED 0
#define MRPC_CONN_CONNECTING 1
#define MRPC_CONN_CONNECTED 2
#define MRPC_CONN_FROZEN 3
#define MRPC_CONN_DURATION 600
#define MRPC_TX_TO 180
#define MRPC_MAX_PENDING 100
#define MRPC_CONNECT_TO 5


typedef struct mrpc_upstreamer_s {
	int listen_fd;
	list_head_t conn_list; //connecting list
	size_t conn_count;
}mrpc_upstreamer_t;

typedef struct mrpc_us_item_s {
	char *uri;
	struct rb_node rbnode;
	list_head_t conn_list; //connection list 
	list_head_t frozen_list; //frozen connection list
	int current_conn;
        size_t conn_count;
	list_head_t pending_list; // pending request list 
	size_t pend_count;
}mrpc_us_item_t;

typedef struct mrpc_message_s {
	uint32_t mask;
	uint32_t package_length;
	short header_length;
	short packet_options;
	union {
		mrpc_request_header req_head;
		mrpc_response_header resp_head;
	}h;
	short is_resp;
	struct 	pbc_slice body;
}mrpc_message_t;

typedef struct mrpc_buf_s {
	char *buf;
	size_t len;  //buf len
	size_t size; //data size
	size_t offset;
}mrpc_buf_t;

typedef struct mrpc_connection_s {
	int fd;
	mrpc_buf_t *send_buf;
	mrpc_buf_t *recv_buf;
	mrpc_message_t req;
	time_t connecting;
	time_t connected;
	uint32_t seq;
	struct rb_root root;
	list_head_t list_us;
	list_head_t list_to;
	int conn_status;
	mrpc_us_item_t * us;
	ev_file_item_t *event;
}mrpc_connection_t;

typedef struct mrpc_stash_req_s {
	struct rb_node rbnode;
	uint32_t seq;
	int32_t user_id;
	char *epid;
	uint32_t mcp_seq;
} mrpc_stash_req_t;

//TODO load from the config
static inline char* 
get_cmd_func_name(int cmd)
{
	switch(cmd) {
	case 101: return "mcore-Reg1V5";
	case 102: return "mcore-Reg2V5";
	case 103: return "mcore-UnRegV5";
	case 104: return "mcore-KeepAliveV5";
	case 105: return "mcore-FlowV5";
	case 198: return "mcore-Reg3V5";
	case 199: return "mcore-OffStateV5";

	case 165: return "muser-HandleContactRequestV5";
	case 161: return "muser-AddToBlacklistV5";
	case 162: return "muser-RemoveFromBlacklistV5";
	case 163: return "muser-AddBuddyV5";
	case 164: return "muser-DeleteBuddyV5";
	case 166: return "muser-CreateBuddyListV5";
	case 167: return "muser-DeleteBuddyListV5";
	case 168: return "muser-SetBuddyListInfoV5";
	case 169: return "muser-AddChatFriendV5";
	case 170: return "muser-DeleteChatFriendV5";
	case 171: return "muser-GetContactInfoV5";
	case 172: return "muser-InviteToDownloadMCV5";    //
	case 173: return "muser-InviteUserV5";
	case 174: return "muser-PermissionRequestV5";
	case 175: return "muser-PermissionResponseV5";
	case 176: return "muser-PresenceV5";
	case 178: return "muser-SetPresenceV5";
	case 180: return "muser-SmsForwardInfoV5";      //
	case 179: return "muser-SetUserInfoV5";
	case 177: return "muser-SetContactInfoV5";
	case 186: return "muser-GetContactsInfoV5";
	case 187: return "muser-SubContactInfoV5";

		//添加群组处理
	case 142: return "mgroup-PGGetGroupListV5";
	case 141: return "mgroup-PGGetGroupInfoV5";
	case 149: return "mgroup-PGSearchGroupV5";
	case 136: return "mgroup-PGApplyGroupV5";
	case 137: return "mgroup-PGCancelApplicationV5";
	case 148: return "mgroup-PGQuitGroupV5";
	case 147: return "mgroup-PGInviteJoinGroupV5";
	case 146: return "mgroup-PGHandleInviteJoinGroupV5";
	case 143: return "mgroup-PGGetGroupMembersV5";
	case 150: return "mgroup-PGUpdatePersonalInfoV5";
	case 144: return "mgroup-PGGetPersonalInfoV5";
	case 145: return "mgroup-PGHandleApplicationV5";
	case 138: return "mgroup-PGCreateGroupV5";
	case 139: return "mgroup-PGDeleteGroupV5";
	case 140: return "mgroup-PGDeleteGroupMemberV5";
	case 135: return "mgroup-PGGroupRegV5";
	case 132: return "mgroup-PGUACMsgV5";
	case 155: return "mgroup-PGKeepAlive";
	case 156: return "mgroup-PGPresence";

	case 181: return "mgroup-SubPGPresenceV5";
	case 182: return "mgroup-PGSetPresenceV5";
	case 183: return "mgroup-PGApproveInviteJoinV5";
	case 185: return "mgroup-PGSendSmsV5";

		// 讨论组
	case 200: return "mgroup-DGCreateGroupV5";
	case 201: return "mgroup-DGSessionInitializeV5";
	case 202: return "mgroup-DGInviteJoinGroupV5";
	case 205: return "mgroup-DGSendMessageV5";
	case 207: return "mgroup-DGQuitGroupV5";

	case 151: return "mmessage-SendOfflineV5";
	case 152: return "mmessage-SendGroupSMSV5";
	case 153: return "mmessage-SendSMSV5";
	case 154: return "mmessage-ShareContentV5";

		//case 191: return "mmultimedia-CreateMMConversationV5";
		//case 192: return "mmultimedia-FinishMMDownload";
		//case 193: return "mmultimedia-RequireMMData";
		//case 194: return "mmultimedia-SendMMDataV5McpApp";
		//case 195: return "mmultimedia-SendMMDataFinishV5";
	case 196: return "mmultimedia-mmsendimage";
	case 197: return "mmultimedia-MMSendImageNotif";


		// 二期信息
	case 10101: return "mcore-Reg1V501";
	case 10102: return "mcore-Reg2V501";
	case 10103: return "mcore-KeepAliveV501";
	case 10104: return "mcore-Reg3V501";
	case 10105: return "mcore-UnRegV501";
	case 10106: return "mcore-FlowV501";
	case 10107: return "mcore-OffStateV501";

		//muser            
	case 20101: return "muser-AddToBlackListV501";
	case 20102: return "muser-RemoveFromBlacklistV501";
	case 20103: return "muser-AddBuddyV501";
	case 20104: return "muser-DeleteBuddyV501";
	case 20105: return "muser-HandleContactRequestV501";
	case 20106: return "muser-CreateBuddyListV5";
	case 20107: return "muser-DeleteBuddyListV5";
	case 20116: return "muser-PresenceV501";
	case 20108: return "muser-SetBuddyListInfoV501";
	case 20109: return "muser-AddChatFriendV501";
	case 20110: return "muser-DeleteChatFriendV501";
	case 20111: return "muser-GetContactInfoV501";
	case 20112: return "muser-InviteToDownloadMCV501";    //
	case 20113: return "muser-InviteUserV501";
	case 20114: return "muser-PermissionRequestV501";
	case 20115: return "muser-PermissionResponseV501";
	case 20118: return "muser-SetPresenceV501";
	case 20120: return "muser-SmsForwardInfoV501";      //
	case 20119: return "muser-SetUserInfoV501";
	case 20117: return "muser-SetContactInfoV501";
	case 20121: return "muser-SubContactInfoV501";

		// Mmessage
	case 30101: return "mmessage-SendOfflineV501";
	case 30102: return "mmessage-SendGroupSMSV501";
	case 30103: return "mmessage-SendSMSV501";
	case 30104: return "mmessage-ShareContentV501";

		// mmultimedia发图片
	case 50106: return "mmultimedia-mmsendimage01";
	case 50107: return "mmultimedia-MMSendImageNotif01";
	case 50101: return "mmultimedia-CreateMMConversationV5";
	case 50102: return "mmultimedia-FinishMMDownload";
	case 50103: return "mmultimedia-RequireMMData";
	case 50104: return "mmultimedia-SendMMDataV5McpApp";
	case 50105: return "mmultimedia-SendMMDataFinishV5";


		// 01版本讨论组
	case 40101: return "mgroup-DGCreateGroupV501";
	case 40102: return "mgroup-DGInviteJoinGroupV501";
	case 40103: return "mgroup-DGQuitGroupV501";
	case 40104: return "mgroup-DGSendMessageV501";
	case 40105: return "mgroup-DGSessionInitializeV501";
	case 40106: return "mgroup-DGGetGroupListV501";
	case 40107: return "mgroup-DGGetGroupMembersV501";
	case 40108: return "mgroup-DGGetGroupInfoV501";
	case 40109: return "mgroup-DGUpdateGroupInfoV501";
	case 40110: return "mgroup-DGGetPersonalInfoV501";
	case 40111: return "mgroup-DGUpdatePersonalInfoV501";
	case 40112: return "mgroup-DGGroupRegV501";

		// 01版本群组
	case 40121: return "mgroup-PGGetGroupInfoV501";
	case 40122: return "mgroup-PGGetGroupListV501";
	case 40123: return "mgroup-PGSearchGroupV501";
	case 40141: return "mgroup-PGApplyGroupV501";
	case 40142: return "mgroup-PGApproveInviteJoinV501";
	case 40143: return "mgroup-PGCancelApplicationV501";
	case 40144: return "mgroup-PGGetGroupMembersV501";
	case 40145: return "mgroup-PGGetPersonalInfoV501";
	case 40146: return "mgroup-PGHandleApplicationV501";
	case 40147: return "mgroup-PGHandleInviteJoinGroupV501";
	case 40148: return "mgroup-PGInviteJoinGroupV501";
	case 40149: return "mgroup-PGQuitGroupV501";
	case 40150: return "mgroup-PGUpdatePersonalInfoV501";
	case 40161: return "mgroup-PGSendMessageV501";
	case 40162: return "mgroup-PGSendSMSV501";
	case 40181: return "mgroup-PGGroupRegV501";
	case 40182: return "mgroup-PGKeepAliveV501";
	case 40183: return "mgroup-PGSetPresenceV501";
	case 40184: return "mgroup-PGSubscribeV501";

	default: return NULL;
	}
}

int mrpc_init();
int mrpc_start();
int mrpc_us_send(rec_msg_t *);
void mrpc_us_send_err(rec_msg_t *);
void mrpc_ev_after();
int mrpc_svr_accept(ev_t *, ev_file_item_t*);

mrpc_connection_t* mrpc_conn_new(mrpc_us_item_t *us);
void mrpc_conn_close(mrpc_connection_t *c);
void mrpc_conn_free(mrpc_connection_t *c);
int mrpc_send(mrpc_connection_t *c);
int mrpc_send2(mrpc_buf_t *b, int fd);
int mrpc_recv(mrpc_connection_t *c);
int mrpc_recv2(mrpc_buf_t *b, int fd);
//1 OK, 0 not finish, -1 error
int 
mrpc_parse(mrpc_buf_t *b, mrpc_message_t *msg, mcp_appbean_proto *proto);
mrpc_buf_t* mrpc_buf_new(size_t size);
int mrpc_buf_enlarge(mrpc_buf_t *buf);
void mrpc_buf_reset(mrpc_buf_t *buf);
void mrpc_buf_free(mrpc_buf_t *buf) ;



#endif
