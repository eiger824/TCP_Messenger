#ifndef CLIENT_H
#define CLIENT_H

#include <QDialog>
#include <QTcpSocket>
#include <QKeyEvent>
#include <QMap>
#include <string>

#include "definitions.hpp"

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QTextEdit;
class QPushButton;
class QTcpSocket;
class QTcpServer;
class QNetworkSession;
class QCheckBox;
class QPixmap;
class QComboBox;

namespace tcp_messenger {

  class Client : public QDialog
  {
    Q_OBJECT

  public:
    Client(QWidget *parent = 0);
    void setData(bool debug_mode,
		 std::string username,
		 std::string ip,
		 std::string port);

    private slots:
      void dataReceived();
      void displayError(QAbstractSocket::SocketError socketError);
      void enableGetFortuneButton();
      void sessionOpened();
      void nowOnline();
      void getOnline();
      void nowOffline();
      void getOffline();
      void changeDebugMode(bool toggled);
      void currentIndexChangedSlot(int index);
      void sendMessage();
      void newServerConnection();

  private:
      void keyPressEvent(QKeyEvent *event);
      QString filterMessage(QString& dest, QString& from, quint16& id, QString data);
      void debugInfo(const QString& info);
      void enableServerFields(bool enabled);
  private:
      QLabel *hostLabel;
      QLabel *portLabel;
      QLabel *messageLabel;
      QLabel *m_username;
      QLabel *m_connection_status;
      QLabel *m_status;
      QTextEdit *m_chat;
      QLineEdit *hostLineEdit;
      QLineEdit *portLineEdit;
      QTextEdit *messageLineEdit;
      QLineEdit *usernameLineEdit;
      QLabel *statusLabel;
      QPushButton *getFortuneButton;
      QPushButton *quitButton;
      QDialogButtonBox *buttonBox;
      QCheckBox *checkbox;
      QTcpSocket *m_connection_socket;
      QTcpSocket *m_transmission_socket;
      QTcpServer *m_listen_socket;
      QString currentFortune;
      quint16 blockSize;
      bool m_online;
      bool m_debug;
      QNetworkSession *networkSession;
      QComboBox *m_box;
      QMap<QString,QString>m_convers;
  };
}
#endif
