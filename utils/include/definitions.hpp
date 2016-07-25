#ifndef DEFINITIONS_HPP_
#define DEFINITIONS_HPP_

#include <glog/logging.h>

/*This include file will contain global variables needed by both client and server*/

namespace tcp_messenger {

  static const quint16 LISTEN_PORT = 30500;
  static const int MAX_MESSAGES = 6;
  static const int ENTER = 16777220;
  static const unsigned int PROTOCOL_VERSION = 1.0;

  /************** Protocol specs ****************/

  static const QString OPEN = "(";
  static const QString CLOSE = ")";
  static const QString SEMICOLON = ";";
  
  static const QString UE_REGISTER_NAME_PREFIX = "ue_name";
  static const QString UE_REGISTER_RX_PORT = "ue_rx_port";
  
  static const QString UE_MESSAGE_TEXT = "ue_message";
  static const QString UE_MESSAGE_DEST = "ue_dest";
  static const QString UE_MESSAGE_FROM = "ue_from";
  static const QString UE_MESSAGE_ID = "ue_message_id";

  static const QString UE_UNREGISTER_DISCONNECT = "ue_disconnect";

  static const QString UE_ACK_DEST = "ue_ack_dest";
  static const QString UE_ACK_FROM = "ue_ack_from";
  static const QString UE_ACK_ID = "ue_ack_id";

  static const QString UE_ERROR_HEADER = "ue_error";

  static const QString UE_TYPING_DEST = "ue_typing_dest";
  static const QString UE_TYPING_FROM = "ue_typing_from";
  static const QString UE_TYPING_STATUS = "ue_typing_status";


  static const QString SERVER_ALL_HEADER = "server_all";

  static const QString SERVER_ACK_DEST = "server_ack_dest";
  static const QString SERVER_ACK_ID = "server_ack_id";

  static const QString SERVER_ERROR_HEADER = "server_error";
    
    
  enum ProtocolFieldType_Server {
    SERVER_ERROR_MESSAGE
  };  

  //type of protocol stream to build (CLIENT side): send new message, register, etc
  enum ProtocolStreamType_UE {
    UE_REGISTER,
    UE_MESSAGE,
    UE_UNREGISTER,
    UE_ACK,
    UE_ERROR,
    UE_TYPING
  };

  //type of protocol stream to build (SERVER side): ack resend, etc
  enum ProtocolStreamType_Server {
    SERVER_ALL,
    SERVER_ACK,
    SERVER_ERROR,
    SERVER_FWD_TO_SENDER,
    SERVER_FWD_TO_DEST,
    SERVER_FWD_TYPING
  };
  
  struct UE_Register {
    QString username;
    quint16 rx_port;
  };
  
  struct UE_OutgoingMessage {
    QString message;
    QString dest;
    QString from;
    unsigned int message_id;
  };

  struct UE_OutgoingAck {
    QString dest;
    QString from;
    unsigned int message_id;
  };

  struct UE_TypingAlert {
    QString dest;
    QString from;
    bool status;
  };

  struct Server_FwdTyping {
    QString dest;
    QString from;
    bool status;
  };

  struct Server_OutgoingAck {
    QString dest;
    unsigned int message_id;
  };
}

#endif
