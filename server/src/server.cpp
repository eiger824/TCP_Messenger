#include <QtGui>
#include <QtNetwork>

#include <stdlib.h>
#include <iostream>
#include <glog/logging.h>

#include "server.hpp"

namespace tcp_messenger {

  Server::Server(QWidget *parent)
    :   QDialog(parent), networkSession(0)
  {
    statusLabel = new QLabel;
    quitButton = new QPushButton(tr("Quit"));
    quitButton->setAutoDefault(false);

    //protocol
    m_protocol = new Protocol(PROTOCOL_VERSION);
    
    blockSize = 0;
  
    logLabel = new QTextEdit;
    logLabel->setReadOnly(true);
    logLabel->setStyleSheet("border: 2px solid black; color: black; background-color: white;");
    logLabel->setFixedSize(500,300);
    logLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    onlineUsers = new QTextEdit;
    onlineUsers->setReadOnly(true);
    onlineUsers->setStyleSheet("border: 2px solid black; color: black; background-color: white;");
    onlineUsers->setFixedSize(300,100);
    onlineUsers->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
  
    QNetworkConfigurationManager manager;
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
      // Get saved network configuration
      QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
      settings.beginGroup(QLatin1String("QtNetwork"));
      const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
      settings.endGroup();

      // If the saved network configuration is not currently discovered use the system default
      QNetworkConfiguration config = manager.configurationFromIdentifier(id);
      if ((config.state() & QNetworkConfiguration::Discovered) !=
	  QNetworkConfiguration::Discovered) {
	config = manager.defaultConfiguration();
      }

      networkSession = new QNetworkSession(config, this);
      connect(networkSession, SIGNAL(opened()), this, SLOT(sessionOpened()));

      statusLabel->setText(tr("Opening network session."));
      networkSession->open();
    } else {
      sessionOpened();
    }

