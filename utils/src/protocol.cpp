#include <glog/logging.h>
#include "protocol.hpp"

namespace tcp_messenger {

  Protocol::Protocol(const unsigned int version) : m_version(version) {}
  Protocol::~Protocol() {}

  QString Protocol::constructStream_UE(QStringList params,
				       tcp_messenger::ProtocolStreamType_UE stream_type) {
    switch (stream_type) {
    case UE_REGISTER:
      {
	UE_Register out;
	out.username = params.at(0);
	out.rx_port = (quint16) params.at(1).toInt();
	return getUERegisterStream(out);
      }
    case UE_MESSAGE:
      {
	UE_OutgoingMessage out;
	out.message = params.at(0);
	out.dest = params.at(1);
	out.from = params.at(2);
	out.message_id = (unsigned int) params.at(3).toInt();
	return getUEOutgoingMessageStream(out);
      }
    case UE_UNREGISTER:
      return (UE_UNREGISTER_DISCONNECT + OPEN + CLOSE + SEMICOLON);
    case UE_ACK:
      {
	UE_OutgoingAck out;
	out.dest = params.at(0);
	out.from = params.at(1);
	out.message_id = (unsigned int) params.at(2).toInt();
	return getUEOutgoingAckStream(out);
      }
    case UE_ERROR:
      return (UE_ERROR_HEADER + OPEN + CLOSE + SEMICOLON);
    case UE_TYPING:
      UE_TypingAlert out;
      {
	out.dest = params.at(0);
	out.from = params.at(1);
	out.status = (bool) params.at(2).toInt();
	return getUETypingAlertStream(out);
      }
    }
    return QString();
  }

  QString Protocol::constructStream_Server(QStringList params,
					   tcp_messenger::ProtocolStreamType_Server stream_type) {
    switch (stream_type) {
    case SERVER_ALL:
      {
	return (SERVER_ALL_HEADER + OPEN + params.join("-") + CLOSE + SEMICOLON);
      }
    case SERVER_ACK:
      {
	Server_OutgoingAck out;
	out.dest = params.at(0);
	out.message_id = (unsigned int) params.at(1).toInt();
	return getServerOutgoingAckStream(out);
      }
    case SERVER_ERROR:
      return (SERVER_ERROR_HEADER + OPEN + CLOSE + SEMICOLON);
    case SERVER_FWD_TO_SENDER:
      {
	DLOG (INFO) << "Forwarding stream to user, no need to create new";
	return params.at(0);
      }
    case SERVER_FWD_TO_DEST:
      {
	DLOG (INFO) << "Forwarding stream to user, no need to create new";
	return params.at(0);
      }
    default:
      DLOG (INFO) << "Invalid stream type, returning empty";
      return QString();
    }
  }

  QString Protocol::getUERegisterStream(UE_Register stream) {
    return (UE_REGISTER_NAME_PREFIX + OPEN +
	    stream.username +
	    CLOSE + SEMICOLON +
	    UE_REGISTER_RX_PORT + OPEN +
	    QString::number(stream.rx_port) +
	    CLOSE + SEMICOLON);
  }

  QString Protocol::getUEOutgoingMessageStream(UE_OutgoingMessage stream) {
    return (UE_MESSAGE_TEXT + OPEN +
	    stream.message +
	    CLOSE + SEMICOLON +
	    UE_MESSAGE_DEST + OPEN +
	    stream.dest +
	    CLOSE + SEMICOLON +
	    UE_MESSAGE_FROM + OPEN +
	    stream.from +
	    CLOSE + SEMICOLON +
	    UE_MESSAGE_ID + OPEN +
	    QString::number(stream.message_id) +
	    CLOSE + SEMICOLON);
  }

  QString Protocol::getUEOutgoingAckStream(UE_OutgoingAck stream) {
    return (UE_ACK_DEST + OPEN +
	    stream.dest +
	    CLOSE + SEMICOLON +
	    UE_ACK_FROM + OPEN +
	    stream.from +
	    CLOSE + SEMICOLON +
	    UE_ACK_ID + OPEN +
	    QString::number(stream.message_id) +
	    CLOSE + SEMICOLON);
  }
  
