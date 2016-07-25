#include <glog/logging.h>
#include "message.hpp"

namespace tcp_messenger {
  
  Message::Message(QWidget* parent) : m_self(false) {
    resize(500, 38);

    setObjectName("Message");
    setObjectName("#Message {border-bottom: 2px solid black; background-color: white;}");
    
    m_main_layout = new QHBoxLayout;
    
    m_text_label = new QLabel();
    m_text_label->setContentsMargins(5,0,30,0);
    m_text_label->setWordWrap(true);

    QPixmap image;
    m_status_label = new QLabel();
    if (image.load("images/clock.png"))
      m_status_label->setPixmap(image);

    m_status_label->setContentsMargins(11,0,11,20);
    
    m_main_layout->addWidget(m_text_label);
    m_main_layout->addWidget(m_status_label);

    //ack timer
    m_ack_timer = new QTimer(this);
    connect(m_ack_timer, SIGNAL(timeout()), this, SLOT(timerTimedOut()));

    m_status = SENDING;

    m_message_id = rand() % 1000 + 1;
    
    setLayout(m_main_layout);
    setContentsMargins(0,0,0,0);
    show();
  }
  
  Message::Message(const QString& text, bool self, QWidget* parent) {
    resize(500, 38);
        
    m_main_layout = new QHBoxLayout;

    setObjectName("Message");
    setStyleSheet("#Message {border-bottom : 2px solid #347F91;}");
    
    m_text_label = new QLabel(text);
    m_text_label->setContentsMargins(5,5,30,15);
    m_text_label->setWordWrap(true);
    
    QPixmap image;
    m_status_label = new QLabel();
    if (image.load("images/clock.png"))
      m_status_label->setPixmap(image);

    m_status_label->setContentsMargins(11,5,11,0);

    m_main_layout->addWidget(m_text_label);
    m_main_layout->addWidget(m_status_label);

    fromMe(self);

    m_ack_timer = new QTimer(this);
    connect(m_ack_timer, SIGNAL(timeout()), this, SLOT(timerTimedOut()));
    
    //start timer
    if (self) {
      DLOG (INFO) << "Timer starting..";
      m_ack_timer->start(10 * 1000);
    }
    
    m_status = SENDING;
    m_message_id = rand() % 1000 + 1;
    
    setLayout(m_main_layout);

    changeSize(text.size() / 45 + 1);

    setContentsMargins(0,0,0,0);
    show();
  }

  Message::~Message() {}

  void Message::changeSize(int nr) {
    DLOG (INFO) << "Resizing message box";
    resize(this->width(), this->height() * nr);
    repaint();
  }

  void Message::fromMe(bool self) {
    m_self = self;
    if (self) {
      m_status_label->show();
      m_text_label->show();
      m_status_label->setAlignment(Qt::AlignTop | Qt::AlignRight);
      m_text_label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    } else {
      m_status_label->hide();
      m_text_label->show();
      m_text_label->setAlignment(Qt::AlignTop | Qt::AlignRight);
    }
    DLOG (INFO) << "From self set: " << self;
  }

  void Message::newMessage(QString text) {
    m_text_label->setText(text);
    changeSize(text.size() / 45 + 1);
    if (m_self)
      m_ack_timer->start(10 * 1000);
    DLOG (INFO) << "New message set: " << text.toStdString();
  }

  void Message::timerTimedOut() {
    DLOG (INFO) << "Timeout";
    if (m_status == SENDING) {
      setMessageStatus(LOST);
      m_ack_timer->stop();
    }
  }

  unsigned int Message::getMessageID() {
    return m_message_id;
  }

  void Message::setMessageID(unsigned int id) {
    m_message_id = id;
  }

  QString Message::getText() {
    return m_text_label->text();
  }

  void Message::setSender(bool self) {
    fromMe(self);
  }

  void Message::setMessageStatus(int status) {
    if (!m_self)
      return;
    else
      m_status_label->setVisible(true);
    
    DLOG (INFO) << "Setting status: " << status;
    m_status = (STATUS) status;
    QPixmap image;
    switch (m_status) {
    case SENDING:
      if (image.load("images/clock.png"))
	m_status_label->setPixmap(image);
      break;
    case SENT:
      if (image.load("images/check.png"))
	m_status_label->setPixmap(image);
      if (m_ack_timer->isActive()) {
	m_ack_timer->stop();
      }
      break;
    case RECEIVED:
      if (image.load("images/seen.png"))
	m_status_label->setPixmap(image);
      if (m_ack_timer->isActive()) {
	m_ack_timer->stop();
      }
      break;
    case LOST:
      if (image.load("images/warning.png"))
	m_status_label->setPixmap(image);
      break;
    default:
      DLOG (INFO) << "Something strange happened";
      break;
    }
  }

  bool Message::getSelf() {
    return m_self;
  }

  STATUS Message::getStatus() {
    return m_status;
  }
  
  void Message::operator=(Message const& next_message) {
    newMessage(next_message.m_text_label->text());
    fromMe(next_message.m_self);
    setMessageStatus(next_message.m_status);
    DLOG (INFO) << "OLD message ID: " << m_message_id;
    m_message_id = next_message.m_message_id;
    DLOG (INFO) << "NEW message ID: " << m_message_id;
  }
}
