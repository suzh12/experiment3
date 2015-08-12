//----------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<sys/time.h>

#include <errno.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>

#include <time.h>
#include <netinet/sctp.h>
#include <fcntl.h>

#define UDPDATA 1472
#define SLADATA 1456
#define UDPPORT 10000

#define TCPDATA 1472
#define SLAdata 1456
#define TCPPORT 10001

#define SCTPDATA 1472
#define SLAData 1456
#define SCTPPORT 10002
//----------------------------------------------------------------------------------
struct SLA_header{
	unsigned char typeenddatalen1;
	unsigned char datalen2;
	unsigned short socketseq;
	long int commonseq;
	long int timestemps;
	long int timestempm;
};

//SLA data
struct SLA_data{
	char data[SLADATA];
};
//----------------------------------------------------------------------------------
void usage(char *command)
{
	printf("usage :%s filename\n", command);
	exit(0);
}
//----------------------------------------------------------------------------------
int main(int argc,char **argv){
	
	struct sockaddr_in serv_addr;
	struct sockaddr_in clie_addr;
	int udpsock_id, tcpsock_id, link_id, max, sctpsock_id, connSock, flags, write_len;
	char data_recvu[UDPDATA];
	char data_recvt[TCPDATA];
	char tcp_buf[TCPDATA];
	char data_recvs[SCTPDATA];
	int i, recvtn=0;
	int recv_len;
	int write_leng;
	int clie_addr_len;
	fd_set fd;
	fd_set variablefd;
	int tcpfinish=0, udpfinish=0, sctpfinish=0;
	FILE *fp;
	unsigned short typeenddatalen;
	long int time1[720];//experiment3
	long int time2[720];//experiment3
	long int size[720];//experiment3
	int number=0;//experiment3

	struct sctp_initmsg initmsg;
	struct sctp_sndrcvinfo sndrcvinfo;

	struct SLA_header *headeru = (struct SLA_header *)data_recvu;
	struct SLA_data *datau = (struct SLA_data *)(data_recvu + sizeof(struct SLA_header));
	
	struct SLA_header *headert = (struct SLA_header*)data_recvt;
	struct SLA_data *datat = (struct SLA_data*)(data_recvt + sizeof(struct SLA_header));
	
	struct SLA_header *headers = (struct SLA_header*)data_recvs;
	struct SLA_data *datas = (struct SLA_data*)(data_recvs + sizeof(struct SLA_header));

	struct  timeval    tv;//experiment3
	struct  timezone   tz;//experiment3

	memset(data_recvu,0,sizeof data_recvu);
	memset(data_recvt,0,sizeof data_recvt);
	memset(data_recvs,0,sizeof data_recvs);
//----------------------------------------------------------------------------------
	if (argc != 2) {
		usage(argv[0]);
	}	
//----------------------------------------------------------------------------------
	 /* Create the the file */
	if ((fp = fopen(argv[1], "w")) == NULL) {
		perror("Creat file failed");
		exit(0);
	}
//----------------------------------------------------------------------------------
	/* tcp */
	if ((tcpsock_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Create TCP socket failed\n");
		exit(0);
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(TCPPORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(tcpsock_id, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 ) {
		perror("Bind TCP socket failed\n");
		exit(0);
	}
	if (-1 == listen(tcpsock_id, 10)) {
		perror("Listen TCP socket failed\n");
		exit(0);
	}

//----------------------------------------------------------------------------------
	/* udp */
	if ((udpsock_id = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		perror("Create UDP socket failed\n");
		exit(0);
	}
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(UDPPORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(udpsock_id,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0 ) {
		perror("Bind UDP socket faild\n");
		exit(0);
	}
//----------------------------------------------------------------------------------
	/* sctp */
	if ((sctpsock_id = socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP )) < 0) {
		perror("Create SCTP socket failed\n");
		exit(0);
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SCTPPORT);
	serv_addr.sin_addr.s_addr = htonl( INADDR_ANY );
	if (bind( sctpsock_id, (struct sockaddr *)&serv_addr, sizeof(serv_addr) ) < 0 ) {
		perror("Bind SCTP socket failed\n");
		exit(0);
	}
	memset( &initmsg, 0, sizeof(initmsg) );
	initmsg.sinit_num_ostreams = 5;
	initmsg.sinit_max_instreams = 5;
	initmsg.sinit_max_attempts = 4;
	setsockopt( sctpsock_id, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg) );
	if (-1 == listen(sctpsock_id, 5)) {
		perror("Listen SCTP socket failed\n");
		exit(0);
	}
//----------------------------------------------------------------------------------
	FD_ZERO(&variablefd);
	FD_ZERO(&fd);
	FD_SET(tcpsock_id,&variablefd);
	FD_SET(udpsock_id,&variablefd);
	FD_SET(sctpsock_id,&variablefd);
	
	max = (tcpsock_id > udpsock_id ? tcpsock_id : udpsock_id);
	max = (max > sctpsock_id ? max : sctpsock_id);

	clie_addr_len = sizeof(clie_addr);
	while (1){
		if( (tcpfinish==1) && (udpfinish==1) && (sctpfinish==1) ){
			break;		
		}
		
		fd = variablefd;
		if (select(max+1,&fd,NULL,NULL,NULL)<0) {
			perror("select problem");
			exit(1);
		}
//-----------------------------------------------------------
		if (FD_ISSET(sctpsock_id,&fd) ){
			connSock = accept( sctpsock_id, (struct sockaddr *)NULL, (int *)NULL );
			if (-1 == connSock) {
				perror("Accept socket failed\n");
				exit(0);
			}
			FD_SET(connSock,&variablefd);
			if (connSock > max){
			max=connSock;
			}
		}
		if (FD_ISSET(connSock,&fd) ){
			recv_len = sctp_recvmsg( connSock, data_recvs, SCTPDATA, (struct sockaddr *)NULL, 0, &sndrcvinfo, &flags);
			if(recv_len == 0){
//				printf("Finish receiver sctp\n");//experiment3
				sctpfinish=1;
				close(connSock);
				FD_CLR(connSock,&variablefd);
			}else if(recv_len < 0){
				printf("recv_len < 0!\n");
				typeenddatalen = ((headers->typeenddatalen1 & 0xFF)<< 8) | (headers->datalen2 & 0xFF);
				write_len = fwrite(datas->data, sizeof(char), typeenddatalen%8192, fp);
				
				gettimeofday(&tv,&tz);//experiment3
				time1[number]=tv.tv_sec;//experiment3
				time2[number]=tv.tv_usec;//experiment3
				if( number == 0 ){//experiment3
					size[number]=write_len;//experiment3
				}else{//experiment3
					size[number]=size[number-1]+write_len;//experiment3
				}//experiment3
				number++;//experiment3
				
				if (write_len < typeenddatalen%8192) {
					printf("Write file failed\n");
					break;
				}
			}else{
				typeenddatalen = ((headers->typeenddatalen1 & 0xFF)<< 8) | (headers->datalen2 & 0xFF);

				write_len = fwrite(datas->data, sizeof(char), typeenddatalen%8192, fp);
				
				gettimeofday(&tv,&tz);//experiment3
				time1[number]=tv.tv_sec;//experiment3
				time2[number]=tv.tv_usec;//experiment3
				if( number == 0 ){//experiment3
					size[number]=write_len;//experiment3
				}else{//experiment3
					size[number]=size[number-1]+write_len;//experiment3
				}//experiment3
				number++;//experiment3				
				
				if (write_len < typeenddatalen%8192) {
					printf("Write file failed\n");
					break;
				}
			}
			memset(data_recvs,0,sizeof data_recvs);
		}
//-----------------------------------------------------------
		if (FD_ISSET(tcpsock_id,&fd) ){//link_id//tcpsock_id
			clie_addr_len = sizeof(clie_addr);
			link_id = accept(tcpsock_id, (struct sockaddr *)&clie_addr, &clie_addr_len);
			if (-1 == link_id) {
				perror("Accept socket failed\n");
				exit(0);
			}
			FD_SET(link_id,&variablefd);
			if (link_id > max){
			max=link_id;
			}
		}
		if (FD_ISSET(link_id,&fd) ){
			recv_len = recv(link_id, tcp_buf, TCPDATA, 0);
			if(recv_len == 0){
				typeenddatalen = ((headert->typeenddatalen1 & 0xFF)<< 8) | (headert->datalen2 & 0xFF);
				
				write_leng = fwrite(datat->data, sizeof(char), typeenddatalen%8192, fp);
				
				gettimeofday(&tv,&tz);//experiment3
				time1[number]=tv.tv_sec;//experiment3
				time2[number]=tv.tv_usec;//experiment3
				if( number == 0 ){//experiment3
					size[number]=write_leng;//experiment3
				}else{//experiment3
					size[number]=size[number-1]+write_leng;//experiment3
				}//experiment3
				number++;//experiment3				
				
				if (write_leng < typeenddatalen%8192) {
					printf("Write file failed\n");
					break;
				}
//				printf("Finish receiver tcp\n");//experiment3
				tcpfinish=1;
				close(link_id);
				FD_CLR(link_id,&variablefd);
			}else if(recv_len < 0){
				printf("Recieve Data From Server Failed!\n");
				break;
			}else{
				for(i = 0; i < recv_len; i++){
					data_recvt[recvtn] = tcp_buf[i];
					recvtn++;
					if (recvtn == TCPDATA){
						recvtn = 0;
						typeenddatalen = ((headert->typeenddatalen1 & 0xFF)<< 8) | (headert->datalen2 & 0xFF);
				
						write_leng = fwrite(datat->data, sizeof(char), typeenddatalen%8192, fp);
						
						gettimeofday(&tv,&tz);//experiment3
						time1[number]=tv.tv_sec;//experiment3
						time2[number]=tv.tv_usec;//experiment3
						if( number == 0 ){//experiment3
							size[number]=write_leng;//experiment3
						}else{//experiment3
							size[number]=size[number-1]+write_leng;//experiment3
						}//experiment3
						number++;//experiment3						
						
						if (write_leng < typeenddatalen%8192) {
							printf("Write file failed\n");
							break;
						}
						memset(data_recvt,0,sizeof data_recvt);
					}
				}
			}
		}
//-----------------------------------------------------------
		if (FD_ISSET(udpsock_id,&fd)){
			if (udpfinish==0){
				recv_len = recvfrom(udpsock_id, data_recvu, UDPDATA, 0,(struct sockaddr *)&clie_addr, &clie_addr_len);
				if(recv_len < 0) {
//					printf("Recieve data from client failed!\n");//experiment3
					break;
				}
				typeenddatalen = ((headeru->typeenddatalen1 & 0xFF)<< 8) | (headeru->datalen2 & 0xFF);

				int write_length = fwrite(datau->data, sizeof(char), recv_len-16, fp);
				
				gettimeofday(&tv,&tz);//experiment3
				time1[number]=tv.tv_sec;//experiment3
				time2[number]=tv.tv_usec;//experiment3
				if( number == 0 ){//experiment3
					size[number]=write_length;//experiment3
				}else{//experiment3
					size[number]=size[number-1]+write_length;//experiment3
				}//experiment3
				number++;//experiment3				
				
				if (write_length < recv_len-16){
					printf("File write failed\n");
					break;
				}
				if ( headeru->typeenddatalen1 >= 32){
//					printf("Finish receiver udp\n");//experiment3
					udpfinish=1;
					close(udpsock_id);
					FD_CLR(udpsock_id,&variablefd);
				}
				memset(data_recvu,0,sizeof data_recvu);
			}
		}
//-----------------------------------------------------------
	}
//	printf("Finish receive\n");//experiment3
	printf("first time:%ld %ld\n",time1[0],time2[0]);//experiment3
	for(i=1;i<number;i++){//experiment3
		if(time1[i]>time1[0] && time2[i]<time2[0]){
			if(time2[i]+1000000-time2[0]<10){printf("%ld.00000%ld %d\n",time1[i]-1-time1[0],time2[i]+1000000-time2[0],size[i]);
			}else if(time2[i]+1000000-time2[0]<100){printf("%ld.0000%ld %d\n",time1[i]-1-time1[0],time2[i]+1000000-time2[0],size[i]);
			}else if(time2[i]+1000000-time2[0]<1000){printf("%ld.000%ld %d\n",time1[i]-1-time1[0],time2[i]+1000000-time2[0],size[i]);
			}else if(time2[i]+1000000-time2[0]<10000){printf("%ld.00%ld %d\n",time1[i]-1-time1[0],time2[i]+1000000-time2[0],size[i]);
			}else if(time2[i]+1000000-time2[0]<100000){printf("%ld.0%ld %d\n",time1[i]-1-time1[0],time2[i]+1000000-time2[0],size[i]);
			}else{printf("%ld.%ld %d\n",time1[i]-1-time1[0],time2[i]+1000000-time2[0],size[i]);//experiment3
			}
		}else{
			if(time2[i]-time2[0]<10){printf("%ld.00000%ld %d\n",time1[i]-time1[0],time2[i]-time2[0],size[i]);
			}else if(time2[i]-time2[0]<100){printf("%ld.0000%ld %d\n",time1[i]-time1[0],time2[i]-time2[0],size[i]);
			}else if(time2[i]-time2[0]<1000){printf("%ld.000%ld %d\n",time1[i]-time1[0],time2[i]-time2[0],size[i]);
			}else if(time2[i]-time2[0]<10000){printf("%ld.00%ld %d\n",time1[i]-time1[0],time2[i]-time2[0],size[i]);
			}else if(time2[i]-time2[0]<100000){printf("%ld.0%ld %d\n",time1[i]-time1[0],time2[i]-time2[0],size[i]);
			}else{printf("%ld.%ld %d\n",time1[i]-time1[0],time2[i]-time2[0],size[i]);//experiment3
			}
		}
	}//experiment3

	fclose(fp);
	close(tcpsock_id);
	close(sctpsock_id);
	return 0;
}
