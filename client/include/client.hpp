#ifndef CLIENT_H
#define CLIENT_H

#include <QDialog>
#include <QTcpSocket>
#include <QKeyEvent>
#include <QMap>
#include <string>

#include "chatwrapper.hpp"
#include "protocol.hpp"

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
class QAbstractSocket;
class QVBoxLayout;

namespace tcp_messenger {

  class Client : public QDialog {
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
    void enableConnectButton();
    void sessionOpened();
    void nowOnline();
    void getOnline();
    void nowOffline();
    void getOffline();
    void changeDebugMode(bool toggled);
    void currentIndexChangedSlot(int index);
    void sendMessage();
    void newServerConnection();
  signals:
    void currentWindowChanged(const QString& user);
  private:
    void keyPressEvent(QKeyEvent *event);
    void debugInfo(const QString& info);
    void enableServerFields(bool enabled);
  private:
    QLabel *m_hostname_label;
    QLabel *m_port_label;
    QLabel *m_message_label;
    QLabel *m_username;
    QLabel *m_connection_status;
    QLabel *m_status;
    QLineEdit *m_host_lineedit;
    QLineEdit *m_port_lineedit;
    QTextEdit *m_message_lineedit;
    QLineEdit *m_username_lineedit;
    QLabel *m_status_label;
    QPushButton *m_connect_to_host;
    QPushButton *m_disconnect_from_host;
    QDialogButtonBox *m_button_box;
    QCheckBox *m_checkbox;
    QTcpSocket *m_connection_socket;
    QTcpSocket *m_transmission_socket;
    QTcpServer *m_listen_socket;
    quint16 m_block_size;
    bool m_online;
    bool m_debug;
    QNetworkSession *m_network_session;
    QComboBox *m_box;
    ChatWrapper *m_window;
    Protocol *m_protocol;
    quint16 m_listening_port;
  };
}
#endif
