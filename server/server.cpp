#include <QtGui>
#include <QtNetwork>

#include <stdlib.h>
#include <iostream>
#include <glog/logging.h>

#include "server.h"

Server::Server(QWidget *parent)
  :   QDialog(parent), tcpServer(0), networkSession(0)
{
  statusLabel = new QLabel;
  quitButton = new QPushButton(tr("Quit"));
  quitButton->setAutoDefault(false);
  
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
  debugInfo("New incoming connection");
  //to be called every time a new connection is received
  QTcpSocket *socket = tcpServer->nextPendingConnection();
  //parse user
  UE new_ue;
  new_ue.name = "";
  new_ue.ip = socket->peerAddress().toString();
  new_ue.port = socket->peerPort();
  if (m_online_users.isEmpty()) {
    m_online_users.append(new_ue);
  }
  if (m_online_users.at(0).ip != new_ue.ip) {
    m_online_users.append(new_ue);
    if (new_ue.name != "") {
      if (logLabel->toPlainText() != "") {
	logLabel->setText(logLabel->toPlainText() + "\n[" + new_ue.name + "]@" +
			  new_ue.ip + ":" +
			  QString::number(new_ue.port) + " is now online.");
      } else {
	logLabel->setText("[" + new_ue.name + "]@" +
			  new_ue.ip + ":" +
			  QString::number(new_ue.port) + " is now online.");
      }
    }
    //Don't close this socket, which is the server-end of the connection-socket object
    //socket->close();
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
    
    if (m_debug)
      DLOG (INFO) <<"Message: [" << message.toStdString() << "]" ;
    if (message.contains("ue_name")) {
      QString temp_name = message.mid(8, message.lastIndexOf(")") - message.lastIndexOf("(") - 1);
      UE temp = m_online_users.last();
      temp.name = temp_name;
      m_online_users.replace(m_online_users.size() -1,temp);
      DLOG (INFO) <<"New user is online: " << temp;
      debugInfo("Size of list: " + QString::number(m_online_users.size()));
      if (logLabel->toPlainText() != "") {
	logLabel->setText(logLabel->toPlainText() + "\n[" + temp.name + "]@" +
			  temp.ip + ":" +
			  QString::number(temp.port) + " is now online.");
      } else {
	logLabel->setText(logLabel->toPlainText() + "[" + temp.name + "]@" +
			  temp.ip + ":" +
			QString::number(temp.port) + " is now online.");
      }
      if (onlineUsers->toPlainText() != "") {
	onlineUsers->setText("\n" + temp.name);
      } else {
	onlineUsers->setText(temp.name);
      }
      qobject_cast<QLabel*>(mainLayout->itemAt(2)->widget())->setText("Currently online users("
								      + QString::number(m_online_users.size()) + "):");
      //inform user of currently online users
      QByteArray block;
      QDataStream out(&block, QIODevice::WriteOnly);
      out.setVersion(QDataStream::Qt_4_0);
      out << (quint16)0;
      QString names;
      for (auto item: m_online_users) {
	names+=item.name + "-";
      }
      names.chop(1);
      out << QString("ue_all(" + names + ");");
      out.device()->seek(0);
      out << (quint16)(block.size() - sizeof(quint16));
      DLOG (INFO) <<"Sending information about currently online users...\n";
      socket->write(block);
    } else if (message.contains("ue_disconnect")) {
      unregisterUser();
    } else { //regular message
      logLabel->setText(logLabel->toPlainText() + "\n" + message);
      //and send it back to the user (for now!!)
      debugInfo("Going to resend message to dest client: ["
		+ message + "]");
      QString content = message.mid(message.indexOf("(") + 1, message.indexOf(")") - message.indexOf("(") - 1);
      QString dest = message.mid(message.lastIndexOf("(") + 1, message.lastIndexOf(")") - message.lastIndexOf("(") - 1);
      debugInfo("(message,dest) = (" + content + ","
		+ dest + ")");
      QByteArray block;
      QDataStream out(&block, QIODevice::WriteOnly);
      out.setVersion(QDataStream::Qt_4_0);
      out << (quint16)0;
      out << QString("ue_message(" + content + ");ue_dest(" + dest + ");");
      out.device()->seek(0);
      out << (quint16)(block.size() - sizeof(quint16));
      debugInfo("Sending...");
      socket->write(block);
      DLOG (INFO) << "Sent!";
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
		      QString::number(m_online_users.at(0).port) + " is now offline.");
    QString current = onlineUsers->toPlainText();
    DLOG (INFO) << "Removing: " << current.remove(m_online_users.at(0).name, Qt::CaseInsensitive).toStdString() ;
    onlineUsers->setText(current);
    m_online_users.removeAt(0);
    qobject_cast<QLabel*>(mainLayout->itemAt(2)->widget())->setText("Currently online users("
								    + QString::number(m_online_users.size()) + "):");
  }
}

QString Server::filterMessage(QString& dest, QString data) {
  //currently, a data structure has the following format
  //    ue_message(message);ue_dest(dest);
  //This function extracts both dest and message
  dest = data.mid(data.lastIndexOf("(") + 1, data.lastIndexOf(")") - data.lastIndexOf("(") - 1);
  return data.mid(data.indexOf("(") + 1, data.indexOf(")") - data.indexOf("(") - 1);
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

std::ostream &operator<<(std::ostream &os, const UE& user) {
  os << "Name: " << user.name.toStdString()
     << ", IP: " << user.ip.toStdString()
     << ", Port: " << user.port ;
  return os;
}

bool operator==(const UE& u1, const UE& u2) {
  if (u1.name == u2.name &&
      u1.ip == u2.ip)
    return true;
  else return false;
}
