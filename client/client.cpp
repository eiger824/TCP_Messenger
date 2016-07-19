#include <QtGui>
#include <QtNetwork>
#include <QTextEdit>

#include <iostream>
#include <glog/logging.h>

#include "client.h"

Client::Client(QWidget *parent)
  :   QDialog(parent), networkSession(0)
{
  hostLabel = new QLabel(tr("&Server name:"));
  portLabel = new QLabel(tr("S&erver port:"));
  messageLabel = new QLabel(tr("Mess&age to send:\n(CTRL + Enter to send)"));
  m_username = new QLabel(tr("&Username:"));
  m_connection_status = new QLabel("Connection status:");
  m_status = new QLabel;
  QPixmap image;
  if (image.load("images/offline.png")) {
    m_status->setPixmap(image);
  } else m_status->setText("Error loading icon");
  
  m_chat = new QTextEdit;
  m_chat->setFixedSize(400,400);
  m_chat->setEnabled(true);
  m_chat->setReadOnly(true);
  m_chat->setStyleSheet("border: 2px solid black; background-color: white; color: black;");
  m_chat->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  
  blockSize=0;
  m_online = false;
  m_debug = false;
  // find out which IP to connect to
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

  hostLineEdit = new QLineEdit(ipAddress);
  portLineEdit = new QLineEdit();
  messageLineEdit = new QTextEdit;
  messageLineEdit->setEnabled(false);
  messageLineEdit->setStyleSheet("background-color: #C0C0C0;");
  messageLineEdit->setFixedHeight(100);
  messageLineEdit->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  portLineEdit->setValidator(new QIntValidator(1, 65535, this));
  usernameLineEdit = new QLineEdit("Santi");

  hostLabel->setBuddy(hostLineEdit);
  portLabel->setBuddy(portLineEdit);
  messageLabel->setBuddy(messageLineEdit);
  m_username->setBuddy(usernameLineEdit);

  statusLabel = new QLabel(tr("Basic TCP messenger application developed by Santiago Pagola"));

  getFortuneButton = new QPushButton(tr("Connect"));
  getFortuneButton->setDefault(true);
  getFortuneButton->setEnabled(false);

  quitButton = new QPushButton(tr("Disconnect"));

  buttonBox = new QDialogButtonBox;
  buttonBox->addButton(getFortuneButton, QDialogButtonBox::ActionRole);
  buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

  //investigate how to set local ports!
  m_connection_socket = new QTcpSocket(this);
  //m_connection_socket->setLocalPort(30500);
  m_transmission_socket = new QTcpSocket(this);
  //m_transmission_socket->setLocalPort(30502);
  
  connect(hostLineEdit, SIGNAL(textChanged(QString)),
	  this, SLOT(enableGetFortuneButton()));
  connect(portLineEdit, SIGNAL(textChanged(QString)),
	  this, SLOT(enableGetFortuneButton()));
  connect(getFortuneButton, SIGNAL(clicked()),
	  this, SLOT(getOnline()));
  connect(quitButton, SIGNAL(clicked()), this, SLOT(getOffline()));
  connect(m_transmission_socket, SIGNAL(readyRead()), this, SLOT(dataReceived()));
  connect(m_connection_socket, SIGNAL(error(QAbstractSocket::SocketError)),
	  this, SLOT(displayError(QAbstractSocket::SocketError)));
  connect(m_connection_socket, SIGNAL(connected()), this,
	  SLOT(nowOnline()));
  connect(m_connection_socket, SIGNAL(disconnected()), this,
	  SLOT(nowOffline()));

  checkbox = new QCheckBox("Enable debug messages on background", this);
  connect(checkbox, SIGNAL(toggled(bool)), this, SLOT(changeDebugMode(bool)));

  m_box = new QComboBox;
  m_box->addItem("Select from list...");
  
  QGridLayout *mainLayout = new QGridLayout;
  mainLayout->addWidget(hostLabel, 0, 0);
  mainLayout->addWidget(hostLineEdit, 0, 1);
  mainLayout->addWidget(portLabel, 1, 0);
  mainLayout->addWidget(portLineEdit, 1, 1);
  mainLayout->addWidget(m_username, 2, 0);
  mainLayout->addWidget(usernameLineEdit, 2, 1);
  mainLayout->addWidget(m_connection_status, 3, 0);
  mainLayout->addWidget(m_status, 3, 1);
  mainLayout->addWidget(buttonBox, 4, 0, 1, 2);
  mainLayout->addWidget(new QLabel("Currently online users:"), 5, 0);
  mainLayout->addWidget(m_box, 6, 0);
  mainLayout->addWidget(checkbox, 6, 1);
  mainLayout->addWidget(m_chat, 7, 0, 1, 2, Qt::AlignCenter);
  mainLayout->addWidget(messageLabel, 8, 0);
  mainLayout->addWidget(messageLineEdit, 8, 1);
  mainLayout->addWidget(statusLabel, 9, 0, 1, 2);
  setLayout(mainLayout);

  setWindowTitle(tr("Fortune Client"));
  portLineEdit->setFocus();

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

    getFortuneButton->setEnabled(false);
    statusLabel->setText(tr("Opening network session."));
    networkSession->open();
  }
  enableGetFortuneButton();
  this->layout()->setSizeConstraint( QLayout::SetFixedSize );
}

