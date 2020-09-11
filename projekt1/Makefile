#
#Autor: Sara Skutova
#Projekt: 1. projekt do IPK - komunikace Klient/Server
#Soubory: klient.cpp, server.cpp
#Darutm: 20. 2. 2015
#

CC = g++
CFLAGS = -Wall -Wextra -pedantic

all: server client

server: server.cpp
	$(CC) $(CFLAGS) server.cpp -o server
	
client: klient.cpp
	$(CC) $(CFLAGS) klient.cpp -o client
	
clean:
	rm -f *.o *.out server client *~