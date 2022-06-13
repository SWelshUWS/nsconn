#ifndef NSCONN_H
#define NSCONN_H

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>  // still to be implemented globally
#include <sys/socket.h>
#include <sys/types.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h> // json timestamp
#include <glib.h>
#include <stdbool.h>
#include <json-c/json.h>


// socket values
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)


// parsing packets
#define SYNC          0xAA
#define EXCODE        0x55
#define RAWWAV        0x80
#define ASICEEGPOWER  0x83
#define LOWPOWER      0x02
#define ATTENTION     0x04
#define MEDITATION    0x05

// output format
#define JSON 0
#define BIN 1


// parse packets
void parsePackets(int *, int);

// json format
int jsonFormat(unsigned char *, int, int, FILE *);

// forward json strings
int forward(const char *, int, FILE *);

#endif
