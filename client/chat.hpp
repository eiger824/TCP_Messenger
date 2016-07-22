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
    void addMessage(const QString& text, bool self, const QString& from);
    void setMessageStatus(int status);
  private:
    QVBoxLayout *m_main_layout;
    QString m_name;
  };
  
}

#endif
