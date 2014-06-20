/* userver.c */
/* ref http://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c */
/* ref http://stackoverflow.com/questions/1551597/using-strftime-in-c-how-can-i-format-time-exactly-like-a-unix-timestamp */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>           /* for gettimeofday() */
#include <time.h>            
#include <stdlib.h>
#include <strings.h>

#define TRUE 1

/* This is the server program. It opens a socket with the given port
 * and then begins an infinite loop. Each time through the loop it 
 * accepts a pkt and prints out the contents and the current time. 
 * Command line is `userver portnumber'.  
 */

int main(int argc,char *argv[]){
  if ( argc < 3 ){
    printf("usage: ./ufa <UFA-PORT> <UMN-ADDR> <UMN-PORT>\n", argv[0] );
    return 0;
  }
 
  int sock, length;
  struct sockaddr_in server, client, umn;
  struct hostent *hp, *gethostbyname();
  int msgsock;
  char buf[1024];
  char fmt[64], out[64];
  int rval, clilen;
  int i, seqno, reg, fa_port;
  char str[INET_ADDRSTRLEN];
  struct sockaddr_in* addr;
  struct timeval rcvtime;
  struct timezone zone;
  struct tm *tm;
  
  /* create socket */
  
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock < 0) {
    perror("opening datagram socket");
    exit(1);
  }
  
  /* name socket using wildcards */
  
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(atoi(argv[1]));
  if(bind(sock, (struct sockaddr *)&server, sizeof(server))) {
    perror("binding socket name");
    exit(1);
  }
  
  /* find out assigned port number and print out */
  
  length = sizeof(server);
  if(getsockname(sock, (struct sockaddr *)&server, &length)) {
    perror("getting socket name");
    exit(1);
  }
  printf("Socket has port #%d\n",ntohs(server.sin_port));
  i = 1;
  do {
    bzero(buf,sizeof(buf));
    clilen = sizeof(client);
    while ((rval = recvfrom(sock,buf,1024,0,(struct sockaddr *)&client,&clilen))<0){
      perror("receiver recvfrom");
    }
    
    // foreign agent only cares about sequence number so it can print it.
    // we can bypass this extraction completely. The FA only forwards
    // whatever it receives from the home agent, it doesn't need to check
    // or write anything into the payload.
    sscanf(buf, "%d", &seqno );
    
    /* get current relative time */
    if (gettimeofday(&rcvtime, &zone) < 0) {
      perror("getting time");
      exit(1);
    }
    // here we convert epoch to a string with a specific format
    /* ref */
    if ( (tm = localtime(&rcvtime.tv_sec)) != NULL ){
      strftime(fmt, sizeof(fmt), "%H:%M:%S", tm);
      snprintf(out, sizeof(out), fmt, rcvtime.tv_usec);
    }
    
    umn.sin_family = AF_INET;
    hp = gethostbyname(argv[2]);
    if(hp == 0) {
      printf("%s: unknown host\n",argv[2]);
      exit(2);
    }
    bcopy(hp->h_addr, &umn.sin_addr, hp->h_length);
    umn.sin_port = htons(atoi(argv[3]));
    /* ref */
    addr = (struct sockaddr_in*)&umn;
    int ipAddr = addr->sin_addr.s_addr;
    inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
    
    if ((rval=sendto(sock,buf,sizeof(buf),0,(struct sockaddr *)&umn,sizeof(umn))) < 0) {
      perror("writing on datagram socket");
    } else {
      printf("Sequence Number = %2d Time = %s Forwarded to %s:%d\n", seqno, out, str, atoi(argv[3]) );
    }
    i++;
  } while (rval != 0);
  close(sock);
  return 0;
}
