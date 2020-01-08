#include <string>

struct Command {
    unsigned valid;
    int port;
    std::string payload;
};

Command parseCommand(std::string content) {
    size_t pos = content.find(":");
      if (pos >= 0 && pos != std::string::npos) {
        std::string portString = content.substr(0, pos);
        int port = std::stoi(portString);
        std::string payload = content.substr(pos+1, content.size());
        return {1, port, payload};
      }
    return {0, 0, ""};
}

void printCommand(Command c) {
    printf("[command={%u, %i, %s}]\n", c.valid, c.port, c.payload.c_str());
}

int test_command(void)
{
    Command c1 = parseCommand("5:deadc0de");
    if (c1.valid != 1) {
        printf("c1.valid != 1");
        return 1;
    }
    if (c1.port != 5) {
        printf("c1.port != 5");
        return 1;
    }
    if (c1.payload.compare("deadc0de") != 0) {
        printf("c1.payload.compare(\"deadc0de\") != 0");
        return 1;
    }
    printCommand(c1);

    Command c2 = parseCommand("port_fehlt");
    if (c2.valid != 0) {
        printf("c2.valid != 0");
        return 1;
    }
    printCommand(c2);
    printf("test_command OK\n");
    return 0;
}