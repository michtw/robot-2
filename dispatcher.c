#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>

#include "config.h"

int cgi_sock;
int service2_sock = -1;
int service4_sock = -1;
int stm_sock = -1;

int service2_register = 0;
int service4_register = 0;
int stm_register = 0;

int highsock;        /* Highest #'d file descriptor, needed for select() */
int connectlist[5];
fd_set socks;
int sock;
int sock_now = -1;

static void setnonblocking(int sock)
{
        int opts;

        opts = fcntl(sock, F_GETFL);
        if (opts < 0) {
                perror("fcntl(F_GETFL)");
                exit(EXIT_FAILURE);
        }   
        opts = (opts | O_NONBLOCK);
        if (fcntl(sock, F_SETFL, opts) < 0) {
                perror("fcntl(F_SETFL)");
                exit(EXIT_FAILURE);
        }   
        return;
}

static void handle_new_connection(void) 
{
        int listnum;         /* Current item in connectlist for for loops */
        int connection; /* Socket file descriptor for incoming connections */
        socklen_t addr_length;
	struct sockaddr_un address;

	address.sun_family = AF_UNIX;       /* Unix domain socket */
	strcpy(address.sun_path, DSOCKET_PATH);

	/* The total length of the address includes the sun_family element */
	addr_length = sizeof(address.sun_family) + strlen(address.sun_path);

        /* We have a new connection coming in!  We'll
           try to find a spot for it in connectlist. */
//        connection = accept(sock, NULL, NULL);
	connection = accept(sock, (struct sockaddr *) &address, &addr_length);
        if (connection < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
        }

        setnonblocking(connection);

        for (listnum = 0; (listnum < 5) && (connection != -1); listnum ++) {
                if (connectlist[listnum] == 0) {
                        printf("\nConnection accepted:   FD=%d; Slot=%d\n",
                                connection, listnum);
                        connectlist[listnum] = connection;
                        connection = -1;
                }
	}
        if (connection != -1) {
                /* No room left in the queue! */
                printf("\nNo room left for new client.\n");
                // sock_puts(connection,"Sorry, this server is too busy.  Try again later!\r\n");
                close(connection);
        }
}

/* This function reads from a socket, until it recieves a linefeed
   character.  It fills the buffer "str" up to the maximum size "count".

   This function will return -1 if the socket is closed during the read
   operation.

   Note that if a single line exceeds the length of count, the extra data
   will be read and discarded!  You have been warned. */
static int sock_gets(int sockfd, unsigned char *str, size_t count)
{
	int bytes_read = 1;
	int total_count = 0;
	int idx = 0;
	unsigned char last_read = 0;

	while (bytes_read > 0) {
		bytes_read = read(sockfd, &last_read, 1);

		if (bytes_read <= 0) {
			/* The other side may have closed unexpectedly */
			return -1;
		} else {
			str[idx] = last_read;
			idx++;
			DBG("0x%02x ", last_read);
			total_count++;
		}
		if ((str[0] == 0xf1) && (str[1] == 0xf2) &&
			str[2] == total_count) {
				break;
		}
	}

	DBG("\n");
	return total_count;
}

static int deal_with_data(int listnum, unsigned char *buf) 
{
	int amount, i;

        amount = sock_gets(connectlist[listnum], buf, sizeof(buf));
	if (amount < 0) {
                /* Connection closed, close this end
                   and free up entry in connectlist */
                printf("\nConnection lost: FD=%d; Slot=%d\n", connectlist[listnum], listnum);
                close(connectlist[listnum]);
                connectlist[listnum] = 0;
	} else {
#ifdef DEBUG
		DBG("Service request(%d): ", amount);
		for (i = 0; i < amount; i++) {
			DBG("0x%02x ", buf[i]);
		}
		DBG("\n");
#endif
	}
	return amount;
}

