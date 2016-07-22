#include <QtGui>
#include <QtNetwork>
#include <QTextEdit>

#include <iostream>
#include <glog/logging.h>

#include "client.hpp"
#include "definitions.hpp"

namespace tcp_messenger {
  
  Client::Client(QWidget *parent)
    :   QDialog(parent),
	networkSession(0) {
    
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

    m_stream_generator = new Protocol(PROTOCOL_VERSION);
    
    //chat window
    m_window = new ChatWrapper();
    m_window->setFixedSize(560,380);
    //m_window->setObjectName("ChatWindowWrapper");
    //m_window->setStyleSheet("#ChatWindowWrapper {background-color: white; border : 2px solid black;}");
    
    connect(this, SIGNAL(currentWindowChanged(const QString&)), m_window,
	    SLOT(currentWindowChangedSlot(const QString&)));
    
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
    m_listen_socket = new QTcpServer(this);
    connect(m_listen_socket, SIGNAL(newConnection()), this, SLOT(newServerConnection()));
  
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
    connect(m_box, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChangedSlot(int)));

    QPushButton *send = new QPushButton(tr("&Send"));
    connect(send, SIGNAL(clicked()), this, SLOT(sendMessage()));
  
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
    mainLayout->addWidget(new QLabel("Select user from list:"), 5, 0);
    mainLayout->addWidget(m_box, 6, 0);
    mainLayout->addWidget(checkbox, 6, 1);
    mainLayout->addWidget(m_window, 7, 0, 1, 2, Qt::AlignCenter);
    mainLayout->addWidget(messageLabel, 8, 0);
    mainLayout->addWidget(messageLineEdit, 8, 1);
    mainLayout->addWidget(send, 8, 2);
    mainLayout->addWidget(statusLabel, 9, 0, 1, 2);
    setLayout(mainLayout);

    setWindowTitle(tr("TCP Messenger by Santiago Pagola"));
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
    debugInfo("Reading incoming data from server...");
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
  
    debugInfo("Stream received: " + message);

    ProtocolStreamType_UE type;
    QStringList params = m_stream_generator->parseStream_UE(type,message);
    /********/
    for (auto param: params) {
      DLOG (INFO) << "Param: [" << param.toStdString() << "]";
    }
    /********/
    switch (type) {
    case SERVER_ALL:
      {
      QStringList userlist = params.at(0).split("-");
      userlist.insert(0, "Select from list...");
      m_box->addItems(userlist);
      debugInfo("Online users:" + userlist.join("-"));
      //create qmaps with empty conversations
      for (unsigned i = 1; i < userlist.size(); ++i) {
	//and create conversation windows if not exisiting
	if (m_window->addUser(userlist.at(i))) {
	  debugInfo("User " + userlist.at(i) + " added to conversation stack");
	} else {
	  debugInfo("User " + userlist.at(i) + " was already registered.");
	}
      }
      //and change icon
      QPixmap image;
      if (image.load(QString::fromStdString("images/online.png"))) {
	m_status->setPixmap(image);
	debugInfo("Icon changed!");
      }
      //and disable server fields
      enableServerFields(false);
      break;
      }
    case SERVER_ACK:
      {
      m_window->setMessageStatus((unsigned int) params.at(1).toInt(), 1);
      debugInfo("Message was seen");
      break;
      }
    case SERVER_ERROR:
      {
      QMessageBox::information(this, tr("Error"),
			       tr("The connection was refused by the peer. "
				  "A username matching yours was already found "
				  "on the server on the same IP. "
				  "Choose a different username."));
      enableGetFortuneButton();
      m_online = false;
      blockSize = 0;
      break;
      }
    default:
      {
      debugInfo("Self message detected");
      DLOG (INFO) << m_window->newMessageFromUser(params.at(0), false, params.at(2));
      break;
      }
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
    if (event->key() == ENTER  &&
	messageLineEdit->hasFocus()) {
      sendMessage();
    }
  }

