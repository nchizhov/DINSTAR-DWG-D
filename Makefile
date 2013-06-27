# Makefile to compile dwgclient
#
# Author: Chizhov Nikolay
#         home.kgd.in
#

CC      = gcc

all:
	$(CC) -g -Werror -o dwgd dwgd.c -ldwgsms.pub -lpthread -L. -liniparser
clean:
	rm -rf dwgd