static int read_socks(unsigned char *buf) 
{
	int len = -1;
        int listnum;         /* Current item in connectlist for for loops */

        /* OK, now socks will be set with whatever socket(s)
           are ready for reading.  Lets first check our
           "listening" socket, and then check the sockets
           in connectlist. */
        if (FD_ISSET(sock, &socks))
                handle_new_connection();

        /* Now check connectlist for available data */
        for (listnum = 0; listnum < 5; listnum++) {
                if (FD_ISSET(connectlist[listnum], &socks)) {
			sock_now = connectlist[listnum];
			printf("sock_now: %d\n", sock_now);
                        len = deal_with_data(listnum, buf);
		}
        } /* for (all entries in queue) */
	return len;
}

static void build_select_list(void) 
{
        int listnum;         /* Current item in connectlist for for loops */

        /* First put together fd_set for select(), which will
           consist of the sock veriable in case a new connection
           is coming in, plus all the sockets we have already
           accepted. */
        FD_ZERO(&socks);
        FD_SET(sock, &socks);
        for (listnum = 0; listnum < 5; listnum++) {
                if (connectlist[listnum] != 0) {
                        FD_SET(connectlist[listnum], &socks);
                        if (connectlist[listnum] > highsock) {
                                highsock = connectlist[listnum];
				printf("highsock: %d\n", highsock);
			}
                }   
        }   
}

static int verify_checksum(unsigned char *buf)
{
	int len = buf[2];
	int i;
	int sum = 0;

	len--; // Minus checksum byte.
        for (i = 0; i < len; i++) {
		sum += buf[i];
	}
	sum &= 0xff;
/*
	printf("sum: %#x\n", sum);
	printf("buf[%d]: %#x\n", len, buf[len]);
 */
	if ((buf[len] & 0xff) != sum) {
	    return 1;
	} else {
	    return 0;
	}
}

unsigned char checksum(unsigned char *buf, int len)
{
	int i;
	int sum = 0;

        for (i = 0; i < len; i++) {
		sum += buf[i];
	}
	return (sum &= 0xff);
}

static int bypass_cmd(int conn, unsigned char *data, int len)
{
	int number;
        int path;

	assert(conn > 0);

	path = data[3];

	switch (path) {
		case SERVICE42STM:
			break;
		case SERVICE42SERVICE2:
			break;
		case SERVICE42CGI:
			break;
	}
	number = write(conn, data, len);  // Pass response to CGI.
	DBG("Bypass response.  socket-fd: %d, Written: %d\n", conn, number);
	return number;
}

/*
 *  Just bypass data here.
 */
int cgi2service(int cgi_sock, int service_sock, unsigned char *data, int len, char *serv_path) 
{
	int number;

	assert(cgi_sock > 0);
	assert(service_sock > 0);
        // TODO: Should return NACK here if don't have connect with required service.
	number = write(service_sock, data, len);  // Pass request command to service.
	DBG("Pass request command to service. Written: %d\n", number);

	return 0;
}

