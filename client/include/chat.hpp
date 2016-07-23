#ifndef CHAT_HPP_
#define CHAT_HPP_

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QTimer>

namespace tcp_messenger {
  
  class Chat: public QWidget {
    Q_OBJECT
  public:
    Chat(const QString& name, QWidget *parent=0);
    ~Chat();
    unsigned int addMessage(const QString& text, bool self, const QString& from);
    void setMessageStatus(unsigned int message_id, int status);
    void setTypingNotifier(bool status);
  private:
    int getIndexOfMessageID(unsigned int message_id);
  private:
    QVBoxLayout *m_main_layout;
    QString m_name;
  };
  
}

#endif
