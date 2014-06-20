/* Uclient.c*/
/* ref1  http://stackoverflow.com/questions/1551597/using-strftime-in-c-how-can-i-format-time-exactly-like-a-unix-timestamp */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>       /* for gettimeofday() */
#include <time.h>           /* for gettimeofday() */
#include <stdlib.h>
#include <strings.h>

#define INTERVAL 60

/* This is the client program. It creates a socket and initiates a
 * connection with the socket given in the command line. 
 * The form of the command line is `uclient hostname portnumber'. 
 */

int main(int argc,char *argv[]){
  
  if ( argc < 2 ){
    printf("usage: ./uclient <HA-ADDR> <HA-PORT>\n", argv[0] );
    return 0;
  }
 
  int sock;
  struct sockaddr_in uha, client;
  struct hostent *hp, *gethostbyname();
  char buf[1024];
  char fmt[64], out[64];
  int rval, seqnum;
  int i, seqno, fa;
  char *addr;
  struct timeval rcvtime;
  struct timezone zone;
  struct tm *tm;
  /* create socket */
  
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock < 0) {
    perror("Opening datagram socket");
    exit(1);
  }
  
  /* now bind client address. Port is wildcard, doesn't matter. */
  client.sin_family = AF_INET;
  client.sin_addr.s_addr = INADDR_ANY;
  client.sin_port = 0;
  if (bind(sock, (struct sockaddr *)&client, sizeof(client)) < 0) {
    perror("binding socket name");
    exit(1);
  }
  
  uha.sin_family = AF_INET;
  hp = gethostbyname(argv[1]);
  if(hp == 0) {
    printf("%s: unknown host\n", argv[1]);
    exit(2);
  }
  bcopy(hp->h_addr, &uha.sin_addr, hp->h_length);
  
  // We initialize the first home agent port here
  // from the command ine input. The port then change
  // as per request by the mobile node.
  uha.sin_port = htons(atoi(argv[2]));
  
  // We initialize the sequence number here.
  seqnum = 0;
  
  for ( i = 0; i < INTERVAL; i++ ){
    
    seqnum++;  
    /* create a packet */
    sprintf(buf, "%d %d %d", seqnum, 0, atoi(argv[2]) );
    if ((rval=sendto(sock,buf,sizeof(buf),0,(struct sockaddr *)&uha,sizeof(uha))) < 0) {
      perror("writing on datagram socket");
    } else {
      /* get current relative time */
      if (gettimeofday(&rcvtime, &zone) < 0) {
	perror("getting time");
	exit(1);
      }
      /* ref */
      if ( (tm = localtime(&rcvtime.tv_sec)) != NULL ){
	strftime(fmt, sizeof fmt, "%H:%M:%S", tm);
	snprintf(out, sizeof out, fmt, rcvtime.tv_usec);
      }
      addr = inet_ntoa(uha.sin_addr);
      printf("Sequence Number = %2d Time = %s Dest %s:%d\n", seqnum, out, addr, htons(uha.sin_port) );
      sleep(1);
    }
  }
  close(sock);
  return 0;
}