void *cgi_incoming(void *ptr)
{
	struct sockaddr_un address;
	int sockc, conn;
	socklen_t addr_length;
	int amount;
	fd_set ready;
	struct timeval to;
        unsigned char buf[512];
        int i;
        int path;

        //const char ack_cmd[] = {0xF1, 0xF2, 0x07, 0x13, 0x90, 0x03, 0x90};
        const char nack_checksum_err[] = {0xF1, 0xF2, 0x08, 0x03, 0x90, 0x04, 0x06, 0x88};

	if ((sockc = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	/* Remove any preexisting socket (or other file) */
	unlink(CGI_SOCKET_FILE);

	address.sun_family = AF_UNIX;       /* Unix domain socket */
	strcpy(address.sun_path, CGI_SOCKET_FILE);

	/* The total length of the address includes the sun_family
	   element */
	addr_length = sizeof(address.sun_family) + strlen(address.sun_path);

	if (bind(sockc, (struct sockaddr *) &address, addr_length)) {
		perror("bind");
		exit(1);
	}

	chmod(CGI_SOCKET_FILE, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | 
                               S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
     /*
	I second using SOMAXCONN, unless you have a specific reason to use a short queue.

	Keep in mind that if there is no room in the queue for a new connection, 
	no RST will be sent, allowing the client to automatically continue trying 
	to connect by retransmitting SYN.

	Also, the backlog argument can have different meanings in different socket implementations.

	In most it means the size of the half-open connection queue, 
	in some it means the size of the completed connection queue.
	In many implementations, the backlog argument will multiplied to yield a different queue length.
	If a value is specified that is too large, all implementations will 
	silently truncate the value to maximum queue length anyways.

      */

	if (listen(sockc, SOMAXCONN)) {
		perror("listen");
		exit(1);
	}

	do {
		FD_ZERO(&ready);
		FD_SET(sockc, &ready);
		to.tv_sec = 5;
		if (select(sockc + 1, &ready, 0, 0, &to) < 0) {
			//perror("select");
			continue;
		}

		if (FD_ISSET(sockc, &ready)) {
			conn = accept(sockc, (struct sockaddr *) &address, &addr_length);

                        cgi_sock = conn;
			if (conn == -1) {
				perror("accept");
				continue;
			}
			DBG("---- Dispatcher getting data from CGI.\n");

			memset(buf, '\0', sizeof(buf));
			amount = read(conn, buf, sizeof(buf)); // CGI request.
			
			if (verify_checksum(buf)) {
				printf("Command checksum error.\n");
				// Send NACK package.
                                write(conn, nack_checksum_err, sizeof(nack_checksum_err));
				continue;
			}

			DBG("CGI request(%d): ", amount);
			for (i = 0; i < amount; i++) {
				printf("0x%02x ", buf[i] & 0xff);
			}
			DBG("\n");

			path = buf[3];

			switch (path) {
				case CGI2SERVICE2:
					printf("CGI 2 Service-2\n");
					cgi2service(cgi_sock, service2_sock, buf, 
					            amount, DIS_SER2_SOCKET_FILE);
					break;
				case CGI2SERVICE4:
					printf("CGI 2 Service-4\n");
					cgi2service(cgi_sock, service4_sock, buf, 
					            amount, DIS_SER4_SOCKET_FILE);
					break;
				case CGI2STM:
					printf("CGI 2 STM\n");
					cgi2service(cgi_sock, stm_sock, buf, 
					            amount, DIS_STM_SOCKET_FILE);
					break;
				default:
					printf("0x%x: Wrong data format.\n", path);
					break;
			}

			DBG("---- done\n");
			/////////////////////// close(conn);
		} else {
			//printf("Do something else ...\n");
		}
	} while (1);

	close(sockc);
}

static int reg_service(int conn, unsigned char path)
{
	int amount;
        unsigned char route = 0x00;
	unsigned char register_ack[] = {0xF1, 0xF2, 0x07, 0x02, 0x90, 0x03, 0xFF};

	switch (path) { // command route path
		case STM2DIS:
		        DBG("Service STM register.\n");
			stm_register = 1;
			stm_sock = conn;
			route = 0x01;
			break;
		case SERVICE22DIS:
			DBG("Service 2 register.\n");
			service2_register = 1;
			service2_sock = conn;
			route = 0x02;
			break;
		case SERVICE42DIS:
			DBG("Service 4 register.\n");
			service4_register = 1;
			service4_sock = conn;
			route = 0x04;
			break;
		default:
			printf("ERROR: Wrong command path. (%#x)\n", path);
			break;
	}
	register_ack[3] = route;
	register_ack[6] = checksum(register_ack, 6);
	amount = write(conn, register_ack, sizeof(register_ack));
	DBG("Register ack written: %d\n", amount);
	return amount;
}

int parse_route(unsigned char route)
{
    int sock = -1;

    switch (route) {
	    case STM2CGI:
	    case SERVICE22CGI:
	    case SERVICE42CGI:
		    sock = cgi_sock;
		    break;
	    case STM2SERVICE2:
		    sock = service2_sock;
		    break;
	    case STM2SERVICE4:
		    sock = service4_sock;
		    break;
	    case SERVICE22STM:
		    sock = stm_sock;
		    break;
	    case SERVICE22SERVICE4:
		    sock = service4_sock;
		    break;
	    case SERVICE42STM:
		    sock = stm_sock;
		    break;
	    case SERVICE42SERVICE2:
		    sock = service2_sock;
		    break;
	    default:
		    printf("ERROR: Wrong command route path. (%#x)\n",
			   route);
		    break;
    }
    return sock;
}

int is_cgi(unsigned char route)
{
    int v = 0;
    switch (route) {
	    case CGI2SERVICE2:
	    case CGI2SERVICE4:
	    case CGI2STM:
		    v = 1;
		    break;
    }
    return v;
}

void *service_incoming(void *ptr)
{
	struct sockaddr_un address;
	socklen_t addr_length;
	int amount;
	struct timeval timeout;
        unsigned char buf[512];
        int path;
        int command;
        int readsocks;
        int which_sock;
        fd_set ready;
        struct timeval to;

        memset(connectlist, 0, 5);

	if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	/* Remove any preexisting socket (or other file) */
	unlink(DSOCKET_PATH);

	address.sun_family = AF_UNIX;       /* Unix domain socket */
	strcpy(address.sun_path, DSOCKET_PATH);

	/* The total length of the address includes the sun_family element */
	addr_length = sizeof(address.sun_family) + strlen(address.sun_path);

	if (bind(sock, (struct sockaddr *) &address, addr_length)) {
		perror("bind");
		exit(1);
	}

	chmod(DSOCKET_PATH, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | 
                            S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);

	if (listen(sock, SOMAXCONN)) {
		perror("listen");
		exit(1);
	}

/*
	conn = accept(sock, (struct sockaddr *) &address, &addr_length);
	if (conn < 0) {
		perror("accept");
	}
 */
	/* Since we start with only one socket, the listening socket,
	      it is the highest socket so far. */
	highsock = sock;

	do {
                build_select_list();

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		
		// nfds is the highest-numbered file descriptor in any of the three sets, plus 1.
                readsocks = select(highsock + 1, &socks, (fd_set *) 0, (fd_set *) 0, &timeout);
                
                if (readsocks < 0) {
                        perror("select");
                        exit(EXIT_FAILURE);
                } 
	
                if (readsocks > 0) {
			amount = read_socks(buf);
                        if (amount < 0) {
				continue;
			}
			command = (buf[4] << 8) | buf[5];
			path = buf[3];

			if (command == REGISTER) {
				reg_service(sock_now, path);
				continue;
			}

			which_sock = parse_route(path);

			if (which_sock < 0) {
				printf("Invalid request descriptor.\n");
			}

	      	        amount = bypass_cmd(which_sock, buf, amount);
			if (amount < 0) {
				perror("write");
			}
	 
			if (command == ACK) {
				continue;
			}

			FD_ZERO(&ready);
			FD_SET(which_sock, &ready);
			to.tv_sec = 3;
			//printf("Start read ACK. 2 seconds timeout.\n");
			if (select(which_sock + 1, &ready, 0, 0, &to) < 0) {
				//perror("select");
				close(which_sock);
				continue;
			}
		        if (FD_ISSET(which_sock, &ready)) {
				amount = read(which_sock, buf, sizeof(buf));  // Read ACK from CGI.
				if (is_cgi(buf[3])) {
				        printf("Read ACK from CGI. amount: %d\n", amount);
				        close(which_sock);
				} else {
				        printf("Read ACK from service. amount: %d\n", amount);
				}

			} else {
			        printf("Cannot get ACK from CGI.\n");
				close(which_sock);
                                //close(conn);
				continue;
			}

			amount = write(sock_now, buf, amount);     // Bypass ACK to request side.
			printf("Bypass ACK to request side. Amount: %d\n", amount);
		} 

	} while (1);

	close(sock);
}

int main(int argc, char *argv[]) 
{
        int iret_cgi, iret_s;
	pthread_t thread_cgi;  // Accept for CGI connection.
	pthread_t thread_s;   // Accept for Service connection.

        iret_cgi = pthread_create(&thread_cgi, NULL, cgi_incoming, (void *)NULL);
        iret_s = pthread_create(&thread_s, NULL, service_incoming, (void *)NULL);

        pthread_join(thread_cgi, NULL);
        pthread_join(thread_s, NULL); 

        printf("Thread CGI returns: %d\n", iret_cgi);
        printf("Thread Service returns: %d\n", iret_s);

        return 0;
}

