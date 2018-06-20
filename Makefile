CFLAGS= -std=c99 -Wall -pedantic
all: mftp mftpserve

mftp: mftp.o
	gcc -g mftp.o -o mftp
mftpserve: mftpserve.o
	gcc -g mftpserve.o -o mftpserve
mftp.o: mftp.c
	gcc -g -c $(CFLAGS) mftp.c
mftpserve.o: mftpserve.c
	gcc -g -c $(CFLAGS) mftpserve.c
clean:
	rm *.o mftp mftpserve
