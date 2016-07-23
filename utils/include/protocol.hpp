#ifndef PROTOCOL_UTILS_HPP
#define PROTOCOL_UTILS_HPP

#include <QString>
#include <QStringList>
#include <iostream>

#include "definitions.hpp"

namespace tcp_messenger {

  class Protocol {
  public:
    Protocol(const unsigned int version);
    ~Protocol();
    //outgoing streams
    QString constructStream_UE(QStringList params,
			       tcp_messenger::ProtocolStreamType_UE stream_type);
    QString constructStream_Server(QStringList params,
				   tcp_messenger::ProtocolStreamType_Server stream_type);
    //incoming streams
    QStringList parseStream_UE(ProtocolStreamType_UE& type, QString stream);
    QStringList parseStream_Server(ProtocolStreamType_Server& type, QString stream);
    
    QString print(ProtocolStreamType_UE type);
    QString printType(ProtocolStreamType_Server type);
  private:
    QString getUERegisterStream(UE_Register stream);
    QString getUEOutgoingMessageStream(UE_OutgoingMessage stream);
    QString getUEOutgoingAckStream(UE_OutgoingAck stream);
    QString getUETypingAlertStream(UE_TypingAlert stream);

    QString getServerOutgoingAckStream(Server_OutgoingAck stream);
    QString getServerFwdTypingStream(Server_FwdTyping stream);

    QStringList getUEStreamFields(QString stream);
    QStringList getServerStreamFields(QString stream);

    //Local operations on streams
    QStringList extractFields(QStringList before);
  private:
    unsigned int m_version;
  };  
}

#endif