void Client::getOnline()
{
  //1)client registration on server
  getFortuneButton->setEnabled(false);
  blockSize = 0;
  m_connection_socket->abort();
  m_connection_socket->connectToHost(hostLineEdit->text(),
  			   portLineEdit->text().toInt());
}

void Client::dataReceived()
{
  if (m_debug)
    DLOG (INFO) << "Reading incoming data from server...";
  //2)server response with online users
  QDataStream in(m_transmission_socket);
  in.setVersion(QDataStream::Qt_4_0);

  if (blockSize == 0) {
    if (m_transmission_socket->bytesAvailable() < (int)sizeof(quint16))
      return;

    in >> blockSize;
  }

  if (m_transmission_socket->bytesAvailable() < blockSize)
    return;

  QString message;
  in >> message;
  
  if (m_debug)
    DLOG (INFO) << "Data received: " << message.toStdString();
  
  if (message.contains("ue_all(", Qt::CaseSensitive)) { //info about users
    QString users = message.mid(message.indexOf("(") + 1,
				message.indexOf(")") - message.indexOf("(") - 1);
    QString showing_users = users.insert(users.indexOf(usernameLineEdit->text()) + usernameLineEdit->text().size(), "(Me)");
    m_box->addItems(showing_users.split(13));
    debugInfo("Online users:" + showing_users);
    //and change icon
    QPixmap image;
    if (image.load(QString::fromStdString("images/online.png"))) {
      m_status->setPixmap(image);
      debugInfo("Icon changed!");
    }
    //and disable server fields
    enableServerFields(false);
  } else {
    QString dest;
    QString content = filterMessage(dest,message);
    DLOG (INFO) << "Message received: " << content.toStdString();
    
    m_chat->setText(m_chat->toPlainText() + "\n" + dest + ": " + content);
    getFortuneButton->setEnabled(true);
  }

  //reset blocksize
  blockSize=0;

  //disconnect from host
  m_transmission_socket->disconnectFromHost();
  DLOG (INFO) << "Success!";
}

void Client::displayError(QAbstractSocket::SocketError socketError)
{
  switch (socketError) {
  case QAbstractSocket::RemoteHostClosedError:
    break;
  case QAbstractSocket::HostNotFoundError:
    QMessageBox::information(this, tr("Fortune Client"),
			     tr("The host was not found. Please check the "
				"host name and port settings."));
    break;
  case QAbstractSocket::ConnectionRefusedError:
    QMessageBox::information(this, tr("Fortune Client"),
			     tr("The connection was refused by the peer. "
				"Make sure the fortune server is running, "
				"and check that the host name and port "
				"settings are correct."));
    break;
  default:
    QMessageBox::information(this, tr("Fortune Client"),
			     tr("The following error occurred: %1.")
			     .arg(m_connection_socket->errorString()));
  }

  getFortuneButton->setEnabled(true);
}

void Client::enableGetFortuneButton()
{
  getFortuneButton->setEnabled((!networkSession || networkSession->isOpen()) &&
			       !hostLineEdit->text().isEmpty() &&
			       !portLineEdit->text().isEmpty());

}

void Client::sessionOpened()
{
  // Save the used configuration
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

  statusLabel->setText(tr("Basic TCP messenger application developed by Santiago Pagola"));

  enableGetFortuneButton();
}

void Client::keyPressEvent(QKeyEvent *event) { 
  if (event->key() == ENTER &&
      !messageLineEdit->toPlainText().isEmpty() &&
      messageLineEdit->hasFocus()) {
    QString message = messageLineEdit->toPlainText();
    if (m_chat->toPlainText() != "") 
      m_chat->setText(m_chat->toPlainText() + "\nMe: " + message);
    else
      m_chat->setText("Me: " + message);

    messageLineEdit->clear();

    //and send it to the server
    blockSize = 0;
    m_transmission_socket->abort();
    m_transmission_socket->connectToHost(hostLineEdit->text(),
					 portLineEdit->text().toInt());
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << (quint16)0;
    out << "ue_message(" + message + ");ue_dest(PC_Santi_v2);";
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    debugInfo( "Transmitting message...");
    m_transmission_socket->write(block);
    DLOG (INFO) << "Message sent to server!";
  }
}

