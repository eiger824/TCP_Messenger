#include <glog/logging.h>
#include "message.hpp"

namespace tcp_messenger {
  
  Message::Message(QWidget* parent) : m_self(false) {
    setFixedSize(530, 38);

    setObjectName("Message");
    setStyleSheet("#Message {border: 2px solid black;}");
    
    m_main_layout = new QHBoxLayout;
    
    m_text_label = new QLabel();
    m_text_label->setContentsMargins(5,0,30,15);
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

    //local signal-slot connection
    connect(this, SIGNAL(statusChanged(STATUS)), this, SLOT(statusChangedSlot(STATUS)));

    m_status = SENDING;
    
    setLayout(m_main_layout);
    show();
  }
  
  Message::Message(const QString& text, bool self, QWidget* parent) {
    setFixedSize(530, 38);
        
    m_main_layout = new QHBoxLayout;

    setObjectName("Message");
    this->setStyleSheet("#Message {border: 2px solid black;}");
    
    m_text_label = new QLabel(text);
    m_text_label->setContentsMargins(5,5,30,15);
    m_text_label->setWordWrap(true);
    
    QPixmap image;
    m_status_label = new QLabel();
    if (image.load("images/clock.png"))
      m_status_label->setPixmap(image);

    m_status_label->setContentsMargins(11,5,11,20);

    m_main_layout->addWidget(m_text_label);
    m_main_layout->addWidget(m_status_label);

    fromMe(self);

    //start timer
    if (self) {
      m_ack_timer = new QTimer(this);
      connect(m_ack_timer, SIGNAL(timeout()), this, SLOT(timerTimedOut()));
      DLOG (INFO) << "Timer starting..";
      m_ack_timer->start(10 * 1000);
    }

    //local signal-slot connection
    connect(this, SIGNAL(statusChanged(STATUS)), this, SLOT(statusChangedSlot(STATUS)));
    
    m_status = SENDING;
    
    setLayout(m_main_layout);
    
    show();
  }

  Message::~Message() {}

  void Message::changeSize(int nr) {
    resize(this->width(), this->width() * nr);
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
  }

  void Message::newMessage(const QString& text) {
    m_text_label->setText(text);
    if (m_self)
      m_ack_timer->start(10 * 1000);
  }

  void Message::timerTimedOut() {
    DLOG (INFO) << "Timeout!!";
    if (m_status == SENDING) {
      m_status = LOST;
      DLOG (INFO) << "Emitting signal";
      emit statusChanged(LOST);
      m_ack_timer->stop();
    }
  }

  void Message::statusChangedSlot(STATUS status) {
    QPixmap image;
    switch (status) {
    case SENDING:
      if (image.load("images/clock.png"))
	m_status_label->setPixmap(image);
      break;
    case RECEIVED:
      if (image.load("images/check.png"))
	m_status_label->setPixmap(image);
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

  void Message::setMessageStatus(int status) {
    if (status == 0) {
      m_status = SENDING;
      emit statusChanged(SENDING);
    } else if (status ==1) {
      m_status = RECEIVED;
      emit statusChanged(RECEIVED);
    } else {
      m_status = LOST;
      emit statusChanged(LOST);
    }
  }
}
