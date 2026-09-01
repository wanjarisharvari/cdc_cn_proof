CC = gcc
CFLAGS = -Wall -pthread
BINS = mysmtp_server mysmtp_client

all: $(BINS)

mysmtp_server: mysmtp_server.c
	$(CC) $(CFLAGS) -o mysmtp_server mysmtp_server.c

mysmtp_client: mysmtp_client.c
	$(CC) $(CFLAGS) -o mysmtp_client mysmtp_client.c

clean:
	rm -f $(BINS)