  void Client::sendMessage() {
    debugInfo("Message slot called");
    if (!messageLineEdit->toPlainText().isEmpty() &&
	messageLineEdit->isEnabled()) {
      QString message = messageLineEdit->toPlainText();
      
      unsigned int message_id =
	m_window->newMessageFromUser("Me:" + message,true,m_box->currentText());
      
      QString user = m_box->itemText(m_box->currentIndex());
      messageLineEdit->clear();
      //Fill parameters to send
      QStringList params;
      params << message << user << usernameLineEdit->text() << QString::number(message_id);
      //and send it to the server
      blockSize = 0;
      m_transmission_socket->abort();
      m_transmission_socket->connectToHost(hostLineEdit->text(),
					   portLineEdit->text().toInt());
      QString string_stream =
	m_stream_generator->constructStream_UE(params,							     ProtocolStreamType_UE::UE_MESSAGE);
      
      QByteArray block;
      QDataStream out(&block, QIODevice::WriteOnly);
      out.setVersion(QDataStream::Qt_4_0);
      out << (quint16)0;
      out << string_stream;
      out.device()->seek(0);
      out << (quint16)(block.size() - sizeof(quint16));
      debugInfo( "Transmitting stream: " + string_stream);
      m_transmission_socket->write(block);
      DLOG (INFO) << "Stream sent to server!";
    }
  }

