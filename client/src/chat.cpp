#include <glog/logging.h>

#include "message.hpp"
#include "chat.hpp"

namespace tcp_messenger {

  static const int MAX_MESSAGES = 8;
  
  Chat::Chat(const QString& name, QWidget *parent) : m_name(name){
    setObjectName("ChatWindow");
    setStyleSheet("#ChatWindow {background-color: white;}");

    m_side_bar = new QScrollBar;
    m_side_bar->setMinimum(0);
    m_side_bar->setMaximum(MAX_MESSAGES - 1);
    m_side_bar->setSliderPosition(m_side_bar->maximum());
    m_side_bar->hide();
    connect(m_side_bar, SIGNAL(sliderMoved(int)), this, SLOT(sliderMovedSlot(int)));
    
    m_main_layout = new QVBoxLayout;
    m_principal_layout = new QHBoxLayout;
    
    m_main_layout->setSpacing(0);
    m_main_layout->setAlignment(Qt::AlignTop);

    m_principal_layout->addLayout(m_main_layout);
    m_principal_layout->addWidget(m_side_bar);
    
    setLayout(m_principal_layout);
    show();
  }

  Chat::~Chat() {}

  unsigned int Chat::addMessage(const QString& text, bool self, const QString& from) {
    if (m_main_layout->count() < MAX_MESSAGES) {
      if (!self)
	m_main_layout->addWidget(new Message(from + ":" + text,self));
      else
	m_main_layout->addWidget(new Message(text,self));

     
    } else {
      Message *current, *next;
      DLOG (INFO) << "@@@@@@@@@@@@@@@@@@@@@@@@@@ COPYING MESSAGES @@@@@@@@@@@@@@@@";
      for (unsigned i = 0; i < MAX_MESSAGES - 1; ++i) {
	current = qobject_cast<Message*>(m_main_layout->itemAt(i)->widget());
	next = qobject_cast<Message*>(m_main_layout->itemAt(i+1)->widget());
	DLOG (INFO) << "Copying elem " << i+1 << "/" << m_main_layout->count()
		    << " to elem " << i << "/" << m_main_layout->count();

	*current = *next;
      }
      DLOG (INFO) << "@@@@@@@@@@@@@@@@@@@@@@@@@@ CASTING LAST  @@@@@@@@@@@@@@@@";
      Message *last = qobject_cast<Message*>(m_main_layout->itemAt(MAX_MESSAGES - 1)->widget());
      unsigned int id = rand() & 1000 + 1;
      DLOG (INFO) << "Casted last, going to set message ID: " << id;
      last->newMessage(text);
      last->setSender(self);
      last->setMessageID(id);
    }
    unsigned int message_id =
      qobject_cast<Message*>(m_main_layout->itemAt(m_main_layout->count() -1 )->widget())->getMessageID();
    //and append it to buffer
    MessageData newmessage;
    newmessage.text = text;
    newmessage.self = self;
    newmessage.status = 0;
    newmessage.id = message_id;
    m_message_buffer.append(newmessage);
    DLOG (INFO) << "Nr. message on buffer: " << m_message_buffer.size();
    m_side_bar->setMaximum(m_message_buffer.size());
    m_side_bar->setSliderPosition(m_side_bar->maximum());
    m_side_bar->show();
    return message_id;
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

  void Chat::sliderMovedSlot(int value) {
    DLOG (INFO) << "New position: " << value;
  } 
 
}
