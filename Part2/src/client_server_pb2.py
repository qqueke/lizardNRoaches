# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: client_server.proto
"""Generated protocol buffer code."""
from google.protobuf.internal import builder as _builder
from google.protobuf import descriptor as _descriptor
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(b'\n\x13\x63lient_server.proto\"R\n\x0eHeader_Message\x12 \n\x0b\x63lient_type\x18\x01 \x02(\x0e\x32\x0b.ClientType\x12\x1e\n\x08msg_type\x18\x02 \x02(\x0e\x32\x0c.MessageType\"A\n\x15Proto_Display_Message\x12\n\n\x02\x63h\x18\x01 \x02(\r\x12\r\n\x05pos_x\x18\x02 \x02(\x03\x12\r\n\x05pos_y\x18\x03 \x02(\x03\"(\n\x11Roach_Connect_Req\x12\x13\n\x0broach_digit\x18\x01 \x03(\r\"%\n\x10Wasp_Connect_Req\x12\x11\n\tnum_wasps\x18\x01 \x02(\r\"D\n\x10\x42ot_Connect_Resp\x12\x0e\n\x06\x62ot_id\x18\x01 \x03(\r\x12\r\n\x05token\x18\x02 \x02(\x04\x12\x11\n\tclient_id\x18\x03 \x02(\r\"c\n\x10\x42ot_Movement_Req\x12\x0e\n\x06\x62ot_id\x18\x01 \x02(\r\x12\r\n\x05token\x18\x02 \x02(\x04\x12\x1d\n\tdirection\x18\x03 \x02(\x0e\x32\n.Direction\x12\x11\n\tclient_id\x18\x04 \x02(\r\",\n\x11\x42ot_Movement_Resp\x12\x17\n\x04resp\x18\x01 \x02(\x0e\x32\t.Response\"6\n\x12\x42ot_Disconnect_Req\x12\x11\n\tclient_id\x18\x01 \x02(\r\x12\r\n\x05token\x18\x02 \x02(\x04\".\n\x13\x42ot_Disconnect_Resp\x12\x17\n\x04resp\x18\x01 \x02(\x0e\x32\t.Response\"\"\n\x12Lizard_Connect_Req\x12\x0c\n\x04null\x18\x01 \x01(\x0c\"0\n\x13Lizard_Connect_Resp\x12\n\n\x02\x63h\x18\x01 \x02(\x0c\x12\r\n\x05token\x18\x02 \x02(\x04\"O\n\x13Lizard_Movement_Req\x12\n\n\x02\x63h\x18\x01 \x02(\x0c\x12\r\n\x05token\x18\x02 \x02(\x04\x12\x1d\n\tdirection\x18\x03 \x02(\x0e\x32\n.Direction\"/\n\x14Lizard_Movement_Resp\x12\x17\n\x04resp\x18\x01 \x02(\x0e\x32\t.Response\"2\n\x15Lizard_Disconnect_Req\x12\n\n\x02\x63h\x18\x01 \x02(\x0c\x12\r\n\x05token\x18\x02 \x02(\x04\"\'\n\x16Lizard_Disconnect_Resp\x12\r\n\x05score\x18\x01 \x02(\x04*2\n\tDirection\x12\x06\n\x02UP\x10\x00\x12\x08\n\x04\x44OWN\x10\x01\x12\x08\n\x04LEFT\x10\x02\x12\t\n\x05RIGHT\x10\x03*E\n\x0bMessageType\x12\x0b\n\x07\x43ONNECT\x10\x00\x12\x0c\n\x08MOVEMENT\x10\x01\x12\x0e\n\nDISCONNECT\x10\x02\x12\x0b\n\x07REQUEST\x10\x03*-\n\nClientType\x12\n\n\x06LIZARD\x10\x00\x12\t\n\x05ROACH\x10\x01\x12\x08\n\x04WASP\x10\x02*>\n\x08Response\x12\x0b\n\x07SUCCESS\x10\x00\x12\x0b\n\x07NOTHING\x10\x01\x12\x0b\n\x07\x46\x41ILURE\x10\x02\x12\x0b\n\x07TIMEOUT\x10\x03')

_builder.BuildMessageAndEnumDescriptors(DESCRIPTOR, globals())
_builder.BuildTopDescriptorsAndMessages(DESCRIPTOR, 'client_server_pb2', globals())
if _descriptor._USE_C_DESCRIPTORS == False:

  DESCRIPTOR._options = None
  _DIRECTION._serialized_start=885
  _DIRECTION._serialized_end=935
  _MESSAGETYPE._serialized_start=937
  _MESSAGETYPE._serialized_end=1006
  _CLIENTTYPE._serialized_start=1008
  _CLIENTTYPE._serialized_end=1053
  _RESPONSE._serialized_start=1055
  _RESPONSE._serialized_end=1117
  _HEADER_MESSAGE._serialized_start=23
  _HEADER_MESSAGE._serialized_end=105
  _PROTO_DISPLAY_MESSAGE._serialized_start=107
  _PROTO_DISPLAY_MESSAGE._serialized_end=172
  _ROACH_CONNECT_REQ._serialized_start=174
  _ROACH_CONNECT_REQ._serialized_end=214
  _WASP_CONNECT_REQ._serialized_start=216
  _WASP_CONNECT_REQ._serialized_end=253
  _BOT_CONNECT_RESP._serialized_start=255
  _BOT_CONNECT_RESP._serialized_end=323
  _BOT_MOVEMENT_REQ._serialized_start=325
  _BOT_MOVEMENT_REQ._serialized_end=424
  _BOT_MOVEMENT_RESP._serialized_start=426
  _BOT_MOVEMENT_RESP._serialized_end=470
  _BOT_DISCONNECT_REQ._serialized_start=472
  _BOT_DISCONNECT_REQ._serialized_end=526
  _BOT_DISCONNECT_RESP._serialized_start=528
  _BOT_DISCONNECT_RESP._serialized_end=574
  _LIZARD_CONNECT_REQ._serialized_start=576
  _LIZARD_CONNECT_REQ._serialized_end=610
  _LIZARD_CONNECT_RESP._serialized_start=612
  _LIZARD_CONNECT_RESP._serialized_end=660
  _LIZARD_MOVEMENT_REQ._serialized_start=662
  _LIZARD_MOVEMENT_REQ._serialized_end=741
  _LIZARD_MOVEMENT_RESP._serialized_start=743
  _LIZARD_MOVEMENT_RESP._serialized_end=790
  _LIZARD_DISCONNECT_REQ._serialized_start=792
  _LIZARD_DISCONNECT_REQ._serialized_end=842
  _LIZARD_DISCONNECT_RESP._serialized_start=844
  _LIZARD_DISCONNECT_RESP._serialized_end=883
# @@protoc_insertion_point(module_scope)
