# BOTS_SERVER for python

import zmq
import src.client_server_pb2 as proto

def zmq_send_HeaderMessage(requester: zmq.Socket, client_type: proto.ClientType, msg_type: proto.MessageType):
    m = proto.Header_Message()
    m.client_type = client_type
    m.msg_type= msg_type

    try:
        requester.send(m.SerializeToString(), flags=zmq.SNDMORE)
    except Exception as e:
        raise Exception(f"zmq_send_HeaderMessage: {e}")

def zmq_send_WaspConnectReq(requester: zmq.Socket, num_wasps: int):
    m = proto.Wasp_Connect_Req()
    m.num_wasps = num_wasps

    try:
        zmq_send_HeaderMessage(requester, proto.WASP, proto.CONNECT)
        requester.send(m.SerializeToString())
    except Exception as e:
        raise Exception(f"zmq_send_WaspConnectReq: {e}")

def zmq_recv_BotConnectResp(requester: zmq.Socket):
    msg_raw = zmq.Message()
    msg_raw = requester.recv(flags=0)

    ret_value = proto.Bot_Connect_Resp()
    try:
        ret_value.ParseFromString(msg_raw)
        return ret_value
    except Exception as e:
        raise Exception(f"zmq_recv_BotConnectResp: {e}")

def zmq_send_WaspMovementReq(requester: zmq.Socket, bot_id: int, token: int, direction: proto.Direction, client_id: int):
    m = proto.Bot_Movement_Req()
    m.bot_id = bot_id
    m.token = token
    m.direction = direction
    m.client_id = client_id

    try:
        zmq_send_HeaderMessage(requester, proto.WASP, proto.MOVEMENT)
        requester.send(m.SerializeToString())
    except Exception as e:
        raise Exception(f"zmq_send_WaspMovementReq: {e}")

def zmq_recv_BotMovementResp(requester: zmq.Socket):
    msg_raw = zmq.Message()
    msg_raw = requester.recv(flags=0)

    ret_value = proto.Bot_Movement_Resp()
    try:
        ret_value.ParseFromString(msg_raw)
        return ret_value
    except Exception as e:
        raise Exception(f"zmq_recv_BotMovementResp: {e}")

def zmq_send_WaspDisconnectReq(requester: zmq.Socket, client_id: int, token: int):
    m = proto.Bot_Disconnect_Req()
    m.client_id = client_id
    m.token = token

    try:
        zmq_send_HeaderMessage(requester, proto.WASP, proto.DISCONNECT)
        requester.send(m.SerializeToString())
    except Exception as e:
        raise Exception(f"zmq_send_WaspDisconnectReq: {e}")

def zmq_recv_BotDisconnectResp(requester: zmq.Socket):
    msg_raw = zmq.Message()
    msg_raw = requester.recv(flags=0)

    ret_value = proto.Bot_Disconnect_Resp()
    try:
        ret_value.ParseFromString(msg_raw)
        return ret_value
    except Exception as e:
        raise Exception(f"zmq_recv_BotDisconnectResp: {e}")
