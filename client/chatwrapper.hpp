#ifndef CHAT_WRAPPER_HPP_
#define CHAT_WRAPPER_HPP_

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QTimer>

namespace tcp_messenger {
  
  class ChatWrapper: public QWidget {
    Q_OBJECT
  public:
    ChatWrapper(QWidget *parent=0);
    ~ChatWrapper();
    bool addUser(const QString& from);
    void newMessageFromUser(const QString& message, bool self, const QString& from);
    void setMessageStatus(int status);
  private slots:
    void currentWindowChangedSlot(const QString& user);
  private:
    QVBoxLayout *m_main_layout;
    QStackedWidget *m_conver_stack;
    QStringList m_current_users;
  };
  
}

#endif
