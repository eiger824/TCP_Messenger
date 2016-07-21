#ifndef SERVER_H
#define SERVER_H

#include <QDialog>
#include <QTimer>
#include <iostream>

class QLabel;
class QPushButton;
class QTcpServer;
class QTextEdit;
class QVBoxLayout;
class QNetworkSession;

namespace tcp_messenger {

  struct UE {
    QString name;
    QString ip;
    quint16 rx_port;
  };

  class Server : public QDialog
  {
    Q_OBJECT

  public:
    Server(QWidget *parent = 0);
    void setDebugMode(bool debug_mode);
  
    private slots:
      void sessionOpened();
      void acceptUser();
  private:
      void unregisterUser();
      QString filterMessage(QString& dest, QString data);
      void debugInfo(const QString& info);
      bool isThisIpRegistered(const QString& ip);
      bool isThisNameRegistered(const QString& name);
      int getIndexOfUEName(const QString& name);
      int getIndexOfUEIp(const QString& ip);
  private:
      QVBoxLayout *mainLayout;
      QLabel *statusLabel;
      QTextEdit *logLabel;
      QTextEdit *onlineUsers;
      QPushButton *quitButton;
      QTcpServer *tcpServer;
      QNetworkSession *networkSession;
      QList<UE> m_online_users;
      quint16 blockSize;
      bool m_debug;
  };

  std::ostream & operator<<( std::ostream &os, const UE& user);
  bool operator==(const UE& u1, const UE& u2);

}
 
#endif
