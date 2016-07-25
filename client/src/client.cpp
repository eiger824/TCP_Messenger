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
	m_network_session(0),
	m_listening_port(LISTEN_PORT),
	m_typing_hold(false) {
    
    m_hostname_label = new QLabel(tr("&Server name:"));
    m_port_label = new QLabel(tr("S&erver port:"));
    m_message_label = new QLabel(tr("Mess&age to send:\n(CTRL + Enter to send)"));
    m_username = new QLabel(tr("&Username:"));
    m_connection_status = new QLabel("Connection status:");
    m_status = new QLabel;
    QPixmap image;
    if (image.load("images/offline.png")) {
      m_status->setPixmap(image);
    } else m_status->setText("Error loading icon");

    m_protocol = new Protocol(PROTOCOL_VERSION);

    m_typing_label = new QLabel();
    
    //chat window
    m_window = new ChatContainer();
    
    connect(this, SIGNAL(currentWindowChanged(const QString&)), m_window,
	    SLOT(currentWindowChangedSlot(const QString&)));
    
    m_block_size=0;
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

    m_host_lineedit = new QLineEdit(ipAddress);
    m_port_lineedit = new QLineEdit();
    m_message_lineedit = new QTextEdit;
    m_message_lineedit->setEnabled(false);
    m_message_lineedit->setStyleSheet("background-color: #C0C0C0;");
    m_message_lineedit->setFixedHeight(100);
    m_message_lineedit->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_typing_timer = new QTimer(this);
    
    m_port_lineedit->setValidator(new QIntValidator(1, 65535, this));
    m_username_lineedit = new QLineEdit("Santi");

    m_hostname_label->setBuddy(m_host_lineedit);
    m_port_label->setBuddy(m_port_lineedit);
    m_message_label->setBuddy(m_message_lineedit);
    m_username->setBuddy(m_username_lineedit);

    m_status_label = new QLabel(tr("Basic TCP messenger application developed by Santiago Pagola"));

    m_connect_to_host = new QPushButton(tr("Connect"));
    m_connect_to_host->setDefault(true);
    m_connect_to_host->setEnabled(false);

    m_disconnect_from_host = new QPushButton(tr("Disconnect"));

    m_button_box = new QDialogButtonBox;
    m_button_box->addButton(m_connect_to_host, QDialogButtonBox::ActionRole);
    m_button_box->addButton(m_disconnect_from_host, QDialogButtonBox::RejectRole);

    m_connection_socket = new QTcpSocket(this);
    m_transmission_socket = new QTcpSocket(this);
    m_listen_socket = new QTcpServer(this);

    m_checkbox = new QCheckBox("Enable debug messages on background", this);

    m_box = new QComboBox;
    m_box->addItem("Select from list...");

    QPushButton *send = new QPushButton(tr("&Send"));
     
    connect(m_listen_socket, SIGNAL(newConnection()), this, SLOT(newServerConnection()));
    connect(m_host_lineedit, SIGNAL(textChanged(QString)), this, SLOT(enableConnectButton()));
    connect(m_port_lineedit, SIGNAL(textChanged(QString)), this, SLOT(enableConnectButton()));
    connect(m_connect_to_host, SIGNAL(clicked()), this, SLOT(getOnline()));
    connect(m_disconnect_from_host, SIGNAL(clicked()), this, SLOT(getOffline()));
    connect(m_transmission_socket, SIGNAL(readyRead()), this, SLOT(dataReceived()));
    connect(m_connection_socket, SIGNAL(error(QAbstractSocket::SocketError)),
	    this, SLOT(displayError(QAbstractSocket::SocketError)));
    connect(m_connection_socket, SIGNAL(connected()), this, SLOT(nowOnline()));
    connect(m_connection_socket, SIGNAL(disconnected()), this, SLOT(nowOffline()));
    connect(m_message_lineedit, SIGNAL(textChanged()), this, SLOT(textChangedSlot()));
    connect(m_typing_timer, SIGNAL(timeout()), this, SLOT(timeoutSlot()));
    connect(m_checkbox, SIGNAL(toggled(bool)), this, SLOT(changeDebugMode(bool)));
    connect(m_box, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChangedSlot(int)));
    connect(send, SIGNAL(clicked()), this, SLOT(sendMessage()));
  
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(m_hostname_label, 0, 0);
    mainLayout->addWidget(m_host_lineedit, 0, 1);
    mainLayout->addWidget(m_port_label, 1, 0);
    mainLayout->addWidget(m_port_lineedit, 1, 1);
    mainLayout->addWidget(m_username, 2, 0);
    mainLayout->addWidget(m_username_lineedit, 2, 1);
    mainLayout->addWidget(m_connection_status, 3, 0);
    mainLayout->addWidget(m_status, 3, 1);
    mainLayout->addWidget(m_button_box, 4, 0, 1, 2);
    mainLayout->addWidget(new QLabel("Select user from list:"), 5, 0);
    mainLayout->addWidget(m_box, 6, 0);
    mainLayout->addWidget(m_checkbox, 6, 1);
    mainLayout->addWidget(m_typing_label, 7,0);
    mainLayout->addWidget(m_window, 8, 0, 1, 2, Qt::AlignCenter);
    mainLayout->addWidget(m_message_label, 9, 0);
    mainLayout->addWidget(m_message_lineedit, 9, 1);
    mainLayout->addWidget(send, 9, 2);
    mainLayout->addWidget(m_status_label, 10, 0, 1, 2);
    setLayout(mainLayout);

    setWindowTitle(tr("TCP Messenger by Santiago Pagola"));
    m_port_lineedit->setFocus();

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

      m_network_session = new QNetworkSession(config, this);
      connect(m_network_session, SIGNAL(opened()), this, SLOT(sessionOpened()));

      m_connect_to_host->setEnabled(false);
      m_status_label->setText(tr("Opening network session."));
      m_network_session->open();
    }
    enableConnectButton();
    this->layout()->setSizeConstraint( QLayout::SetFixedSize );
  }

  void Client::getOnline()
  {
    //1)client registration on server
    m_connect_to_host->setEnabled(false);
    m_block_size = 0;
    m_connection_socket->abort();
    m_connection_socket->connectToHost(m_host_lineedit->text(),
				       m_port_lineedit->text().toInt());
  }

  void Client::dataReceived()
  {
    debugInfo("Reading incoming data from server...");
    QDataStream in(m_transmission_socket);
    in.setVersion(QDataStream::Qt_4_0);

    if (m_block_size == 0) {
      if (m_transmission_socket->bytesAvailable() < (int)sizeof(quint16))
	return;

      in >> m_block_size;
    }

    if (m_transmission_socket->bytesAvailable() < m_block_size)
      return;

    QString message;
    in >> message;
  
    debugInfo("Stream received: " + message);

    ProtocolStreamType_Server type;
    QStringList params = m_protocol->parseStream_Server(type,message);
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
	m_window->setMessageStatus(m_username_lineedit->text(),
				   (unsigned int) params.at(1).toInt(), 1);
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
	enableConnectButton();
	m_online = false;
	m_block_size = 0;
	break;
      }
      
    default:
      {
	debugInfo("Self message detected");
	DLOG (INFO) << m_window->newMessageFromUser(params.at(0), false, params.at(2));
	debugInfo("Setting seen status");
	m_window->setMessageStatus(m_username_lineedit->text(),
				   (unsigned int) params.at(3).toInt(), 2);
	break;
      }
    }
  
    //reset blocksize
    m_block_size=0;

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

    m_connect_to_host->setEnabled(true);
  }

  void Client::enableConnectButton()
  {
    m_connect_to_host->setEnabled((!m_network_session || m_network_session->isOpen()) &&
				  !m_host_lineedit->text().isEmpty() &&
				  !m_port_lineedit->text().isEmpty());

  }

  void Client::sessionOpened()
  {
    // Save the used configuration
    QNetworkConfiguration config = m_network_session->configuration();
    QString id;
    if (config.type() == QNetworkConfiguration::UserChoice)
      id = m_network_session->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
    else
      id = config.identifier();

    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
    settings.beginGroup(QLatin1String("QtNetwork"));
    settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
    settings.endGroup();

    m_status_label->setText(tr("Basic TCP messenger application developed by Santiago Pagola"));

    enableConnectButton();
  }

  void Client::keyPressEvent(QKeyEvent *event) { 
    if (event->key() == ENTER  &&
	m_message_lineedit->hasFocus()) {
      sendMessage();
    }
  }

  void Client::sendMessage() {
    debugInfo("Message slot called");
    if (!m_message_lineedit->toPlainText().isEmpty() &&
	m_message_lineedit->isEnabled()) {
      QString message = m_message_lineedit->toPlainText();
      QString dest_user = m_box->currentText();
      unsigned int message_id =
	m_window->newMessageFromUser("Me:" + message, true, dest_user);
      
      m_message_lineedit->clear();
      //Fill parameters to send
      QStringList params;
      params << message << dest_user << m_username_lineedit->text() << QString::number(message_id);
      //and send it to the server
      m_block_size = 0;
      m_transmission_socket->abort();
      m_transmission_socket->connectToHost(m_host_lineedit->text(),
					   m_port_lineedit->text().toInt());
      QString string_stream =
	m_protocol->constructStream_UE(params,
				       ProtocolStreamType_UE::UE_MESSAGE);
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
      //if typing timer is active, just stop it
      if (m_typing_timer->isActive()) {
	m_typing_timer->stop();
      }
      //typing label
      m_typing_label->setText("");
      m_typing_hold = false;
    }
  }

  void Client::nowOnline() {
    //start listening server
    unsigned cnt = 0;
    debugInfo("Attempting connection on port: " + QString::number(m_listening_port));
    while (!m_listen_socket->listen(QHostAddress(m_host_lineedit->text()), m_listening_port)) {
      if (m_listening_port <= 65535 && cnt < 10) {
	m_listening_port += 2;
	++cnt;
	debugInfo("Error: Trying next port: " + QString::number(m_listening_port));
      } else break;
    }
    if (m_listening_port < 65536) {
      debugInfo("Success: client will listen to server @ " +
		m_host_lineedit->text() + ":" + QString::number(m_listening_port));
      
    } else {
      LOG (ERROR) << "No available ports were found. Closing client...";
      close();
      return;
    }
    
    DLOG (INFO) << "Successfully connected to server. Sending user info";
    m_block_size = 0;
    m_transmission_socket->abort();
    m_transmission_socket->connectToHost(m_host_lineedit->text(),
					 m_port_lineedit->text().toInt());
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << (quint16)0;
    QStringList params;
    params << m_username_lineedit->text() << QString::number(m_listening_port);
    out << m_protocol->constructStream_UE(params,
					  ProtocolStreamType_UE::UE_REGISTER);
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
    m_message_lineedit->setEnabled(false);
    m_message_lineedit->setStyleSheet("background-color: #C0C0C0;");

    debugInfo("Removing my name from list");
    //and remove my name from list
    m_box->removeItem(1);
  }

  void Client::getOffline() {
    if (m_online) {
      //notify server
      DLOG (INFO) << "About to go offline";
      m_block_size = 0;
      m_transmission_socket->abort();
      m_transmission_socket->connectToHost(m_host_lineedit->text(),
					   m_port_lineedit->text().toInt());
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
      enableConnectButton();
      //and disable message field
      m_message_lineedit->setEnabled(false);
      m_message_lineedit->setStyleSheet("background-color: #C0C0C0;");
      //set icon
      QPixmap image;
      if (image.load("images/offline.png")) {
	m_status->setPixmap(image);
      } else m_status->setText("Error loading icon");
      enableServerFields(true);
      //and close listening socket
      m_listen_socket->close();
      debugInfo("Success: listening socket was closed!");
    } else {
      DLOG (INFO) << "User already offline";
    }
  }

  void Client::setData(bool debug_mode,
		       std::string username,
		       std::string ip,
		       std::string port,
		       bool save_output,
		       std::string path) {
    m_debug = debug_mode;
    m_save = save_output;
    if (m_debug) {
      m_checkbox->toggle();
    } else {
      DLOG (INFO) << "Debug mode is disabled";
    }
    if (m_save) {
      DLOG (INFO) << "Will save output on: " << path;
      m_log_file = new QFile(QString::fromStdString(path));
      if (!m_log_file->open(QIODevice::WriteOnly | QIODevice::Text)) {
	LOG (ERROR) << "ERROR: Could not open file to read: " << path;
      }
    }

    if (!QString::fromStdString(username).isEmpty())
      m_username_lineedit->setText(QString::fromStdString(username));
    if (!QString::fromStdString(ip).isEmpty())
      m_host_lineedit->setText(QString::fromStdString(ip));
    if (!QString::fromStdString(port).isEmpty())
      m_port_lineedit->setText(QString::fromStdString(port));
  }

  void Client::debugInfo(const QString& info) {
    if (m_debug) {
      DLOG (INFO) << info.toStdString();
    }
    if (m_save) {
      QTextStream out(m_log_file);
      out << info << "\n";
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
      m_host_lineedit->setStyleSheet("background-color: #E0E0E0;");
      m_host_lineedit->setEnabled(false);
      m_port_lineedit->setStyleSheet("background-color: #E0E0E0;");
      m_port_lineedit->setEnabled(false);
      m_username_lineedit->setStyleSheet("background-color: #E0E0E0;");
      m_username_lineedit->setEnabled(false);
    } else {
      m_host_lineedit->setStyleSheet("background-color: #FFFFFF;");
      m_host_lineedit->setEnabled(true);
      m_port_lineedit->setStyleSheet("background-color: #FFFFFF;");
      m_port_lineedit->setEnabled(true);
      m_username_lineedit->setStyleSheet("background-color: #FFFFFF;");
      m_username_lineedit->setEnabled(true);
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
      m_message_lineedit->setEnabled(false);
      m_message_lineedit->setStyleSheet("background-color: #C0C0C0;");
    } else {
      //disable message field
      m_message_lineedit->setEnabled(true);
      m_message_lineedit->setStyleSheet("background-color: #FFFFFF;");
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
      debugInfo("blocksize: " + QString::number(m_block_size));
      debugInfo("bytes available in client socket: " + QString::number(socket->bytesAvailable()));
      if (m_block_size == 0) {
	if (socket->bytesAvailable() < (int)sizeof(quint16))
	  return;
      
	in >> m_block_size;
      }
        
      if (socket->bytesAvailable() < m_block_size)
	return;
    
      QString message;
      in >> message;
    
      debugInfo("Message: [" + message + "]");

      ProtocolStreamType_Server type;
      QStringList params = m_protocol->parseStream_Server(type,message);
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
	  m_box->clear();
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
	  m_window->setMessageStatus(m_username_lineedit->text(),
				     (unsigned int) params.at(1).toInt(), 1);
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
	  enableConnectButton();
	  m_online = false;
	  m_block_size = 0;
	  break;
	}
      case SERVER_FWD_TO_DEST:
	{
	  QString dest = params.at(0);
	  QString from = params.at(1);
	  unsigned int message_id = (unsigned int) params.at(2).toInt();
	  //extra check
	  debugInfo("@@@ ack received, FROM: " + from + ", TO: " + dest + ", to MESSAGE ID: " +
		    QString::number(message_id));
	  if (dest == m_username_lineedit->text()) {
	    m_window->setMessageStatus(from, message_id, 2);
	    debugInfo("Message was successfully sent & seen!");
	  }
	  break;
	}
      case SERVER_FWD_TYPING:
	{
	  QString dest = params.at(0);
	  QString from = params.at(1);
	  QString status = params.at(2);
	  debugInfo("(" + dest + "," + from + "," + status + ")");
	  if (status == "1") {
	    m_typing_label->setText("(" + from + " is typing...)");
	  } else {
	    m_typing_label->setText("");
	  }
	  break;
	}
      default:
	{
	  debugInfo("New message was received");
	  unsigned int message_id =
	    m_window->newMessageFromUser(params.at(0), false, params.at(2));
	  m_typing_label->setText("");
	  debugInfo("Now sending ACK to user " + params.at(2) + ", to message ID: " + QString::number(message_id));
	  m_block_size = 0;
	  m_transmission_socket->abort();
	  m_transmission_socket->connectToHost(m_host_lineedit->text(),
					       m_port_lineedit->text().toInt());
	  QByteArray block;
	  QDataStream out(&block, QIODevice::WriteOnly);
	  out.setVersion(QDataStream::Qt_4_0);
	  out << (quint16)0;
	  QStringList ack_params;
	  ack_params << params.at(2) << m_username_lineedit->text() << QString::number(message_id);
	  debugInfo("ABOUT TO SEND ACK: PARAMETERS(" +
		    params.at(2) + "," +
		    m_username_lineedit->text() + "," +
		    QString::number(message_id) + ")");
	  out << m_protocol->constructStream_UE(ack_params,
						ProtocolStreamType_UE::UE_ACK);
	  out.device()->seek(0);
	  out << (quint16)(block.size() - sizeof(quint16));

	  m_transmission_socket->write(block);
	  if (!m_transmission_socket->waitForBytesWritten(2000)) {
	    LOG (ERROR) << "ERROR: transmission timeout.";
	  } else {
	    debugInfo("Success! ACK was sent to server");
	  }
	  m_transmission_socket->disconnectFromHost();
	  break;
	}
      }
    }
    m_block_size=0;
  }

  void Client::textChangedSlot() {
    m_typing_timer->start(1000);
    if (!m_typing_hold) {
      //then we send the typing information
      debugInfo("Text changing, going to send typing info");
      sendTypingInfo(true);
      //and lock the variable
      m_typing_hold = true;
    }
  }

  void Client::timeoutSlot() {
    if (m_typing_timer->isActive()) {
      debugInfo("Going to stop typing timer");
      m_typing_timer->stop();
      //then we sent the typing information
      debugInfo("Text stopped changing, going to send typing info");
      sendTypingInfo(false);
      //and unlock the variable
      m_typing_hold = false;
    }
  }

  void Client::sendTypingInfo(bool typing) {
    m_block_size = 0;
    m_transmission_socket->abort();
    m_transmission_socket->connectToHost(m_host_lineedit->text(),
					 m_port_lineedit->text().toInt());
    debugInfo("Attempting connection on: " + m_host_lineedit->text() + ":" + m_port_lineedit->text());
    if (!m_transmission_socket->waitForConnected(3000)) {
      LOG (ERROR) << "ERROR: Connection timeout.";
      m_block_size = 0;
      return;
    } else {
      int nr = (int) typing;
      QByteArray block;
      QDataStream out(&block, QIODevice::WriteOnly);
      out.setVersion(QDataStream::Qt_4_0);
      out << (quint16)0;
      QStringList typing_params;
      typing_params << m_box->currentText() << m_username_lineedit->text() << QString::number(nr);
      debugInfo("ABOUT TO SEND TYPING: PARAMETERS(" +
		m_box->currentText() + "," +
		m_username_lineedit->text() + "," +
		QString::number(nr) + ")");
    
      out << m_protocol->constructStream_UE(typing_params,
					    ProtocolStreamType_UE::UE_TYPING);
      out.device()->seek(0);
      out << (quint16)(block.size() - sizeof(quint16));
    
      m_transmission_socket->write(block);
      if (!m_transmission_socket->waitForBytesWritten(2000)) {
	LOG (ERROR) << "ERROR: transmission timeout.";
      } else {
	debugInfo("Success! Typing information was sent to server");
      }
      m_transmission_socket->disconnectFromHost();
    }
  }

}
