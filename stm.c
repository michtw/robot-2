#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#include "config.h"
#include "stm.h"

int moving_status(void)
{
    return 3;
}

void auto_drive(char parm) 
{
    switch (parm) {
            case AD_ACTION_STOP:
		    printf("AD_ACTION_STOP\n");
		    break;
            case AD_AUTO_ACTION:
		    printf("AD_AUTO_ACTION\n");
		    break;
            case AD_SPOT1_ACTION:
		    printf("AD_SPOT1_ACTION\n");
		    break;
            case AD_SPOT2_ACTION:
		    printf("AD_SPOT2_ACTION\n");
		    break;
            case AD_WALL_ACTION:
		    printf("AD_WALL_ACTION\n");
		    break;
            case AD_PCI_ACTION:
		    printf("AD_PCI_ACTION\n");
		    break;
    }
}

void manual_drive(char parm)
{
    switch (parm) {
            case MD_ACTION_STOP:
		    printf("MD_ACTION_STOP\n");
		    break;
            case MD_FORWARD:
		    printf("MD_FORWARD\n");
		    break;
            case MD_BACK:
		    printf("MD_BACK\n");
		    break;
            case MD_RIGHT_TURN:
		    printf("MD_RIGHT_TURN\n");
		    break;
            case MD_LEFT_TURN:
		    printf("MD_LEFT_TURN\n");
		    break;
    }
}

void get_moving_status(void)
{
    struct sockaddr_un address;
    int sock;
    size_t addr_length;
    int len;
    int i, status, idx = 0;
    short sum = 0x00;
 
    char buf[128];

    if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
	    perror("socket");
	    return;
    }

    address.sun_family = AF_UNIX;    /* Unix domain socket */
    strcpy(address.sun_path, DSOCKET_PATH);

    /* The total length of the address includes the sun_family element */
    addr_length = sizeof(address.sun_family) + strlen(address.sun_path);

    if (connect(sock, (struct sockaddr *) &address, addr_length)) {
	    perror("connect");
	    return;
    }

    buf[idx++] = 0xF1; 
    buf[idx++] = 0xF2; 
    buf[idx++] = 0x08;  // command length
    buf[idx++] = 0x13; 
    buf[idx++] = 0xA3; 
    buf[idx++] = 0x04; 

    status = moving_status();
    
    buf[idx++] = status;

    for (i = 0; i < idx; i++) {
	    sum += buf[i];
    }
    buf[idx++] = sum & 0xff;

    printf("Moving status response: ");
    for (i = 0; i < idx; i++) {
	    printf("0x%02x ", buf[i] & 0xff);
    }
    printf("\n");

    len = write(sock, buf, idx);
    printf("Moving status. Written: %d. \n", len);

    memset(buf, '\0', sizeof(buf));
    len = read(sock, buf, sizeof(buf)); // Read ACK.
    printf("ACK read: len: %d\n", len);

    printf("%s", "Close socket.\n");
    close(sock);
}

static void startSubStitutionSpeech(int sock, unsigned char table)
{
    int i;
    unsigned char  buf[256];
    int len;
    //unsigned char ack_cmd[] = {0xF1, 0xF2, 0x07, 0x13, 0x90, 0x03, 0x90};
    unsigned char cmd[] = {0xF1, 0xF2, 0x08, 0x12, 0xA4, 0x02, table, 0xFF};

    cmd[7] = (cmd[0] + cmd[1] + cmd[2] + cmd[3] + cmd[4] + cmd[5] + cmd[6]) & 0xff;  // checksum

    printf("Speed table: %d\n", table);
    //write(sock, ack_cmd, sizeof(ack_cmd)); // Ack to CGI.

    //usleep(100000);

    len = write(sock, cmd, sizeof(cmd));  // Send command to STM
    printf("startSubStitutionSpeech command write: %d\n", len);
    memset(buf, '\0', sizeof(buf));
    len = read(sock, buf, sizeof(buf));
    printf("startSubStitutionSpeech ACK read: %d\n", len);
    for (i = 0; i < len; i++) {
	    printf("0x%02x ", buf[i]);
    }
    printf("\n");
}

void handle_cmd(int sock, unsigned char *buf)
{
    int cmd = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);
    int parm = buf[6];

    switch (cmd) {
	    case AutoDrive:
		    auto_drive(parm);
		    break;
	    case ManualDrive:
		    manual_drive(parm);
		    break;
	    case GoHomeAndDock:
		    printf("GoHomeAndDock NOT support now.\n");
		    break;
	    case GetMovingStatus:
		    get_moving_status();
		    break;
	    case GetJobStatus:
		    printf("GetJobStatus NOT support now.\n");
		    break;
	    case GetErrorCode:
		    printf("GetErrorCode NOT support now.\n");
		    break;
	    case StartSubstitutionSpeech:
		    startSubStitutionSpeech(sock, buf[6]);
                    break;
	    default:
		    printf("[STM] ERROR: Command not support.\n");
		    break;
    }
}