  QString Protocol::getUETypingAlertStream(UE_TypingAlert stream) {
    return (UE_TYPING_DEST + OPEN +
	    stream.dest +
	    CLOSE + SEMICOLON +
	    UE_TYPING_FROM + OPEN +
	    stream.from +
	    CLOSE + SEMICOLON +
	    UE_TYPING_STATUS + OPEN +
	    QString::number(stream.status) +
	    CLOSE + SEMICOLON);
  }
  
  QString Protocol::getServerOutgoingAckStream(Server_OutgoingAck stream) {
    return (SERVER_ACK_DEST + OPEN +
	    stream.dest +
	    CLOSE + SEMICOLON +
	    SERVER_ACK_ID + OPEN +
	    QString::number(stream.message_id) +
	    CLOSE + SEMICOLON);
  }

  QStringList Protocol::parseStream_UE(ProtocolStreamType_UE& type, QString stream) {
    //first, determine which type of stream it is
    if (stream.contains(UE_MESSAGE_TEXT)) {
      type = UE_MESSAGE;
    } else if (stream.contains(UE_REGISTER_NAME_PREFIX)) {
      type = UE_REGISTER;
    } else if (stream.contains(UE_UNREGISTER_DISCONNECT)) {
      type = UE_UNREGISTER;
    } else if (stream.contains(UE_TYPING_STATUS)) {
      type = UE_TYPING;
    } else if (stream.contains(UE_ACK_ID)) {
      type = UE_ACK;
    } else {
      type = UE_ERROR;
    }
    DLOG (INFO) << "Incoming stream from user, of type " << print(type).toStdString();
    return getUEStreamFields(stream);
  }

  QStringList Protocol::parseStream_Server(ProtocolStreamType_Server& type, QString stream) {
    if (stream.contains(SERVER_ALL_HEADER)) {
      type = SERVER_ALL;
    } else if (stream.contains(SERVER_ACK_ID)) {
      type = SERVER_ACK;
    } else if (stream.contains(SERVER_ERROR_HEADER)) {
      type = SERVER_ERROR;
    } else if (stream.contains(UE_MESSAGE_TEXT)) {
      type = SERVER_FWD_TO_SENDER;
    } else if (stream.contains(UE_ACK_ID)) {
      type = SERVER_FWD_TO_DEST;
    } else {
      LOG (WARNING) << "Unrecognized input stream.";
      return QStringList();
    }
    DLOG (INFO) << "Incoming stream from server, of type: " << printType(type).toStdString();
    return getServerStreamFields(stream);
  }

  QString Protocol::print(ProtocolStreamType_UE type) {
    switch(type) {
    case UE_MESSAGE:
      return "UE_MESSAGE";
    case UE_REGISTER:
      return "UE_REGISTER";
    case UE_ACK:
      return "UE_ACK";
    case UE_UNREGISTER:
      return "UE_UNREGISTER";
    case UE_ERROR:
      return "UE_ERROR";
    case UE_TYPING:
      return "UE_TYPING";
    default:
      return "Unrecognized incoming stream";
    }
  }

  QString Protocol::printType(ProtocolStreamType_Server type) {
    switch(type) {
    case SERVER_ALL:
      return "SERVER_ALL";
    case SERVER_ACK:
      return "SERVER_ACK";
    case SERVER_ERROR:
      return "SERVER_ERROR";
    case SERVER_FWD_TO_DEST:
      return "SERVER_FWD_TO_DEST";
    default:
      return "SERVER_FWD_TO_SENDER";
    }
  }

  QStringList Protocol::getUEStreamFields(QString stream) {
    return extractFields(stream.split(SEMICOLON));
  }

  QStringList Protocol::getServerStreamFields(QString stream) {
    return extractFields(stream.split(SEMICOLON));
  }

  QStringList Protocol::extractFields(QStringList before) {
    QStringList after;
    for (unsigned i = 0; i < before.size() - 1; ++i) {
      QString temp = before.at(i);
      after << temp.mid(temp.indexOf(OPEN) + 1, temp.lastIndexOf(CLOSE) - temp.indexOf(OPEN) - 1);
      DLOG (INFO) << "Obtained: " << after.at(i).toStdString();
    }
    return after;
  }
}
