#ifndef MESSAGE_HPP_
#define MESSAGE_HPP_

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QTimer>

namespace tcp_messenger {

  enum STATUS {
    SENDING,
    SENT,
    RECEIVED,
    LOST
  };
  
  class Message: public QWidget {
    Q_OBJECT
  public:
    Message(QWidget *parent=0);
    Message(const QString& text, bool self, QWidget *parent=0);
    ~Message();
    //will change height
    void changeSize(int nr);
    //will change alingments
    void fromMe(bool self);
    //sets message
    void newMessage(const QString& text);
    void setMessageStatus(int status);
    void setSender(bool self);
    unsigned int getMessageID();
    void setMessageID(unsigned int id);
    QString getText();
    bool getSelf();
    STATUS getStatus();
    void setStatusIcons();
  signals:
    void statusChanged(STATUS status);
  private slots:
    void timerTimedOut();
    void statusChangedSlot(STATUS status);
  public:
    void operator=(Message const& next_message);
  private:
    QHBoxLayout *m_main_layout;
    QLabel *m_text_label;
    QLabel *m_status_label;
    bool m_self;
    QTimer *m_ack_timer;
    STATUS m_status;
    unsigned int m_message_id;
  };
}

#endif