#if 0
void *dispatcher_incoming(void *ptr)
{
    struct sockaddr_un address;
    int sock, conn;
    socklen_t addr_length;
    int amount;
    fd_set ready;
    struct timeval to;
    char buf[1024];
    int i;
    int cmd;

    const char ack_cmd[] = {0xF1, 0xF2, 0x07, 0x13, 0x90, 0x03, 0x90};

    if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
	    perror("socket");
	    exit(1);
    }

    /* Remove any preexisting socket (or other file) */
    unlink(DIS_STM_SOCKET_FILE);

    address.sun_family = AF_UNIX;       /* Unix domain socket */
    strcpy(address.sun_path, DIS_STM_SOCKET_FILE);

    /* The total length of the address includes the sun_family
       element */
    addr_length = sizeof(address.sun_family) + strlen(address.sun_path);

    if (bind(sock, (struct sockaddr *) &address, addr_length)) {
	    perror("bind");
	    exit(1);
    }

    chmod(DIS_STM_SOCKET_FILE, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | 
	                        S_IROTH | S_IWOTH | S_IXOTH);

    if (listen(sock, 5)) {
	    perror("listen");
	    exit(1);
    }

    do {
	    FD_ZERO(&ready);
	    FD_SET(sock, &ready);
	    to.tv_sec = 5;
	    if (select(sock + 1, &ready, 0, 0, &to) < 0) {
		    perror("select");
		    continue;
	    }

	    if (FD_ISSET(sock, &ready)) {
		    conn = accept(sock, (struct sockaddr *) &address, &addr_length);

		    if (conn < 0) {
			    perror("accept");
			    continue;
		    }
		    printf("[STM] ---- getting data\n");

		    memset(buf, '\0', sizeof(buf));
		    amount = read(conn, buf, sizeof(buf)); // CGI request.
		    printf("[STM] Dispatcher request(%d): ", amount);
		    for (i = 0; i < amount; i++) {
			    printf("0x%02x ", buf[i] & 0xff);
		    }
		    printf("\n");

		    amount = write(conn, ack_cmd, sizeof(ack_cmd));
		    close(conn);
		    printf("Write amount: %d\n", amount);

		    printf("[STM] ---- done\n");

		    usleep(100000);
		    cmd = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);

		    handle_cmd(cmd, buf[6]);
	    } else {
		    //printf("Do something else ...\n");
	    }
    } while (1);

    close(sock);
}
#endif

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

    const unsigned char register_cmd[] = {0xF1, 0xF2, 0x07, 0x10, 0x90, 0x01, 0x8B};
    const unsigned char ack_cmd[] = {0xF1, 0xF2, 0x07, 0x13, 0x90, 0x03, 0x90};

    if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
	    perror("socket");
	    exit(1);
    }

    address.sun_family = AF_UNIX;    /* Unix domain socket */
    strcpy(address.sun_path, DSOCKET_PATH);

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

    //printf("Register done.\n");

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
		printf("[STM] Dispatcher request(%d): ", amount);
		for (i = 0; i < amount; i++) {
			printf("0x%02x ", buf[i] & 0xff);
		}
		printf("\n");

		amount = write(sock, ack_cmd, sizeof(ack_cmd));
		//close(sock);
		printf("ACK write amount: %d\n", amount);

		printf("[STM] ---- done\n");

		usleep(100000);

		handle_cmd(sock, buf);
	} 
    } while (1);

    printf("Close socket.\n");
    close(sock);

    return 0;
}

void *bottom_incoming(void *ptr)
{
	while (1) {
		sleep(1);
	}
}

int main(int argc, char *argv[]) 
{
//    int iret_dispatcher;
    int iret_bottom, iret_conn;
//    pthread_t thread_dispatcher;  // Accept for Dispatcher connection.
    pthread_t thread_bottom;      // Accept for Bottom connection.
    pthread_t thread_conn;

//    iret_dispatcher = pthread_create(&thread_dispatcher, NULL, dispatcher_incoming, (void *)NULL);
    iret_bottom = pthread_create(&thread_bottom, NULL, bottom_incoming, (void *)NULL);
    iret_conn = pthread_create(&thread_conn, NULL, conn, (void *)NULL);

//    pthread_join(thread_dispatcher, NULL);
    pthread_join(thread_bottom, NULL); 
    pthread_join(thread_conn, NULL); 

//    printf("Thread dispatcher returns: %d\n", iret_dispatcher);
    printf("Thread bottom returns: %d\n", iret_bottom);
    printf("Thread conn returns: %d\n", iret_conn);
    return 0;
}

