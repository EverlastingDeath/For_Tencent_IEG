#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <ctime>
#include <sys/time.h>
#include <uuid/uuid.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

using namespace std;

typedef struct socketinfo
{
    int fd;
    int epfd;
    sockaddr_in cli;
    socklen_t clilen;
}socktinfo;

sem_t com_trd_max, acc_trd_max;
pthread_mutex_t print_lock;

void access_log (int fd, sockaddr_in cli, socklen_t clilen, char *buf) {
	clock_t startTime, endTime;
    time_t now_time, end_time;
    struct tm *p, *p_end;
    time(&now_time);
    p = localtime(&now_time);
    struct timeval tv;
    gettimeofday(&tv,NULL);
	//printf("[%d:%d:%d:%02d:%02d:%02d:%d]", p->tm_year + 1900, p->tm_mon, p->tm_mday ,p->tm_hour, p->tm_min, p->tm_sec, (int)tv.tv_usec / 1000);
	if(getsockname(fd, (struct sockaddr *)&cli, &clilen) == -1){
		printf("getsockname error\n");
		return;
	}
	//printf("recv %s, ",inet_ntoa(cli.sin_addr));
	uuid_t uu;
	uuid_generate(uu);
	char uuid_str[37];
	uuid_unparse_lower(uu, uuid_str);
	//printf("msgid: %s", uuid_str);
	//printf("msgid: %s, ", uu.substring(0,15));
	//printf("msgcontent: \"%s\"\n", buf);
    printf("[%d:%d:%d:%02d:%02d:%02d:%d] recv %s, msgid: %s, msgcontent: \"%s\"\n", 
    p->tm_year + 1900, p->tm_mon, p->tm_mday ,p->tm_hour, p->tm_min, p->tm_sec, (int)tv.tv_usec / 1000,
    inet_ntoa(cli.sin_addr),
    uuid_str,
    buf);
}

void *accept_trd(void *arg) {
    socketinfo *soc = (socktinfo*) arg;
    int fd = soc->fd;
    sockaddr_in cli = soc->cli;
    socklen_t clilen = soc->clilen;
    int epfd = soc->epfd;

    int cfd = accept(fd, (struct sockaddr *)&cli, &clilen);
    int flag = fcntl(cfd, F_GETFL);
    flag |= fcntl(cfd, O_NONBLOCK);
    // 如果成功，送读事件上树
    struct epoll_event ev;
    if(cfd>0){
        ev.data.fd = cfd;
        ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
    }
    sem_post(&acc_trd_max);
    free(soc);
    return NULL;
}

void *commu_trd(void *arg) {
        socketinfo *soc = (socktinfo*) arg;
        //struct epoll_event ev;
        int fd = soc->fd;
        sockaddr_in cli = soc->cli;
        socklen_t clilen = soc->clilen;
        int epfd = soc->epfd;
        /*if(getsockname(fd, (struct sockaddr *)&cli, &clilen) == -1) {
		    printf("getsockname error!!!!!!!!!!!!!!!!!!\n");
	    }
        printf("recv %s!!!!!!!!!!!!!!!!!!!\n",inet_ntoa(cli.sin_addr));*/
        char buf[1024]={0};
        while (1) {
            int len = recv(fd,buf,sizeof(buf), 0);
            if(len == -1){
                perror("recv error");
                break;
            }
            else if(len == 0){
                epoll_ctl(epfd,EPOLL_CTL_DEL,fd, NULL);
                close(fd);
                break;
            }
            else {
                access_log(fd, cli, clilen, buf);
                break;
            }   
        }
    free(soc);
    sem_post(&com_trd_max);
    return NULL;
}

int main(){
sem_init(&acc_trd_max, 1, 32);
sem_init(&com_trd_max, 1, 32);

// 三部曲
int lfd=socket(AF_INET,SOCK_STREAM,0);

struct sockaddr_in svr,cli;
socklen_t clilen = sizeof(cli);
memset(&svr,0x00,sizeof(svr));
svr.sin_family = AF_INET;
svr.sin_port = htons(8081);
svr.sin_addr.s_addr = 0 ;

int opt=1;
setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

bind(lfd,(struct sockaddr*)&svr,sizeof(svr));

listen(lfd,128);

//建树
int epfd=epoll_create(1);

//挂树
struct epoll_event ev,evs[128];
ev.data.fd=lfd;
ev.events=EPOLLIN;
epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);

// 循环等待
while(1){
    
    int s=epoll_wait(epfd,evs,1024,-1);
    for(int i=0;i<s;i++){
        pthread_t tid;
        socketinfo *info = (socketinfo*)malloc(sizeof(socketinfo));
        int fd = evs[i].data.fd;
        info->fd = fd;
        info->epfd = epfd;
        info->cli = cli;
        info->clilen = clilen;
        //连接事件
        if(evs[i].events & EPOLLIN){
            if(evs[i].data.fd == lfd){
                sem_wait(&acc_trd_max);
                pthread_create(&tid, NULL, accept_trd, (void*)info);
                pthread_detach(tid);
            }
            //读事件
            else{
            /*if(getsockname(info->fd, (struct sockaddr *)&info->cli, &info->clilen ) == -1) {
		        printf("getsockname error!!!!!!!!!!!!!!!!!!\n");
	        }
            printf("recv %s!!!!!!!!!!!!!!!!!!!\n",inet_ntoa(info->cli.sin_addr));*/
                sem_wait(&com_trd_max);
                pthread_create(&tid, NULL, commu_trd, (void*)info);
                pthread_detach(tid);
            }
        }
    }
	}
close(epfd);
close(lfd);
return 0;
}
