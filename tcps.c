#include	<stdio.h>
#include	<fcntl.h>
#include	<sys/errno.h>
#include 	<termios.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<arpa/telnet.h>
//
// Convert ip to dot
//
char	*lib_ip2dot(unsigned long ip)
{
	unsigned char	*c;
	static char	ip_dot[20];

	c = (unsigned char *)&ip;
	sprintf(ip_dot, "%d.%d.%d.%d", c[0], c[1], c[2], c[3]);
	return ip_dot;
}

int	main(int argc, char *argv[])
{
	int			port,i,l,len;
	unsigned long		ip,mode;
	char			buf[4096];
	char			hname[80];
	struct hostent		*hent;
	int			fd,f,a,s;
	struct sockaddr_in	so;
	fd_set			readfds,writefds;
	struct timeval		authtime;
	struct termios		t;

	if ( argc < 3 ) {
		printf("Syntax: %s TCP_Port tty\n",argv[0]);
		return -1;
	}
	port = atoi(argv[1]);
	gethostname(hname,sizeof(hname));
	hent = gethostbyname(hname);
	if (hent == NULL) {
		printf("can't get host!\n");
		return(-1);
	}
	ip = *(unsigned long *)hent->h_addr;
	printf("host ip address = %s,port = %d\n",(char *)lib_ip2dot(ip),port);
//
// TCPS init
//
	memset((char *)&so,0,sizeof(so));
	so.sin_family = AF_INET;
	so.sin_port = htons((unsigned short)port);
	so.sin_addr.s_addr = INADDR_ANY;
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	len = sizeof(so);
	if ( bind(fd, (struct sockaddr *)&so, len) < 0 ) {
		printf("Can't bind to TCP port %d!\n",port);
		return(-1);
	}

	f = open(argv[2],O_RDWR);
	if (f < 0) {
		printf("open %s error (%d)!\n",argv[2],f);
		close(f);
	} else {
		printf("open %s as 9600 n,8,1 H/W flow control ok.\n",argv[2]);
	}
	tcgetattr(f,&t);
	t.c_lflag = 0;
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 0;
	t.c_cflag = B9600 | CRTSCTS | CS8 | CREAD | CLOCAL; 
	t.c_iflag = 0;
	t.c_oflag = 0;
	tcsetattr(f,TCSANOW,&t);
	tcflush(f,TCIOFLUSH);
	a = -1;
	l = listen(fd,1);
	printf("listen result = %d\n",l);
	for (;;) {
	    if (a < 0) {
		a = accept(fd,0,0);
		if (a >= 0 ) {
			printf("connection established.\n");
			if (a >= f)
				s = a + 1;
			else
				s = f + 1;
			tcflush(f,TCIOFLUSH);
	        }
	    }
	    if (a >= 0 ) {
		authtime.tv_usec = 0L;
		authtime.tv_sec = 1;
		FD_ZERO(&readfds);
		FD_SET(a, &readfds);
		FD_SET(f, &readfds);
		if (select (s, &readfds, NULL, NULL, &authtime) >= 0) {
			if (FD_ISSET (a, &readfds)) {
				len = recv(a,buf,sizeof(buf),0);
/* printf("lan %d\n",len); */
				if (len <= 0) {
					printf("Connection lost!\n");
					close(a);
					a = -1;
				} else {
					write(f,buf,len);
				}
			}
			if (FD_ISSET (f, &readfds)) {
				len = read(f,buf,sizeof(buf));
/* printf("tty %d\n",len); */
				if (len > 0) {
					send(a,buf,len,0);
				}
			}
		}
	   }
	}
	close(fd);
	printf("program exit.\n");
	return 0;
}
