/**
* gcc -o server server.c -lswoole
*/
#include "Server.h"
#include <google/profiler.h>

int my_onReceive(swFactory *factory, swEventData *req);
char* rtrim(char *str, int len);
void my_onStart(swServer *serv);
void my_onShutdown(swServer *serv);
void my_onConnect(swServer *serv, int fd, int from_id);
void my_onClose(swServer *serv, int fd, int from_id);
void my_onTimer(swServer *serv, int interval);
void my_onWorkerStart(swServer *serv, int worker_id);
void my_onWorkerStop(swServer *serv, int worker_id);
int my_onControlEvent(swFactory *factory, swEventData *event);

static int g_receive_count = 0;
static int g_controller_id = 0;

char* rtrim(char *str, int len)
{
	int i;
	for (i = len; i > 0; i--)
	{
		switch(str[i])
		{
		case ' ':
		case '\0':
		case '\n':
		case '\r':
		case '\t':
		case '\v':
			str[i] = 0;
			break;
		default:
			return str;
		}
	}
	return str;
}


int main(int argc, char **argv)
{
	int ret;
	swServer serv;
	swServer_init(&serv); //初始化

	//config
	serv.backlog = 128;
	serv.poll_thread_num = 2; //reactor线程数量
	serv.writer_num = 2;      //writer线程数量
	serv.worker_num = 2;      //worker进程数量

	serv.factory_mode = SW_MODE_PROCESS; //SW_MODE_PROCESS SW_MODE_THREAD SW_MODE_BASE
	serv.max_conn = 1000;
	//serv.open_cpu_affinity = 1;
	//serv.open_tcp_nodelay = 1;
	//serv.daemonize = 1;
	serv.open_eof_check = 0;
	memcpy(serv.data_eof, SW_STRL("\r\n\r\n")-1);      //开启eof检测，启用buffer区
//	memcpy(serv.log_file, SW_STRL("/tmp/swoole.log")); //日志

	serv.dispatch_mode = 2;
	serv.open_tcp_keepalive = 1;

	serv.onStart = my_onStart;
	serv.onShutdown = my_onShutdown;
	serv.onConnect = my_onConnect;
	serv.onReceive = my_onReceive;
	serv.onClose = my_onClose;
	serv.onTimer = my_onTimer;
	serv.onWorkerStart = my_onWorkerStart;
	serv.onWorkerStop = my_onWorkerStop;

	//create Server
	ret = swServer_create(&serv);
	if (ret < 0)
	{
		swTrace("create server fail[error=%d].\n", ret);
		exit(0);
	}
	//swServer_addListen(&serv, SW_SOCK_UDP, "127.0.0.1", 9500);
	swServer_addListen(&serv, SW_SOCK_TCP, "127.0.0.1", 9501);
	//swServer_addListen(&serv, SW_SOCK_UDP, "127.0.0.1", 9502);
	//swServer_addListen(&serv, SW_SOCK_UDP, "127.0.0.1", 8888);

	//swServer_addTimer(&serv, 2);
	//swServer_addTimer(&serv, 4);

//	g_controller_id = serv.factory.controller(&serv.factory, my_onControlEvent);
	ret = swServer_start(&serv);
	if (ret < 0)
	{
		swTrace("start server fail[error=%d].\n", ret);
		exit(0);
	}
	return 0;
}

int my_onControlEvent(swFactory *factory, swEventData *event)
{
	printf("Event: type=%d|data=%s\n", event->info.type, event->data);
	return SW_OK;
}

void my_onWorkerStart(swServer *serv, int worker_id)
{
	printf("WorkerStart[%d]PID=%d\n", worker_id, getpid());
}

void my_onWorkerStop(swServer *serv, int worker_id)
{
	printf("WorkerStop[%d]PID=%d\n", worker_id, getpid());
}

void my_onTimer(swServer *serv, int interval)
{
	printf("Timer Interval=[%d]\n", interval);
}

int my_onReceive(swFactory *factory, swEventData *req)
{
	int ret;
	char resp_data[SW_BUFFER_SIZE];
	swServer *serv = factory->ptr;
	swConnection *conn = swServer_get_connection(serv, req->info.fd);

	swSendData resp;
	g_receive_count ++;
	resp.info.fd = req->info.fd; //fd can be not source fd.
	resp.info.len = req->info.len + 8;
	resp.info.from_id = req->info.from_id;
	req->data[req->info.len] = 0;

	snprintf(resp_data, resp.info.len, "Server:%s", req->data);
	resp.data = resp_data;
	ret = factory->finish(factory, &resp);
	if (ret < 0)
	{
		printf("send to client fail.errno=%d\n", errno);
	}
//	printf("onReceive[%d]: ip=%s|port=%d Data=%s|Len=%d\n", g_receive_count,
//			inet_ntoa(conn->addr.sin_addr), conn->addr.sin_port,
//			rtrim(req->data, req->info.len), req->info.len);
//	req->info.type = 99;
//	factory->event(factory, g_controller_id, req);
	return SW_OK;
}

void my_onStart(swServer *serv)
{
	sw_log("Server is running");
}

void my_onShutdown(swServer *serv)
{
	sw_log("Server is shutdown\n");
}

void my_onConnect(swServer *serv, int fd, int from_id)
{
//	ProfilerStart("/tmp/profile.prof");
	printf("PID=%d\tConnect fd=%d|from_id=%d\n", getpid(), fd, from_id);
}

void my_onClose(swServer *serv, int fd, int from_id)
{
	printf("PID=%d\tClose fd=%d|from_id=%d\n", getpid(), fd, from_id);
//	ProfilerStop();
}