void Client::nowOnline() {
  DLOG (INFO) << "Successfully connected to server. Sending user info";
  blockSize = 0;
  m_transmission_socket->abort();
  m_transmission_socket->connectToHost(hostLineEdit->text(),
				       portLineEdit->text().toInt());
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_4_0);
  out << (quint16)0;
  out << QString("ue_name(" + usernameLineEdit->text() + ")");
  out.device()->seek(0);
  out << (quint16)(block.size() - sizeof(quint16));

  m_transmission_socket->write(block);
  
  //and enable message field
  messageLineEdit->setEnabled(true);
  messageLineEdit->setStyleSheet("background-color: white;");
  //online variable
  m_online = true;
  debugInfo("Success!");
  debugInfo("Changing icon status...");
}

void Client::nowOffline() {
  DLOG (INFO) << "Successfully disconnected from server.";
  m_online = false;
  //and disable message field in case server shuts down
  messageLineEdit->setEnabled(false);
  messageLineEdit->setStyleSheet("background-color: #C0C0C0;");

  debugInfo("Removing my name from list");
  //and remove my name from list
  m_box->removeItem(1);
}

void Client::getOffline() {
  if (m_online) {
    //notify server
    DLOG (INFO) << "About to go offline";
    blockSize = 0;
    m_transmission_socket->abort();
    m_transmission_socket->connectToHost(hostLineEdit->text(),
					 portLineEdit->text().toInt());
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << (quint16)0;
    out << QString("ue_disconnect()");
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));

    m_transmission_socket->write(block);
    m_transmission_socket->disconnectFromHost();

    //and close connection socket
    m_connection_socket->disconnectFromHost();
    enableGetFortuneButton();
    //and disable message field
    messageLineEdit->setEnabled(false);
    messageLineEdit->setStyleSheet("background-color: #C0C0C0;");
    //set icon
    QPixmap image;
    if (image.load("images/offline.png")) {
      m_status->setPixmap(image);
    } else m_status->setText("Error loading icon");
    enableServerFields(true);
  } else {
    DLOG (INFO) << "User already offline";
  }
}

QString Client::filterMessage(QString& dest, QString data) {
  //currently, a data structure has the following format
  //    ue_message(message);ue_dest(dest);
  //This function extracts both dest and message
  dest = data.mid(data.lastIndexOf("(") + 1, data.lastIndexOf(")") - data.lastIndexOf("(") - 1);
  return data.mid(data.indexOf("(") + 1, data.indexOf(")") - data.indexOf("(") - 1);
}

void Client::setData(bool debug_mode,
		     std::string username,
		     std::string ip,
		     std::string port) {
  m_debug = debug_mode;
  if (m_debug) {
    checkbox->toggle();
  }
  else
    DLOG (INFO) << "Debug mode is disabled";

  if (!QString::fromStdString(username).isEmpty())
    usernameLineEdit->setText(QString::fromStdString(username));
  if (!QString::fromStdString(ip).isEmpty())
    hostLineEdit->setText(QString::fromStdString(ip));
  if (!QString::fromStdString(port).isEmpty())
    portLineEdit->setText(QString::fromStdString(port));
}

void Client::debugInfo(const QString& info) {
  if (m_debug) {
    DLOG (INFO) << info.toStdString();
  }
}

void Client::changeDebugMode(bool toggled) {
  m_debug = toggled;
  if (m_debug)
    DLOG (INFO) << "Debug mode enabled";
  else
    DLOG (INFO) << "Debug mode disabled";
}

void Client::enableServerFields(bool enabled) {
  if (!enabled) { 
    hostLineEdit->setStyleSheet("background-color: #E0E0E0;");
    hostLineEdit->setEnabled(false);
    portLineEdit->setStyleSheet("background-color: #E0E0E0;");
    portLineEdit->setEnabled(false);
    usernameLineEdit->setStyleSheet("background-color: #E0E0E0;");
    usernameLineEdit->setEnabled(false);
  } else {
    hostLineEdit->setStyleSheet("background-color: #FFFFFF;");
    hostLineEdit->setEnabled(true);
    portLineEdit->setStyleSheet("background-color: #FFFFFF;");
    portLineEdit->setEnabled(true);
    usernameLineEdit->setStyleSheet("background-color: #FFFFFF;");
    usernameLineEdit->setEnabled(true);
  }
}
