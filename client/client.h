#ifndef CLIENT_H
#define CLIENT_H

#include <QDialog>
#include <QTcpSocket>
#include <QKeyEvent>
#include <string>

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QTextEdit;
class QPushButton;
class QTcpSocket;
class QNetworkSession;

static const int ENTER = 16777220;

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

 private:
    void keyPressEvent(QKeyEvent *event);
    QString filterMessage(QString& dest, QString data);
    void debugInfo(const QString& info);
    
 private:
    QLabel *hostLabel;
    QLabel *portLabel;
    QLabel *messageLabel;
    QLabel *m_username;
    QTextEdit *m_chat;
    QTextEdit *onlineUsers;
    QLineEdit *hostLineEdit;
    QLineEdit *portLineEdit;
    QTextEdit *messageLineEdit;
    QLineEdit *usernameLineEdit;
    QLabel *statusLabel;
    QPushButton *getFortuneButton;
    QPushButton *quitButton;
    QDialogButtonBox *buttonBox;

    QTcpSocket *m_connection_socket;
    QTcpSocket *m_transmission_socket;
    QString currentFortune;
    quint16 blockSize;
    bool m_online;
    bool m_debug;
    QNetworkSession *networkSession;
};

#endif
