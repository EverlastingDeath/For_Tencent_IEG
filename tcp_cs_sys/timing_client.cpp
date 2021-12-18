#include <sys/types.h>
#include <sys/socket.h>
#include <cstdio>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <ctype.h>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <pthread.h>
#include <vector>
#include <algorithm>

using namespace std;

#define MYPORT  8081
#define BUFFER_SIZE 1024

int para_num = 1;
int tot_num = 0;
char str2bsend[BUFFER_SIZE];
pthread_mutex_t mutex, mutex_time;
vector<double> rp_time;

int get_opt_string(int argc, char* argv[]) {
    printf("Commandline is under processing, please wait\n");
    int status = 0;
    int opt = 0;
    const char *opt_params_string = "c::n:s:";
    while ((opt = getopt(argc, argv, opt_params_string)) != EOF) {
        switch(opt) {
            case 'c': 
                para_num = atoi(optarg);
                break;
            case 'n':
                tot_num = atoi(optarg);
                break;
            case 's':
                char tmp[1024];
                memset(tmp, 0, 1024);
                strcat(tmp, optarg);
                while (argv[optind]) {
                    strcat(tmp," ");
                    strcat(tmp,argv[optind++]);
                }
                strcpy(str2bsend, tmp);
                break;
            default:
                printf("Nonsense param, please check\n");
                status = 1;
                break;
        }
    }
    printf("Commandline has been processed\n");
    return status;
}

void *link_and_send(void *arg) {
    int thrd_num = *((int*)arg);
    printf("Thread %d is running, please wait\n", thrd_num);

    clock_t startTime, endTime;

    while(tot_num != 0){
        startTime = clock();//计时开始 
        int sock_cli = 0;

        sock_cli = socket(AF_INET,SOCK_STREAM, 0);
        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(MYPORT);  ///服务器端口
        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");  ///服务器ip

        //连接服务器
        while (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        {
            printf("Connecting to the server encounterd error\n");
            pthread_exit(0);
        }

        int res = pthread_mutex_lock(&mutex);
        if (res) {
            printf("mutex error");
            pthread_exit(0);
        }
        if (tot_num <= 0) {
            pthread_mutex_unlock(&mutex);
            printf("Thread %d is closing\n", thrd_num);
            pthread_exit(0);
        }
        --tot_num;
        pthread_mutex_unlock(&mutex);

        char sendbuf[BUFFER_SIZE];
        memset(sendbuf, 0, 1024);
        strcpy(sendbuf, str2bsend);

        send(sock_cli, sendbuf, strlen(sendbuf),0); //发送
        char *str = sendbuf;
        printf("msg: [%s] in thread %d has been sent\n", str, thrd_num);
        close(sock_cli);
        endTime = clock();//计时开始
        pthread_mutex_lock(&mutex_time);
        rp_time.push_back(endTime - startTime);
        pthread_mutex_unlock(&mutex_time);
    }
    printf("Thread %d is closing\n", thrd_num);
    pthread_exit(0);
}


int cmp(const void *p1, const void *p2)
{
	return *(int*)p1 - *(int*)p2;
}

int test_log(int whole_num) {
    sort(rp_time.begin(), rp_time.end());
    double sum = 0.0;
    int n = rp_time.size();
    vector<double> percent;
    for (int i = n - 1; i >= 0; --i) {
        sum += rp_time[i];
        if (i < 0.5 * n) {
            percent.push_back(rp_time[i]);
            continue;
        }
        if (i < 0.6 * n) {
            percent.push_back(rp_time[i]);
            continue;
        } 
        if (i < 0.7 * n) {
            percent.push_back(rp_time[i]);
            continue;
        } 
        if (i < 0.8 * n) {
            percent.push_back(rp_time[i]);
            continue;
        } 
        if (i < 0.9 * n) {
            percent.push_back(rp_time[i]);
            continue;
        } 
        if (i < n) {
            percent.push_back(rp_time[i]);
            continue;
        } 
    }
    //printf("%lf", sum);
    //printf("%d!!!!!!!!!!!!!!!\n", whole_num);
    double m_time = sum / (double)whole_num;
    //printf("%lf!!!!!!!!!!!!!\n", m_time);
    double qps = para_num / m_time * 1000;
    printf("QPS: %lf [fetches/sec]\n", qps);
    printf("Percentage ofthe requests served within a certain time(ms)\n");
    printf("P50: %lf\n", percent[5]);
    printf("P60: %lf\n", percent[4]);
    printf("P70: %lf\n", percent[3]);
    printf("P80: %lf\n", percent[2]);
    printf("P90: %lf\n", percent[1]);
    printf("P100: %lf\n", percent[0]);
    return(0);
}

int main(int argc, char *argv[]) {
    time_t now_time;
    struct tm *p;
    time(&now_time);
    p = localtime(&now_time);
    struct timeval tv;
    gettimeofday(&tv,NULL);
    
    int status = 0;
    status = get_opt_string(argc, argv);
    if (status) {
        printf("Command processing error");
        exit(0);
    }

    int whole_num = tot_num;
    pthread_t thread[para_num];
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutex_time, NULL);
    for (int i = 0; i < para_num; ++i) {
        void* tmp = &i;
        status = pthread_create(&thread[i], NULL, link_and_send, tmp);
        if (status) {
            printf("Fail to create thread %d\n", i);
            exit(0);
        }
    }

    void *thrd_ret;
    for (int i = 0; i < para_num; ++i) {
        status = pthread_join(thread[i], &thrd_ret);
        if (status) {
            printf("Failed to join the thread %d\n", i);
            exit(0);
        }
    }

    printf("test start time: %02d:%02d:%02d:%ld\n", p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec / 1000);
    time_t end_time;
    struct tm *p_end;
    time(&end_time);
    p_end = localtime(&end_time);
    gettimeofday(&tv,NULL);
    printf("test end time: %02d:%02d:%02d:%ld\n", p_end->tm_hour, p_end->tm_min, p_end->tm_sec, tv.tv_usec / 1000);

    status = test_log(whole_num);
    if (status) {
        printf("Print log error\n");
        exit(0);
    }

    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutex_time);
    return 0;
}