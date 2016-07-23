#include <glog/logging.h>

#include "message.hpp"
#include "chat.hpp"

namespace tcp_messenger {

  static const int MAX_MESSAGES = 9;
  
  Chat::Chat(const QString& name, QWidget *parent) : m_name(name){
    setObjectName("ChatWindow");
    setStyleSheet("#ChatWindow {background-color: white;}");
    m_main_layout = new QVBoxLayout;
    m_main_layout->setSpacing(0);
    m_main_layout->setAlignment(Qt::AlignTop);
    setLayout(m_main_layout);
    show();
  }

  Chat::~Chat() {}

  unsigned int Chat::addMessage(const QString& text, bool self, const QString& from) {
    //first check there are no pending "typing" messages
    if (m_main_layout->count() > 0) {
      Message *last =
	qobject_cast<Message*>(m_main_layout->itemAt(m_main_layout->count() - 1)->widget());
      if (last->getText().contains("Typing", Qt::CaseInsensitive)) {
	//then remove the previous
	m_main_layout->removeWidget(last);
	delete last;
      }
    }
    if (m_main_layout->count() < MAX_MESSAGES - 1) {
      if (!self)
	m_main_layout->addWidget(new Message(from + ":" + text,self));
      else
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
    return (qobject_cast<Message*>(m_main_layout->itemAt(m_main_layout->count() -1 )->widget())->getMessageID());
  }

  void Chat::setMessageStatus(unsigned int message_id, int status) {
    int index = getIndexOfMessageID(message_id);
    if (index != -1) {
      qobject_cast<Message*>(m_main_layout->itemAt(index)->widget())->setMessageStatus(status);
    } else {
      LOG (ERROR) << "Message ID " << message_id << " was not found on conversation. Returning.";
    }
  }

  int Chat::getIndexOfMessageID(unsigned int message_id) {
    for (unsigned i = 0; i < m_main_layout->count(); ++i) {
      if (qobject_cast<Message*>(m_main_layout->itemAt(i)->widget())->getMessageID() == message_id) {
	return i;
      }
    }
    return -1;
  }
 
}
