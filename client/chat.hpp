#ifndef CHAT_HPP_
#define CHAT_HPP_

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>

namespace tcp_messenger {
  
  class Chat: public QWidget {
    Q_OBJECT
  public:
    Chat(QWidget *parent=0);
    ~Chat();
    void addMessage(const QString& text, bool self);
  private:
    QVBoxLayout *m_main_layout;
  };
  
}

#endif
