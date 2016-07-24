#include <glog/logging.h>

#include "chat.hpp"
#include "chatcontainer.hpp"

namespace tcp_messenger {

  ChatContainer::ChatContainer(QWidget *parent) {
    setObjectName("ChatWindowWrapper");
    setStyleSheet("background-color: white;");
    m_conver_stack = new QStackedWidget(this);
    m_main_layout = new QVBoxLayout;
    m_main_layout->setAlignment(Qt::AlignTop);
    m_main_layout->addWidget(m_conver_stack);
    setLayout(m_main_layout);
  }

  ChatContainer::~ChatContainer() {}

  bool ChatContainer::addUser(const QString& from) {
    //first, register user if not in list
    if (!m_current_users.contains(from)) {
      DLOG (INFO) << "Registering new user: " << from.toStdString();
      m_current_users << from;
      DLOG (INFO) << "Adding chat window for new user at index: " <<
	m_conver_stack->addWidget(new Chat(from));
      DLOG (INFO) << "Success! New user registered";
      return true;
    }
    return false;
  }

  unsigned int ChatContainer::newMessageFromUser(const QString& message, bool self, const QString& from) {
    //check if registered, just in case
    if (!m_current_users.contains(from)) {
      LOG (ERROR) << "Error: user " << from.toStdString() << " was not found. Returning...";
      return 0;
    } else {
      DLOG (INFO) << "Adding message";
      return (qobject_cast<Chat*>(m_conver_stack->widget(m_current_users.indexOf(from)))->addMessage(message,self,from));
    }
  }
  
  void ChatContainer::setMessageStatus(const QString& from, unsigned int message_id, int status) {
    int index = getIndexOfUser(from);
    if (index != -1)
      qobject_cast<Chat*>(m_conver_stack->widget(index))->setMessageStatus(message_id, status);
    else
      LOG (ERROR) << "Error: user " << from.toStdString() << " was not found";
  }
  
  void ChatContainer::currentWindowChangedSlot(const QString& user) {
    DLOG (INFO) << "Changing conversation...";
    m_conver_stack->setCurrentIndex(m_current_users.indexOf(user));
  }

  int ChatContainer::getIndexOfUser(const QString& user) {
    for (unsigned i = 0; i < m_current_users.size(); ++i) {
      if (m_current_users.at(i) == user) {
	return i;
      }
    }
    return -1;
  }
  
}
