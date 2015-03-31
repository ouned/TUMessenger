#################################################################################
# TUMessenger Makefile
# Autor: download
#
# Anmerkung:
# Ich kompiliere den TUMessenger immer mit wxWidgets 2.9.2
# und libcurl 7.21.4
#################################################################################

CC = g++
LIBS = `wx-config --libs all` `curl-config --libs`
CFLAGS = `wx-config --cflags` `curl-config --cflags`


build:
	mkdir -p obj/Linux
	$(CC) -s -O3 -Os $(CFLAGS) -c TUStartFrame.cpp -o obj/Linux/TUStartFrame.o
	$(CC) -s -O3 -Os $(CFLAGS) -c TUBuddylist.cpp -o obj/Linux/TUBuddylist.o
	$(CC) -s -O3 -Os $(CFLAGS) -c TUChatwindow.cpp -o obj/Linux/TUChatwindow.o
	$(CC) -s -O3 -Os $(CFLAGS) -c TUConnection.cpp -o obj/Linux/TUConnection.o
	$(CC) -s -O3 -Os $(CFLAGS) -c TUExFuncs.cpp -o obj/Linux/TUExFuncs.o
	mkdir -p bin/Linux
	$(CC) -o bin/Linux/tumessenger obj/Linux/TUChatwindow.o obj/Linux/TUBuddylist.o obj/Linux/TUConnection.o obj/Linux/TUExFuncs.o obj/Linux/TUStartFrame.o -s $(LIBS)

install:
	mkdir -p /usr/share/tumessenger
	cp -r -f Linux/* /usr/share/tumessenger
	mv -f /usr/share/tumessenger/TUMessenger.desktop /usr/share/applications
	cp -f bin/Linux/tumessenger /usr/bin

remove:
	rm -r /usr/share/tumessenger
	rm /usr/share/applications/TUMessenger.desktop
	rm /usr/bin/tumessenger

clean:
	rm -r obj
	rm bin/Linux/tumessenger
