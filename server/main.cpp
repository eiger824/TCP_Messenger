#include <QApplication>
#include <QtCore>

#include <stdlib.h>

#include "server.h"

void displayHelp() {
  std::cout << "Usage: ./server <args>\n";
  std::cout << "Arg. list:\n";
  std::cout << "-d, --debug Enable debug mode (disabled by default)\n";
}

int main(int argc, char *argv[])
{
  bool debug_enabled = false;

  if (argc > 1) {
    if (!strcmp(argv[1], "-d") ||
	!strcmp(argv[1], "--debug")) {
      debug_enabled = true;
    }
  }
  
  QApplication app(argc, argv);
  Server server;
  server.setDebugMode(debug_enabled);
#ifdef Q_OS_SYMBIAN
  server.showMaximized();
#else
  server.show();
#endif
  qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));
  return server.exec();
}
