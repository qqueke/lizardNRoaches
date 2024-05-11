#ifndef CLIENT_SERVER_H
#define CLIENT_SERVER_H

#include "client_server.pb-c.h"

int zmq_send_HeaderMessage(void *requester, ClientType client_type, MessageType msg_type);
HeaderMessage *zmq_recv_HeaderMessage(void *responder);

/**
 * BOTS
*/

int zmq_send_RoachConnectReq(void *requester, const int *roach_digit, size_t size);
RoachConnectReq *zmq_recv_RoachConnectReq(void *responder);

int zmq_send_WaspConnectReq(void *requester, int wasp_num);
WaspConnectReq *zmq_recv_WaspConnectReq(void *responder);

int zmq_send_BotConnectResp(void *responder, const int *bot_id, size_t size, int token, int client_id);
BotConnectResp *zmq_recv_BotConnectResp(void *requester);

int zmq_send_BotMovementReq(void *requester, int bot_id, int token, Direction direction, int client_id);
BotMovementReq *zmq_recv_BotMovementReq(void *responder);

int zmq_send_BotMovementResp(void *responder, Response resp);
BotMovementResp *zmq_recv_BotMovementResp(void *requester);

int zmq_send_BotDisconnectReq(void *requester, int client_id, int token);
BotDisconnectReq *zmq_recv_BotDisconnectReq(void *responder);

int zmq_send_BotDisconnectResp(void *responder, Response resp);
BotDisconnectResp *zmq_recv_BotDisconnectResp(void *requester);

/**
 * LIZARDS
*/

int zmq_send_LizardConnectReq(void *requester);
LizardConnectReq *zmq_recv_LizardConnectReq(void *responder);

int zmq_send_LizardConnectResp(void *responder, char ch, int token);
LizardConnectResp *zmq_recv_LizardConnectResp(void *requester);

int zmq_send_LizardMovementReq(void *requester, char ch, int token, Direction direction);
LizardMovementReq *zmq_recv_LizardMovementReq(void *responder);

int zmq_send_LizardMovementResp(void *responder, Response resp);
LizardMovementResp *zmq_recv_LizardMovementResp(void *requester);

int zmq_send_LizardDisconnectReq(void *requester, char ch, int token);
LizardDisconnectReq *zmq_recv_LizardDisconnectReq(void *responder);

int zmq_send_LizardDisconnectResp(void *responder, int score);
LizardDisconnectResp *zmq_recv_LizardDisconnectResp(void *requester);

#endif