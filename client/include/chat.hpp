#ifndef CHAT_HPP_
#define CHAT_HPP_

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QTimer>
#include <QScrollBar>

namespace tcp_messenger {

  struct MessageData {
    QString text;
    bool self;
    int status;
    unsigned int id;
  };
  
  class Chat: public QWidget {
    Q_OBJECT
  public:
    Chat(const QString& name, QWidget *parent=0);
    ~Chat();
    unsigned int addMessage(const QString& text, bool self, const QString& from);
    void setMessageStatus(unsigned int message_id, int status);
  private slots:
    void sliderMovedSlot(int value);
  private:
    int getIndexOfMessageID(unsigned int message_id);
  private:
    QHBoxLayout *m_principal_layout;
    QVBoxLayout *m_main_layout;
    QString m_name;
    QScrollBar *m_side_bar;
    QList<MessageData> m_message_buffer;
  };
  
}

#endif
