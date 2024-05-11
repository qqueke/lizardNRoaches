#include "client_server.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>

int zmq_send_HeaderMessage(void *requester, ClientType client_type,
                           MessageType msg_type) {
  HeaderMessage m = HEADER__MESSAGE__INIT;
  m.client_type = client_type;
  m.msg_type = msg_type;

  size_t size_bin_msg = header__message__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  header__message__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(requester, m_bin, size_bin_msg, ZMQ_SNDMORE);

  free(m_bin);

  return bytes;
}

HeaderMessage *zmq_recv_HeaderMessage(void *responder) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(responder, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  HeaderMessage *ret_value =
      header__message__unpack(NULL, n_bytes, (const uint8_t *)msg);
  zmq_msg_close(&msg_raw);
  return ret_value;
}

/**
 * BOTS
 */

int zmq_send_RoachConnectReq(void *requester, const int *roach_digit,
                             size_t size) {
  RoachConnectReq m = ROACH__CONNECT__REQ__INIT;

  // Add numbers to the repeated field
  m.n_roach_digit = size;
  m.roach_digit = malloc(m.n_roach_digit * sizeof(uint32_t));
  for (size_t i = 0; i < size; i++) {
    m.roach_digit[i] = roach_digit[i];
  }

  size_t size_bin_msg = roach__connect__req__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  roach__connect__req__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(requester, m_bin, size_bin_msg, 0);

  free(m_bin);
  free(m.roach_digit);

  return bytes;
}

RoachConnectReq *zmq_recv_RoachConnectReq(void *responder) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(responder, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  RoachConnectReq *ret_value =
      roach__connect__req__unpack(NULL, n_bytes, (uint8_t *)msg);
  zmq_msg_close(&msg_raw);
  return ret_value;
}

int zmq_send_WaspConnectReq(void *requester, int num_wasps) {
  WaspConnectReq m = WASP__CONNECT__REQ__INIT;
  m.num_wasps = num_wasps;

  size_t size_bin_msg = wasp__connect__req__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  wasp__connect__req__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(requester, m_bin, size_bin_msg, 0);

  free(m_bin);

  return bytes;
}

WaspConnectReq *zmq_recv_WaspConnectReq(void *responder) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(responder, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  WaspConnectReq *ret_value =
      wasp__connect__req__unpack(NULL, n_bytes, (uint8_t *)msg);
  zmq_msg_close(&msg_raw);
  return ret_value;
}

int zmq_send_BotConnectResp(void *responder, const int *bot_id, size_t size,
                            int token, int client_id) {
  BotConnectResp m = BOT__CONNECT__RESP__INIT;
  // Add numbers to the repeated field
  m.n_bot_id = size;
  m.bot_id = malloc(m.n_bot_id * sizeof(uint32_t));
  for (size_t i = 0; i < size; i++) {
    m.bot_id[i] = bot_id[i];
  }
  m.token = token;
  m.client_id = client_id;

  size_t size_bin_msg = bot__connect__resp__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  bot__connect__resp__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(responder, m_bin, size_bin_msg, 0);

  free(m_bin);
  free(m.bot_id);

  return bytes;
}

BotConnectResp *zmq_recv_BotConnectResp(void *requester) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(requester, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  BotConnectResp *ret_value =
      bot__connect__resp__unpack(NULL, n_bytes, (uint8_t *)msg);
  zmq_msg_close(&msg_raw);
  return ret_value;
}

int zmq_send_BotMovementReq(void *requester, int bot_id, int token,
                            Direction direction, int client_id) {
  BotMovementReq m = BOT__MOVEMENT__REQ__INIT;
  m.bot_id = bot_id;
  m.token = token;
  m.direction = direction;
  m.client_id = client_id;

  size_t size_bin_msg = bot__movement__req__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  bot__movement__req__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(requester, m_bin, size_bin_msg, 0);

  free(m_bin);

  return bytes;
}

BotMovementReq *zmq_recv_BotMovementReq(void *responder) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(responder, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  BotMovementReq *ret_value =
      bot__movement__req__unpack(NULL, n_bytes, (uint8_t *)msg);
  zmq_msg_close(&msg_raw);
  return ret_value;
}

int zmq_send_BotMovementResp(void *responder, Response resp) {
  BotMovementResp m = BOT__MOVEMENT__RESP__INIT;
  m.resp = resp;

  size_t size_bin_msg = bot__movement__resp__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  bot__movement__resp__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(responder, m_bin, size_bin_msg, 0);

  free(m_bin);

  return bytes;
}

BotMovementResp *zmq_recv_BotMovementResp(void *requester) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(requester, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  BotMovementResp *ret_value =
      bot__movement__resp__unpack(NULL, n_bytes, (uint8_t *)msg);
  zmq_msg_close(&msg_raw);
  return ret_value;
}

int zmq_send_BotDisconnectReq(void *requester, int client_id, int token) {
  BotDisconnectReq m = BOT__DISCONNECT__REQ__INIT;
  m.client_id = client_id;
  m.token = token;

  size_t size_bin_msg = bot__disconnect__req__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  bot__disconnect__req__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(requester, m_bin, size_bin_msg, 0);

  free(m_bin);

  return bytes;
}

BotDisconnectReq *zmq_recv_BotDisconnectReq(void *responder) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(responder, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  BotDisconnectReq *ret_value =
      bot__disconnect__req__unpack(NULL, n_bytes, (uint8_t *)msg);

  zmq_msg_close(&msg_raw);
  return ret_value;
}

