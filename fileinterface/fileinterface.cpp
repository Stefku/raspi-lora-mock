#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <lmic.h>
#include <string.h>

#include <iostream>
#include <fstream>

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = 0;

static uint8_t mydata[100];

void test() {
	std::string from = "hallo";
	uint8_t to[100];
	for (int i; i<from.size(); i++) {
		std::cout << from[i] << std::endl;
		to[i] = from[i];
		std::cout << to[i] << std::endl;
	}
}

void check_file_2() {
  std::ifstream ifs("command.txt");
  std::string content( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
  if (content.size() > 0) {
    std::cout << "command: ";
    std::cout << content << std::endl;
    // do something
    std::remove("command.txt");
  }
}


void sig_handler(int sig)
{
  printf("\nBreak received, exiting!\n");
  force_exit=true;
}


int main(void) 
{
	test();
	return 0;

    printf("Waiting for commands in command.txt\n");

    while(!force_exit) {
      check_file_2();
      usleep(1000000);
    }

    printf( "done my job!\n");
    return 0;
}