  void Client::nowOnline() {
    //start listening server
    quint16 listening_port = LISTEN_PORT;
    debugInfo("Attempting connection on port: " + QString::number(listening_port));
    while (!m_listen_socket->listen(QHostAddress(hostLineEdit->text()), listening_port) &&
	   listening_port <= 65535) {
      listening_port += 2;
      debugInfo("Error: Trying next port: " + QString::number(listening_port));
    }
    if (listening_port < 65536) {
      debugInfo("Success: client will listen to server @ " +
		hostLineEdit->text() + ":" + QString::number(listening_port));
      
    } else {
      LOG (ERROR) << "No available ports were found. Closing client...";
      close();
      return;
    }
    
    DLOG (INFO) << "Successfully connected to server. Sending user info";
    blockSize = 0;
    m_transmission_socket->abort();
    m_transmission_socket->connectToHost(hostLineEdit->text(),
					 portLineEdit->text().toInt());
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << (quint16)0;
    out << QString("ue_name(" + usernameLineEdit->text() + ");ue_rx_port(" + QString::number(listening_port) + ")");
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));

    m_transmission_socket->write(block);
  
    //online variable
    m_online = true;
    debugInfo("Success!");    
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

  QString Client::filterMessage(QString& dest, QString& from, QString data) {
    //currently, a data structure has the following format
    //    ue_message(message);ue_dest(dest);ue_from(sender);
    //This function extracts both dest and message
    int i1 = data.indexOf("ue_message") + QString("ue_message").size() + 1;
    int i2 = data.indexOf("ue_dest") - 2;
    int i3 = data.indexOf("ue_dest") + QString("ue_dest").size() + 1;
    int i4 = data.indexOf("ue_from") - 2;
    int i5 = data.indexOf("ue_from") + QString("ue_from").size() + 1;
    int i6 = data.lastIndexOf(";") - 1;
    dest = data.mid(i3, i4 - i3);
    from = data.mid(i5, i6 - i5);
    debugInfo("Parsed dest:" + dest);
    debugInfo("Parsed sender:" + from);
    debugInfo("Parsed message:" + data.mid(i1, i2 - i1));
    return data.mid(i1, i2 - i1);
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

  void Client::currentIndexChangedSlot(int index) {
    QString current_conver = m_box->itemText(index);
    if (index != 0) {
      emit currentWindowChanged(current_conver);
      debugInfo("New index: " + QString::number(index) + " (chatting with " + current_conver + ")");
      debugInfo("Switched conversation");
    }
    if (index == 0) {
      //disable message field
      messageLineEdit->setEnabled(false);
      messageLineEdit->setStyleSheet("background-color: #C0C0C0;");
    } else {
      //disable message field
      messageLineEdit->setEnabled(true);
      messageLineEdit->setStyleSheet("background-color: #FFFFFF;");
    }
  }

  void Client::newServerConnection() {
    QTcpSocket *socket = m_listen_socket->nextPendingConnection();
    debugInfo("Incoming connection from server: " + socket->peerAddress().toString() + ":"
	      + QString::number(socket->peerPort()));
    if (!socket->waitForReadyRead(5000)) {
      debugInfo("ERROR: transmission timed out");
    } else {
      //parse data
      QDataStream in(socket);
      in.setVersion(QDataStream::Qt_4_0);
      debugInfo("blocksize: " + QString::number(blockSize));
      debugInfo("bytes available in client socket: " + QString::number(socket->bytesAvailable()));
      if (blockSize == 0) {
	if (socket->bytesAvailable() < (int)sizeof(quint16))
	  return;
      
	in >> blockSize;
      }
        
      if (socket->bytesAvailable() < blockSize)
	return;
    
      QString message;
      in >> message;
    
      debugInfo("Message: [" + message + "]");

      ProtocolStreamType_UE type;
      QStringList params = m_stream_generator->parseStream_UE(type,message);
      /********/
      for (auto param: params) {
	DLOG (INFO) << "Param: [" << param.toStdString() << "]";
      }
      /********/
      switch (type) {
      case SERVER_ALL:
	{
	  QStringList userlist = params.at(0).split("-");
	  userlist.insert(0, "Select from list...");
	  m_box->addItems(userlist);
	  debugInfo("Online users:" + userlist.join("-"));
	  //create qmaps with empty conversations
	  for (unsigned i = 1; i < userlist.size(); ++i) {
	    //and create conversation windows if not exisiting
	    if (m_window->addUser(userlist.at(i))) {
	      debugInfo("User " + userlist.at(i) + " added to conversation stack");
	    } else {
	      debugInfo("User " + userlist.at(i) + " was already registered.");
	    }
	  }
	  //and change icon
	  QPixmap image;
	  if (image.load(QString::fromStdString("images/online.png"))) {
	    m_status->setPixmap(image);
	    debugInfo("Icon changed!");
	  }
	  //and disable server fields
	  enableServerFields(false);
	  break;
	}
      case SERVER_ACK:
	{
	  m_window->setMessageStatus((unsigned int) params.at(1).toInt(), 1);
	  debugInfo("Message was seen");
	  break;
	}
      case SERVER_ERROR:
	{
	  QMessageBox::information(this, tr("Error"),
				   tr("The connection was refused by the peer. "
				      "A username matching yours was already found "
				      "on the server on the same IP. "
				      "Choose a different username."));
	  enableGetFortuneButton();
	  m_online = false;
	  blockSize = 0;
	  break;
	}
      default:
	{
	  debugInfo("Self message detected");
	  DLOG (INFO) << m_window->newMessageFromUser(params.at(0), false, params.at(2));
	  break;
	}
      }
  

      /*if (message.contains("ue_all(", Qt::CaseSensitive)) { //info about users
	QString users = message.mid(message.indexOf("(") + 1,
				    message.indexOf(")") - message.indexOf("(") - 1);
	QStringList userlist = users.split('-');
	m_box->clear();
	userlist.insert(0, "Select from list...");
	m_box->addItems(userlist);
	debugInfo("Online users:" + users);
	//create qmaps with empty conversations
	for (unsigned i = 1; i < userlist.size(); ++i) {
	  //and create conversation windows if not exisiting
	  if (m_window->addUser(userlist.at(i))) {
	    debugInfo("User " + userlist.at(i) + " added to conversation stack");
	  } else {
	    debugInfo("User " + userlist.at(i) + " was already registered.");
	  }
	}
	//and change icon
	QPixmap image;
	if (image.load(QString::fromStdString("images/online.png"))) {
	  m_status->setPixmap(image);
	  debugInfo("Icon changed!");
	}
	//and disable server fields
	enableServerFields(false);
      } else {
	QString dest, from;
	QString content = filterMessage(dest, from, message);
	
	//next, change corresponding label
	int index = m_box->findText(from);
	if (index != -1)
	  m_box->setCurrentIndex(index);
	//and update text buffers
	m_window->newMessageFromUser(from + ":" + content,false,from);
	
      }*/
    }
    blockSize=0;
  }
  
}
