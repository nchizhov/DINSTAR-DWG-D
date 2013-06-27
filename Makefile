# Makefile to compile dwgclient
#
# Author: Carlos Ruiz Diaz
#         caruizdiaz.com
#

CC      = gcc

all:
	$(CC) -g -Werror -o dwgd dwgd.c -ldwgsms.pub -lpthread -L. -liniparser
clean:
	rm -rf dwgd
