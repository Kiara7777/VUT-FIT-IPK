#
#Autor: Sara Skutova
#Projekt: 2. projekt do IPK - UDP tester
#Soubory: klient.cpp, server.cpp
#Darutm: 3. 5. 2015
#

AUTOR = xskuto00
CC = g++
CFLAGS = -Wall -Wextra -pedantic

all: ipkperfclient ipkperfserver

server: ipkperfserver.cpp
	$(CC) $(CFLAGS) ipkperfserver.cpp -o ipkperfserver
	
client: ipkperfclient.cpp
	$(CC) $(CFLAGS) ipkperfclient.cpp -o ipkperfclient
	
clean:
	rm -f *.o *.out ipkperfserver ipkperfclient *.tgz *~

pack:
	tar -cvzf $(AUTOR).tgz ipkperfserver.cpp ipkperfclient.cpp Makefile README
 
	