#include <QApplication>
#include "client.h"
#include <iostream>
#include <cstring>
#include <string>

void displayHelp() {
  std::cout << "Usage: ./client <args>\n";
  std::cout << "Arg. list:\n";
  std::cout << "-d, --debug Enable debug mode (disabled by default)\n";
  std::cout << "-u, --username <name> Username to use\n";
}

int main(int argc, char *argv[])
{
  bool debug_enabled = false;
  std::string username = "";
  
  if (argc > 1) {
    for (unsigned i = 1; i < argc; ++i) {
      if (!strcmp(argv[i],"-d") ||
	  !strcmp(argv[i],"--debug")) {
	debug_enabled = true;
      } else if (!strcmp(argv[i],"-u") ||
		 !strcmp(argv[i],"--username")
		 ) {
	if (i != argc-1) {
	  username = argv[i+1];
	} else {
	  displayHelp();
	  return -1;
	}
      }
    }
  }
  QApplication app(argc, argv);
  Client client;
  client.setData(debug_enabled, username);
  
#ifdef Q_OS_SYMBIAN
  // Make application better looking and more usable on small screen
  client.showMaximized();
#else
  client.show();
#endif
  return client.exec();
}
