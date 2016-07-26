#include <QApplication>
#include "client.hpp"
#include <iostream>
#include <string>
#include <stdexcept>

using namespace tcp_messenger;

void displayHelp() {
  std::cout << "Usage: ./client <args>\n";
  std::cout << "Arg. list:\n";
  std::cout << "  -d, --debug           Enable debug mode (disabled by default)\n";
  std::cout << "  -f, --filename <path> Save output log to filename specified by <path>\n";
  std::cout << "  -h, --help            Prints this help\n";
  std::cout << "  --ip <ipaddr>         Server IP address\n";
  std::cout << "  -p, --port <port>     Server port\n";
  std::cout << "  -u, --username <name> Username to use\n";
  std::cout << "  -vN, --verbose=N      Verbosity level (0: None, ...)\n";
}

int main(int argc, char *argv[])
{
  bool debug_enabled = false;
  std::string username = "";
  std::string ip = "";
  std::string port = "";
  bool save_output = false;
  std::string path = "";
  int vlevel = 0;
  if (argc > 1) {
    for (unsigned i = 1; i < argc; ++i) {
      if (!strcmp(argv[i],"-d") ||
	  !strcmp(argv[i],"--debug")) {
	debug_enabled = true;
      } else if (!strcmp(argv[i],"-h") ||
		 !strcmp(argv[i],"--help")) {
	displayHelp();
	return 0;
      } else if (argv[i][1]=='v' ||
		 argv[i][2]=='v') {
	if (argv[i][1]=='v') {
	  std::string::size_type sz;
	  try {
	    vlevel = (int) argv[i][2] - 48;
	  } catch(const std::invalid_argument &ia) {
	    std::cerr << "ERROR: level must be an integer.\n";
	    displayHelp();
	    return -1;
	  }
	} else {
	  for (unsigned j = 0; j < strlen(argv[i]); ++j) {
	    if (argv[i][j] == '=' &&
		j != strlen(argv[i]) - 1) {
	      std::string::size_type sz;
	      try {
		vlevel = (int) argv[i][j+1] - 48;
	      } catch(const std::invalid_argument &ia) {
		std::cerr << "ERROR: level must be an integer.\n";
		displayHelp();
		return -1;
	      }
	    }
	  }
	}
      } else if (!strcmp(argv[i],"-u") ||
		 !strcmp(argv[i],"--username")) {
	if (i != argc-1) {
	  username = argv[i+1];
	} else {
	  displayHelp();
	  return -1;
	}
	++i;
      } else if (!strcmp(argv[i],"-p") ||
		 !strcmp(argv[i],"--port")) {
	if (i != argc-1) {
	  port = argv[i+1];
	  std::string::size_type sz;
	  int portnr;
	  try {
	    portnr = std::stoi(port,&sz);
	  } catch(const std::invalid_argument& ia) {
	    std::cerr << "ERROR: port must be an integer.\n";
	    displayHelp();
	    return -1;
	  }
	  if (0 >= portnr || portnr > 65535) {
	    std::cerr << "ERROR: port must be an integer between (1-65535).\n";
	    displayHelp();
	    return -1;
	  }
	  ++i;
	} else {
	  displayHelp();
	  return -1;
	}
      } else if (!strcmp(argv[i],"--ip")) {
	if (i != argc-1) {
	  ip = argv[i+1];
	} else {
	  displayHelp();
	  return -1;
	}
	++i;
      } else if (!strcmp(argv[i],"-f") ||
		 !strcmp(argv[i],"--filename")) {
	if (i != argc-1) {
	  if (argv[i+1][0] == '-') {
	    std::cerr << "ERROR: unrecognized filename: " << argv[i+1] << "\n";
	    displayHelp();
	    return -1;
	  } else {
	    path = argv[i+1];
	    save_output = true;
	  }
	} else {
	  displayHelp();
	  return -1;
	}
	++i;
      } else {
	std::cerr << "ERROR: unrecognized option : " << argv[i] << std::endl;
	displayHelp();
	return -1;
      }
    }
  }
  QApplication app(argc, argv);
  Client client;
  client.setData(debug_enabled, vlevel, username, ip, port, save_output, path);
  
#ifdef Q_OS_SYMBIAN
  // Make application better looking and more usable on small screen
  client.showMaximized();
#else
  client.show();
#endif
  return client.exec();
}
