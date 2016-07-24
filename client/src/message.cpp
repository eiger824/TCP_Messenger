#include <glog/logging.h>
#include "message.hpp"

namespace tcp_messenger {
  
  Message::Message(QWidget* parent) : m_self(false) {
    setFixedSize(500, 38);

    setObjectName("Message");
    setObjectName("#Message {border-bottom: 2px solid black; background-color: white;}");
    
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

    m_message_id = rand() % 1000 + 1;
    
    setLayout(m_main_layout);
    show();
  }
  
  Message::Message(const QString& text, bool self, QWidget* parent) {
    setFixedSize(500, 38);
        
    m_main_layout = new QHBoxLayout;

    setObjectName("Message");
    setObjectName("#Message {background-color: white;}");
    
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
    m_message_id = rand() % 1000 + 1;
    
    setLayout(m_main_layout);

    changeSize(text.size() / 45 + 1);
    
    show();
  }

  Message::~Message() {}

  void Message::changeSize(int nr) {
    DLOG (INFO) << "Resizing message box";
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
    DLOG (INFO) << "From self set: " << self;
  }

  void Message::newMessage(const QString& text) {
    m_text_label->setText(text);
    changeSize(text.size() / 45 + 1);
    if (m_self)
      m_ack_timer->start(10 * 1000);
    DLOG (INFO) << "New message set: " << text.toStdString();
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

  unsigned int Message::getMessageID() {
    return m_message_id;
  }

  void Message::setMessageID(unsigned int id) {
    m_message_id = id;
  }

  QString Message::getText() {
    return m_text_label->text();
  }

  void Message::setStatusIcons() {
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
  
  void Message::statusChangedSlot(STATUS status) {
    setStatusIcons();
    DLOG (INFO) << "Status updated to " << status;
  }

  void Message::setSender(bool self) {
    fromMe(self);
  }

  void Message::setMessageStatus(int status) {
    DLOG (INFO) << "Setting status: " << status;
    if (status == 0) {
      m_status = SENDING;
      emit statusChanged(SENDING);
    } else if (status == 1) {
      m_status = SENT;
      emit statusChanged(SENT);
    } else if (status == 2) {
      m_status = RECEIVED;
      emit statusChanged(RECEIVED);
    } else if (status == 3) {
      m_status = LOST;
      emit statusChanged(LOST);
    }
  }

  bool Message::getSelf() {
    return m_self;
  }

  STATUS Message::getStatus() {
    return m_status;
  }
  
  void Message::operator=(Message const& next_message) {
    m_text_label->setText(next_message.m_text_label->text());
    m_message_id = next_message.m_message_id;
    m_status = next_message.m_status;
    //statusChangedSlot(next_message.m_status);
    fromMe(next_message.m_self);
    setStatusIcons();
  }
}
