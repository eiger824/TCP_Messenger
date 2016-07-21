#include <glog/logging.h>

#include "message.hpp"
#include "chat.hpp"

namespace tcp_messenger {

  static const int MAX_MESSAGES = 9;
  
  Chat::Chat(QWidget *parent) {

    setFixedSize(this->width() - 80, this->height() - 100);
    setObjectName("Chat");
    setStyleSheet("#Chat {border: 2px solid black; background-color: white;}");
    
    m_main_layout = new QVBoxLayout;
    m_main_layout->setSpacing(0);
    m_main_layout->setAlignment(Qt::AlignTop);
    setLayout(m_main_layout);
  }

  Chat::~Chat() {}

  void Chat::addMessage(const QString& text, bool self) {
    if (m_main_layout->count() < MAX_MESSAGES - 1) {
      m_main_layout->addWidget(new Message(text,self));
    } else {
      Message *current, *next;
      for (unsigned i = 0; i < MAX_MESSAGES - 1; ++i) {
	current = qobject_cast<Message*>(m_main_layout->itemAt(i)->widget());
	next = qobject_cast<Message*>(m_main_layout->itemAt(i+1)->widget());

	*current = *next;
      }
      Message *last = qobject_cast<Message*>(m_main_layout->itemAt(MAX_MESSAGES - 1)->widget());
      last->newMessage(text);
      last->setSender(self);
    }
  }
  
}