int zmq_send_BotDisconnectResp(void *responder, Response resp) {
  BotDisconnectResp m = BOT__DISCONNECT__RESP__INIT;
  m.resp = resp;

  size_t size_bin_msg = bot__disconnect__resp__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  bot__disconnect__resp__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(responder, m_bin, size_bin_msg, 0);

  free(m_bin);

  return bytes;
}

BotDisconnectResp *zmq_recv_BotDisconnectResp(void *requester) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(requester, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  BotDisconnectResp *ret_value =
      bot__disconnect__resp__unpack(NULL, n_bytes, (uint8_t *)msg);
  zmq_msg_close(&msg_raw);
  return ret_value;
}

/**
 * LIZARDS
 */

int zmq_send_LizardConnectReq(void *requester) {
  LizardConnectReq m = LIZARD__CONNECT__REQ__INIT;
  m.has_null = 0;

  size_t size_bin_msg = lizard__connect__req__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  lizard__connect__req__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(requester, m_bin, size_bin_msg, 0);

  free(m_bin);

  return bytes;
}

LizardConnectReq *zmq_recv_LizardConnectReq(void *responder) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(responder, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  LizardConnectReq *ret_value =
      lizard__connect__req__unpack(NULL, n_bytes, (const uint8_t *)msg);
  zmq_msg_close(&msg_raw);
  return ret_value;
}

int zmq_send_LizardConnectResp(void *responder, char ch, int token) {
  LizardConnectResp m = LIZARD__CONNECT__RESP__INIT;
  m.ch.data = malloc(sizeof(ch));
  memcpy(m.ch.data, &ch, sizeof(ch));
  m.ch.len = sizeof(ch);
  m.token = token;

  size_t size_bin_msg = lizard__connect__resp__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  lizard__connect__resp__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(responder, m_bin, size_bin_msg, 0);

  free(m_bin);
  free(m.ch.data);

  return bytes;
}

LizardConnectResp *zmq_recv_LizardConnectResp(void *requester) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(requester, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  LizardConnectResp *ret_value =
      lizard__connect__resp__unpack(NULL, n_bytes, (const uint8_t *)msg);
  zmq_msg_close(&msg_raw);
  return ret_value;
}

int zmq_send_LizardMovementReq(void *requester, char ch, int token,
                               Direction direction) {
  LizardMovementReq m = LIZARD__MOVEMENT__REQ__INIT;
  m.ch.data = malloc(sizeof(ch));
  memcpy(m.ch.data, &ch, sizeof(ch));
  m.ch.len = sizeof(ch);
  m.token = token;
  m.direction = direction;

  size_t size_bin_msg = lizard__movement__req__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  lizard__movement__req__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(requester, m_bin, size_bin_msg, 0);

  free(m_bin);
  free(m.ch.data);

  return bytes;
}

LizardMovementReq *zmq_recv_LizardMovementReq(void *responder) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(responder, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  LizardMovementReq *ret_value =
      lizard__movement__req__unpack(NULL, n_bytes, (const uint8_t *)msg);
  zmq_msg_close(&msg_raw);
  return ret_value;
}

int zmq_send_LizardMovementResp(void *responder, Response resp) {
  LizardMovementResp m = LIZARD__MOVEMENT__RESP__INIT;
  m.resp = resp;

  size_t size_bin_msg = lizard__movement__resp__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  lizard__movement__resp__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(responder, m_bin, size_bin_msg, 0);

  free(m_bin);

  return bytes;
}

LizardMovementResp *zmq_recv_LizardMovementResp(void *requester) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(requester, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  LizardMovementResp *ret_value =
      lizard__movement__resp__unpack(NULL, n_bytes, (const uint8_t *)msg);
  zmq_msg_close(&msg_raw);
  return ret_value;
}

int zmq_send_LizardDisconnectReq(void *requester, char ch, int token) {
  LizardDisconnectReq m = LIZARD__DISCONNECT__REQ__INIT;
  m.ch.data = malloc(sizeof(ch));
  memcpy(m.ch.data, &ch, sizeof(ch));
  m.ch.len = sizeof(ch);
  m.token = token;

  size_t size_bin_msg = lizard__disconnect__req__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  lizard__disconnect__req__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(requester, m_bin, size_bin_msg, 0);

  free(m_bin);
  free(m.ch.data);

  return bytes;
}

LizardDisconnectReq *zmq_recv_LizardDisconnectReq(void *responder) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(responder, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  LizardDisconnectReq *ret_value =
      lizard__disconnect__req__unpack(NULL, n_bytes, (const uint8_t *)msg);

  zmq_msg_close(&msg_raw);
  return ret_value;
}

int zmq_send_LizardDisconnectResp(void *responder, int score) {
  LizardDisconnectResp m = LIZARD__DISCONNECT__RESP__INIT;
  m.score = score;

  size_t size_bin_msg = lizard__disconnect__resp__get_packed_size(&m);
  char *m_bin = malloc(size_bin_msg);
  lizard__disconnect__resp__pack(&m, (uint8_t *)m_bin);

  int bytes = zmq_send(responder, m_bin, size_bin_msg, 0);

  free(m_bin);

  return bytes;
}

LizardDisconnectResp *zmq_recv_LizardDisconnectResp(void *requester) {
  zmq_msg_t msg_raw;
  zmq_msg_init(&msg_raw);
  int n_bytes = zmq_recvmsg(requester, &msg_raw, 0);
  char *msg = zmq_msg_data(&msg_raw);

  LizardDisconnectResp *ret_value =
      lizard__disconnect__resp__unpack(NULL, n_bytes, (const uint8_t *)msg);
  zmq_msg_close(&msg_raw);
  return ret_value;
}
