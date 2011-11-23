#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include "config.h"

const unsigned long mac = 0x6A6B6C6D6E6FUL;

unsigned char ssid[32];
unsigned char security_method;
unsigned char key[32];

char *getSSID(void)
{
	return "COVIA";
}

void return_ssid(int sock, char *ssid)
{
	int len;
        int i;
        short sum = 0x00;

	unsigned char buf[512];
      
	buf[0] = 0xF1; 
	buf[1] = 0xF2; 
	buf[2] = 0xFF;  // command length
	buf[3] = 0x43; 
	buf[4] = 0xA0; 
	buf[5] = 0x02; 
					
	strncpy((char *)&(buf[6]), ssid, strlen(ssid));

        len = 6 + strlen(ssid);
	buf[2] = len + 1; // Last one value is checksum.
        for (i = 0; i < len; i++) {
                sum += buf[i];
	}
	buf[len] = sum & 0xff;

	printf("SSID response: ");
        for (i = 0; i < (len + 1); i++) {
		printf("0x%02x ", buf[i] & 0xff);
	}
	printf("\n");

	len = write(sock, buf, len + 1);
	printf("SSID(%s) written: %d. \n", ssid, len);

	memset(buf, '\0', sizeof(buf));
	len = read(sock, buf, sizeof(buf)); // Read ACK.
	printf("ACK read: len: %d\n", len);
}

static int set_wlan(unsigned char *data)
{
	int i;
	char *p;
       
	memset(ssid, '\0', sizeof(ssid));
	memset(key, '\0', sizeof(key));

        p = strchr((char *)data, '=');
	p++;
	for (i = 0; i < 32; i++) {
		ssid[i] = p[i];
		if (p[i+1] == ';') {
			break;
		}
	}
        printf("ssid: %s\n", ssid);

        p = strchr(p, '=');
        p++;
	security_method = p[0];
        printf("security_method: %c\n", security_method);

        p = strchr(p, '=');
        p++;
	for (i = 0; i < 32; i++) {
		key[i] = p[i];
		if (p[i+2] == '\0') { // end of data. Don't copy checksum data.
			break;
		}
	}
        printf("key: %s\n", key);

	return 0;
}

static int get_wlan(unsigned char *data)
{
	int len;
	len = sprintf((char *)data, "SSID=%s;KEY=%s;MAC=%lx", ssid, key, mac);
	return len;
}

static void return_data(int sock, unsigned char *data, int len, int cmd)
{
        int i;
        short sum = 0x00;

	unsigned char buf[512];
      
	memset(buf, '\0', sizeof(buf));

	buf[0] = 0xF1; 
	buf[1] = 0xF2; 
	buf[2] = 0x00;  // command length
	buf[3] = 0x43; 
	buf[4] = (cmd >> 8) & 0xff; 
	buf[5] = cmd & 0xff; 
					
	strncpy((char *)&(buf[6]), (char *)data, len);

        len = 6 + len;
	buf[2] = len + 1; // Last one value is checksum.
        for (i = 0; i < len; i++) {
                sum += buf[i];
	}
	buf[len] = sum & 0xff;

	printf("Response: ");
        for (i = 0; i < (len + 1); i++) {
		printf("0x%02x ", buf[i] & 0xff);
	}
	printf("\n");

	len = write(sock, buf, len + 1);
	printf("Written: %d\n", len);

	memset(buf, '\0', sizeof(buf));
	len = read(sock, buf, sizeof(buf)); // Read ACK.
	printf("ACK read: len: %d\n", len);
}

void handle_cmd(int sock, unsigned char *buf)
{
        int cmd = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);
        switch (cmd) {
		case GetSsID: {
			char *ssid = getSSID();
			return_ssid(sock, ssid);
			break;
		}
		case GetWlan: {
			unsigned char data[128];
			int len;
			memset(data, '\0', sizeof(data));
			len = get_wlan(data);
                        return_data(sock, data, len, GetWlan);
			break;
		}
		case SetWlan:
			set_wlan(buf);
			break;
	}
}

void *conn(void *ptr)
{
    int sock;
    struct sockaddr_un address;
    size_t addr_length;
    int amount;
    fd_set ready_r;        
    struct timeval to;
    int i;

    unsigned char buf[256];

    const unsigned char register_cmd[] = {0xF1, 0xF2, 0x07, 0x40, 0x90, 0x01, 0xBB};
    const unsigned char ack_cmd[] = {0xF1, 0xF2, 0x07, 0x43, 0x90, 0x03, 0xC0};

    if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
	    perror("socket");
	    exit(1);
    }

    address.sun_family = AF_UNIX;    /* Unix domain socket */
    strcpy(address.sun_path, SER4_DIS_SOCKET_FILE);

    /* The total length of the address includes the sun_family element */
    addr_length = sizeof(address.sun_family) + strlen(address.sun_path);

    if (connect(sock, (struct sockaddr *) &address, addr_length)) {
	    printf("Cannot register service. (%s)\n", strerror(errno));
	    exit(1);
    }

    amount = write(sock, register_cmd, sizeof(register_cmd));  // Register of service.
    amount = read(sock, buf, sizeof(buf));  // Read ACK from dispatcher.
    if (amount <= 0) {
	    printf("Cannot register service.\n");
	    exit(1);
    }

    do {
	FD_ZERO(&ready_r);
	FD_SET(sock, &ready_r);
	to.tv_sec = 3;
	if (select(sock + 1, &ready_r, 0, 0, &to) < 0) {
		perror("select");
	}
	if (FD_ISSET(sock, &ready_r)) {
		memset(buf, '\0', sizeof(buf));
		amount = read(sock, buf, sizeof(buf)); // Dispatcher request.
		if (amount < 0) {
			perror("read");
			continue;
		}
		printf("[S4] Dispatcher request(%d): ", amount);
		for (i = 0; i < amount; i++) {
			printf("0x%02x ", buf[i] & 0xff);
		}
		printf("\n");

		amount = write(sock, ack_cmd, sizeof(ack_cmd));
		//close(sock);
		printf("ACK write amount: %d\n", amount);

		printf("[S4] ---- done\n");

		usleep(100000); // To wait ACK complete.

		handle_cmd(sock, buf);
	} 
    } while (1);

    close(sock);

    return 0;
}

int main(int argc, char *argv[]) 
{
        int iret_conn;
	pthread_t thread_conn;

        iret_conn = pthread_create(&thread_conn, NULL, conn, (void *)NULL);

        pthread_join(thread_conn, NULL); 

        printf("Thread conn returns: %d\n", iret_conn);

        return 0;
}

