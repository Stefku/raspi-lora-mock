# Makefile
# Sample for ttn-otaa example on Raspberry Pi
# Caution: requires bcm2835 library to be already installed
# http://www.airspayce.com/mikem/bcm2835/

CC       = g++
CFLAGS   = -std=c++11 -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"$*\"
LIBS     = -lbcm2835
LMICBASE = ../src
INCLUDE  = -I$(LMICBASE) 

all: fileinterface

fileinterface.o: fileinterface.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

fileinterface: fileinterface.o
				$(CC) $^ $(LIBS) -o fileinterface

clean:
				rm -rf *.o fileinterface