    //object connections
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(acceptUser()));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(quitButton);
    buttonLayout->addStretch(1);

    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(logLabel);
    mainLayout->addWidget(new QLabel("Currently online users:"));
    mainLayout->addWidget(onlineUsers);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Fortune Server"));
    this->layout()->setSizeConstraint( QLayout::SetFixedSize );
  }

  void Server::sessionOpened()
  {
    // Save the used configuration
    if (networkSession) {
      QNetworkConfiguration config = networkSession->configuration();
      QString id;
      if (config.type() == QNetworkConfiguration::UserChoice)
	id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
      else
	id = config.identifier();

      QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
      settings.beginGroup(QLatin1String("QtNetwork"));
      settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
      settings.endGroup();
    }

    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, 3422)) {
      QMessageBox::critical(this, tr("Fortune Server"),
			    tr("Unable to start the server: %1.")
			    .arg(tcpServer->errorString()));
      close();
      return;
    }
    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
      if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
	  ipAddressesList.at(i).toIPv4Address()) {
	ipAddress = ipAddressesList.at(i).toString();
	break;
      }
    }
    // if we did not find one, use IPv4 localhost
    if (ipAddress.isEmpty())
      ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    statusLabel->setText(tr("The server is running on\n\nIP: %1\nport: %2\n\n"
			    "Run the Fortune Client example now.")
			 .arg(ipAddress).arg(tcpServer->serverPort()));
  }

  void Server::acceptUser() {
    //to be called every time a new connection is received
    QTcpSocket *socket = tcpServer->nextPendingConnection();
    debugInfo("New incoming connection, from IP " +
	      socket->peerAddress().toString() +
	      " and port: " + QString::number(socket->peerPort()));
    //check if ip is registered
    if (!isThisIpRegistered(socket->peerAddress().toString())) {
      //then parse user
      UE new_ue;
      new_ue.name = "";
      new_ue.ip = socket->peerAddress().toString();
      //will change when ue_name() is sent
      new_ue.rx_port = 0;
      m_online_users.append(new_ue);
      debugInfo("New empty UE registered!");
    } else {
      debugInfo("user is transmitting either its name or data");
      socket->waitForReadyRead(1000);
      //parse data
      QDataStream in(socket);
      in.setVersion(QDataStream::Qt_4_0);
      debugInfo("blocksize: " + QString::number(blockSize));
      if (blockSize == 0) {
	if (socket->bytesAvailable() < (int)sizeof(quint16))
	  return;
      
	in >> blockSize;
      }
      debugInfo("bytes available in socket: " + QString::number(socket->bytesAvailable()));
    
      if (socket->bytesAvailable() < blockSize)
	return;
    
      QString message;
      in >> message;
    
      debugInfo(">Message: [" + message + "]");

      ProtocolStreamType_UE type;
      QStringList params = m_protocol->parseStream_UE(type, message);

      switch (type) {
      case UE_REGISTER:
	{
	  QString temp_name = params.at(0);
	  quint16 temp_port = (quint16) params.at(1).toInt();
	  DLOG (INFO) << "Parsed port: " << temp_port;
	  if (temp_name.isEmpty()) {
	    blockSize=0;
	    return;
	  }
	  UE temp;
	  int index;
	  if (!isThisNameRegistered(temp_name)) {
	    //case for same ip, different name
	    debugInfo("New user " + temp_name + " connected from same IP. Registering user.");
	    temp.name = temp_name;
	    temp.ip = socket->peerAddress().toString();
	    //parse ue_rx_port
	    temp.rx_port = temp_port;
	    index = getIndexOfUEIp(socket->peerAddress().toString());
	    if (m_online_users.at(index).name.isEmpty()) {
	      //first time, when username is still empty
	      if (index != -1) {
		temp = m_online_users.at(index);
		temp.name = temp_name;
		temp.rx_port = temp_port;
		m_online_users.replace(index,temp);
	      }
	    } else {
	      //same ip but different username, then append new UE
	      m_online_users.append(temp);
	    }
	  } else {
	    LOG (ERROR) << "User already exists on server. Notifying user...";
	    //inform user of currently online users
	    QByteArray block;
	    QDataStream out(&block, QIODevice::WriteOnly);
	    out.setVersion(QDataStream::Qt_4_0);
	    out << (quint16)0;
	    out << QString("ue_error(Existing user on server. Choose other username);");
	    out.device()->seek(0);
	    out << (quint16)(block.size() - sizeof(quint16));
	    DLOG (INFO) <<"Sending error message to UE ...\n";
	    logLabel->setText(logLabel->toPlainText()
			      + "\nError: attempted connection with same "
			      "username from same IP. Sending error to client...");
	    socket->write(block);
	    //reset blocksize
	    blockSize = 0;
	    return;
	  }
	  DLOG (INFO) << "New user is online: " << temp;
	  debugInfo("Nr. online users: " + QString::number(m_online_users.size()));
	  if (logLabel->toPlainText() != "") {
	    logLabel->setText(logLabel->toPlainText() + "\n[" + temp.name + "]@" +
			      temp.ip + ":" +
			      QString::number(temp.rx_port) + " is now online.");
	  } else {
	    logLabel->setText(logLabel->toPlainText() + "[" + temp.name + "]@" +
			      temp.ip + ":" +
			      QString::number(temp.rx_port) + " is now online.");
	  }
	  //parse online users
	  QString users;
	  for (auto user: m_online_users) {
	    users += user.name + "\n";
	  }
	  users.chop(1);
	  onlineUsers->setText(users);
	  qobject_cast<QLabel*>(mainLayout->itemAt(2)->widget())->setText("Currently online users("
									  + QString::number(m_online_users.size()) + "):");
	  //inform user of currently online users
	  QByteArray block;
	  QDataStream out(&block, QIODevice::WriteOnly);
	  out.setVersion(QDataStream::Qt_4_0);
	  out << (quint16)0;
	  QStringList params;
	  for (auto user: m_online_users) {
	    params << user.name;
	  }
	  out << m_protocol->constructStream_Server(params,
						    ProtocolStreamType_Server::SERVER_ALL);
	  out.device()->seek(0);
	  out << (quint16)(block.size() - sizeof(quint16));
	  DLOG (INFO) <<"Sending information about currently online users...\n";
	  /*At this point, this block will be sent to all current users, not only to the
	    user that is currently connected*/
	  for (auto connection: m_online_users) {
	    QTcpSocket *temp_socket = new QTcpSocket(this);
	    temp_socket->connectToHost(QHostAddress(connection.ip), connection.rx_port);
	    if (!temp_socket->waitForConnected(3000)) {
	      LOG (ERROR) << "ERROR: Connection attempt @"
			  << connection.ip.toStdString() << ":"
			  << connection.rx_port << " timed out. Omitting current...";
	    } else {
	      debugInfo("Connection to client @" + connection.ip + ":"
			+ QString::number(connection.rx_port) + " was established. Now sending...");
	      temp_socket->write(block);
	      if (!temp_socket->waitForBytesWritten()) {
		LOG (ERROR) << "ERROR: Connection attempt @"
			    << connection.ip.toStdString() << ":"
			    << connection.rx_port << " timed out. Omitting current...";
	      } else {
		debugInfo("Transmission to client @" + connection.ip + ":"
			  + QString::number(connection.rx_port) + " was successful!");
	      }
	    }
	    temp_socket->disconnectFromHost();
	    if (temp_socket->state() == QAbstractSocket::UnconnectedState ||
		temp_socket->waitForDisconnected(1000)) {
	      debugInfo("Socket disconnected.");
	    }
	  }

	  break;

	  
	}
      case UE_ACK:
	{
	  logLabel->setText(logLabel->toPlainText() + "\n" + message);
	  debugInfo("Going to forward user ack to destination");
	  QByteArray block;
	  QDataStream out(&block, QIODevice::WriteOnly);
	  out.setVersion(QDataStream::Qt_4_0);
	  out << (quint16)0;
	  out << m_protocol->constructStream_Server(QStringList(message),
						    ProtocolStreamType_Server::SERVER_FWD_TO_SENDER);
	  out.device()->seek(0);
	  out << (quint16)(block.size() - sizeof(quint16));

	  //and lookup destination details for given user
	  QString dest = params.at(0);
	  QString from = params.at(1);
	  unsigned int message_id = (unsigned int) params.at(2).toInt();

	  //Create temporary socket
	  QTcpSocket* dest_socket = new QTcpSocket(this);
	  QString dest_ip;
	  quint16 dest_port;
	  int index = getIndexOfUEName(dest);
	  if (index != -1) {
	    dest_ip = m_online_users.at(index).ip;
	    dest_port = m_online_users.at(index).rx_port;
	    debugInfo("Going to forward ack to " + dest_ip + ":" + QString::number(dest_port));
	  } else {
	    LOG (ERROR) << "ERROR: name was not found on server. Returning...";
	    blockSize=0;
	    return;
	  }
	  dest_socket->connectToHost(QHostAddress(dest_ip), dest_port);

	  if (!dest_socket->waitForConnected(2000)) {
	    debugInfo("ERROR: request timed out");
	  } else {
	    debugInfo("Established connection with client. Forwarding user ack...");
	    dest_socket->write(block);
	    if (!dest_socket->waitForBytesWritten(5000)) {
	      debugInfo("ERROR: transmission timed out");
	    } else {
	      debugInfo("Success! ACK was forwarded to destination");
	    }
	    dest_socket->disconnectFromHost();
	  }
	  break;
	}
      case UE_ERROR:
	{
	  debugInfo("Some error encountered by user. Returning ...");
	  blockSize=0;
	  return;
	}
      case UE_MESSAGE:
	{
	  logLabel->setText(logLabel->toPlainText() + "\n" + message);
	  //and send it back to the user
	  debugInfo("Going to resend message to dest client: [" + message + "]");
	  
	  QString content = params.at(0);
	  QString dest = params.at(1);
	  QString from = params.at(2);
	  unsigned int message_id = (unsigned int) params.at(3).toInt();
	  
	  DLOG (INFO) << "Message: " << content.toStdString() << ", from " << from.toStdString()
		      << " and to " << dest.toStdString()
		      << ", with message ID: " << message_id;
	  QByteArray block;
	  QDataStream out(&block, QIODevice::WriteOnly);
	  out.setVersion(QDataStream::Qt_4_0);
	  out << (quint16)0;
	  out << m_protocol->constructStream_Server(QStringList(message),
						    ProtocolStreamType_Server::SERVER_FWD_TO_SENDER);
	  out.device()->seek(0);
	  out << (quint16)(block.size() - sizeof(quint16));
	  if (dest == from) {
	    debugInfo("WARNING: Message intended for self UE. Sending back to user...");
	    socket->write(block);
	    if (!socket->waitForBytesWritten(2000)) {
	      LOG (ERROR) << "ERROR: transmission timeout";
	    } else {
	      debugInfo("Success!");
	    }
	  } else {
	    QTcpSocket *dest_socket = new QTcpSocket(this);
	    QString dest_ip;
	    quint16 dest_port;
	    int index = getIndexOfUEName(dest);
	    if (index != -1) {
	      dest_ip = m_online_users.at(index).ip;
	      dest_port = m_online_users.at(index).rx_port;
	      debugInfo("Going to forward message to " + dest_ip + ":" + QString::number(dest_port));
	    } else {
	      LOG (ERROR) << "ERROR: name was not found on server. Returning...";
	      blockSize=0;
	      return;
	    }
	    dest_socket->connectToHost(QHostAddress(dest_ip), dest_port);
	    if (!dest_socket->waitForConnected(2000)) {
	      debugInfo("ERROR: request timed out");
	    } else {
	      debugInfo("Established connection with client. Sending...");
	      dest_socket->write(block);
	      if (!dest_socket->waitForBytesWritten(5000)) {
		debugInfo("ERROR: transmission timed out");
	      } else {
		debugInfo("Success! Message was forwarded to destination");
	      }
	      dest_socket->disconnectFromHost();
	      //and send an ack to the user to inform that message was received
	      QByteArray ack_data;
	      QDataStream ack(&ack_data, QIODevice::WriteOnly);
	      ack.setVersion(QDataStream::Qt_4_0);
	      ack << (quint16)0;
	      QStringList params;
	      params << from << QString::number(message_id);
	      ack << m_protocol->constructStream_Server(params,
							ProtocolStreamType_Server::SERVER_ACK);
	      ack.device()->seek(0);
	      debugInfo("Sending ack to user: "  + from);
	      socket->write(ack_data);
	      if (!socket->waitForBytesWritten(2000)) {
		LOG (ERROR) << "ERROR: transmission timeout!";
	      } else {
		debugInfo("Success!");
	      }
	    }
	  }
	  break;
	}
      case UE_UNREGISTER:
	{
	  unregisterUser();
	  break;
	}
      case UE_TYPING:
	{
	  debugInfo("User is typing...");
	  logLabel->setText(logLabel->toPlainText() + "\n" + message);
	  //no need to parse parameters, going to forward to user
	  
	  QString dest = params.at(0);
	  QString from = params.at(1);

	  debugInfo(dest + "," + from);
	  
	  QByteArray typing_data;
	  QDataStream typing_stream(&typing_data, QIODevice::WriteOnly);
	  typing_stream.setVersion(QDataStream::Qt_4_0);
	  typing_stream << (quint16)0;
	  QStringList typing_params;
	  typing_params << params.at(0) << params.at(1) << params.at(2);
	  typing_stream << m_protocol->constructStream_Server(typing_params,
							      ProtocolStreamType_Server::SERVER_FWD_TYPING);
	  typing_stream.device()->seek(0);

	  DLOG (INFO) << "Sending: " << m_protocol->constructStream_Server(typing_params,
							    ProtocolStreamType_Server::SERVER_FWD_TYPING).toStdString();
	  
	  QTcpSocket *dest_socket = new QTcpSocket(this);
	  QString dest_ip;
	  quint16 dest_port;
	  int index = getIndexOfUEName(dest);
	  if (index != -1) {
	    dest_ip = m_online_users.at(index).ip;
	    dest_port = m_online_users.at(index).rx_port;
	    debugInfo("Going to forward typing info to " +
		      dest_ip + ":" + QString::number(dest_port));
	  } else {
	    LOG (ERROR) << "ERROR: name was not found on server. Returning...";
	    blockSize=0;
	    return;
	  }
	  dest_socket->connectToHost(QHostAddress(dest_ip), dest_port);
	  if (!dest_socket->waitForConnected(2000)) {
	    debugInfo("ERROR: request timed out");
	  } else {
	    debugInfo("Established connection with client. Sending...");
	    dest_socket->write(typing_data);
	    if (!dest_socket->waitForBytesWritten(5000)) {
	      debugInfo("ERROR: transmission timed out");
	    } else {
	      debugInfo("Success! Typing information was forwarded to destination");
	    }
	    dest_socket->disconnectFromHost();
	  }

	  break;
	}
      default:
	LOG (WARNING) << "Unrecognized stream type";
	break;
      }
    
    }
    //reset blocksize
    blockSize=0;
  }

  void Server::unregisterUser() {
    DLOG (INFO) << "New user is now offline: " << m_online_users.at(0);
    if (m_online_users.at(0).name != "") {
      logLabel->setText(logLabel->toPlainText() + "\n[" + m_online_users.at(0).name + "]@" +
			m_online_users.at(0).ip + ":" +
			QString::number(m_online_users.at(0).rx_port) + " is now offline.");
      QString current = onlineUsers->toPlainText();
      DLOG (INFO) << "Removing: " << current.remove(m_online_users.at(0).name, Qt::CaseInsensitive).toStdString() ;
      onlineUsers->setText(current);
      m_online_users.removeAt(0);
      qobject_cast<QLabel*>(mainLayout->itemAt(2)->widget())->setText("Currently online users("
								      + QString::number(m_online_users.size()) + "):");
    }
  }

  QString Server::filterMessage(QString& dest, QString &from, QString data) {
    //currently, a data structure has the following format
    //    ue_message(message);ue_dest(dest);ue_from(sender);
    //This function extracts both dest and message
    int i1 = data.indexOf("ue_message") + QString("ue_message").size() + 1;
    int i2 = data.indexOf("ue_dest") - 2;
    int i3 = data.indexOf("ue_dest") + QString("ue_dest").size() + 1;
    int i4 = data.indexOf("ue_from") - 2;
    int i5 = data.indexOf("ue_from") + QString("ue_from").size() + 1;
    int i6 = data.lastIndexOf(";") - 1;
    dest = data.mid(i3, i4 - i3) ;
    from = data.mid(i5, i6 - i5);
    debugInfo("Parsed dest:" + dest);
    debugInfo("Parsed dender:" + from);
    debugInfo("Parsed message:" + data.mid(i1, i2 - i1));
    return data.mid(i1, i2 - i1);
  }

  void Server::setDebugMode(bool debug_mode) {
    m_debug = debug_mode;
    if (m_debug)
      DLOG (INFO) << "Debug mode enabled";
    else
      DLOG (INFO) << "Debug mode disabled";
  }

  void Server::debugInfo(const QString& info) {
    if (m_debug)
      DLOG (INFO) << info.toStdString();
  }

  bool Server::isThisIpRegistered(const QString& ip) {
    for (auto user: m_online_users) {
      if (user.ip == ip) {
	debugInfo(ip + " found a match, already registered.");
	return true;
      }
    }
    debugInfo(ip + " did not found a match, going to register.");
    return false;
  }

  bool Server::isThisNameRegistered(const QString& name) {
    for (auto user: m_online_users) {
      if (user.name == name) {
	debugInfo(name + " found a match, already registered.");
	return true;
      }
    }
    debugInfo(name + " did not found a match, going to register.");
    return false;
  }

  int Server::getIndexOfUEName(const QString& name) {
    for (unsigned i = 0; i < m_online_users.size(); ++i) {
      if (m_online_users.at(i).name == name) {
	debugInfo(name + " found a match at index: " + QString::number(i));
	return i;
      }
    }
    debugInfo(name + " was not found on list. Returning -1.");
    return -1;
  }

  int Server::getIndexOfUEIp(const QString& ip) {
    for (unsigned i = 0; i < m_online_users.size(); ++i) {
      if (m_online_users.at(i).ip == ip) {
	debugInfo(ip + " found a match at index: " + QString::number(i));
	return i;
      }
    }
    debugInfo(ip + " was not found on list. Returning -1.");
    return -1;
  }

  std::ostream &operator<<(std::ostream &os, const UE& user) {
    os << "Name: " << user.name.toStdString()
       << ", IP: " << user.ip.toStdString()
       << ", RX port: " << user.rx_port ;
    return os;
  }

  bool operator==(const UE& u1, const UE& u2) {
    if (u1.name == u2.name &&
	u1.ip == u2.ip)
      return true;
    else return false;
  }

}
