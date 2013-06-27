#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include "util/iniparser.h"
#include "util/dwg_util.h"
#include "util/dwg.h"

dictionary * configs;
char * pid_file;
static int _connected	= 0;

char* getTime()
{
	time_t now;
	struct tm *ptr;
	static char tbuf[64];
	bzero(tbuf,64);
	time(&now);
	ptr = localtime(&now);
	strftime(tbuf,64, "[%Y-%m-%e %H:%M:%S] ", ptr);
	return tbuf;
}

char* getsTime()
{
	time_t now;
	struct tm *ptr;
	static char tbuf[64];
	bzero(tbuf,64);
	time(&now);
	ptr = localtime(&now);
	strftime(tbuf,64, ".%e%m%Y%H%M%S", ptr);
	return tbuf;
}

void trim(char *str)
{
    int i;
    int begin = 0;
    int end = strlen(str) - 1;
    while (isspace(str[begin]))
      begin++;
    while (isspace(str[end]) && (end >= begin))
      end--;
    for (i = begin; i <= end; i++)
      str[i - begin] = str[i];
    str[i - begin] = '\0'; // Null terminate string.
}


void WriteLog(char* Msg, char* Repl)
{
	FILE* f;
	f = fopen(iniparser_getstring(configs, "config:log", "/var/log/dwg.log"), "a");
	if (f)
	{
		char LogMsg[200];
		bzero(LogMsg, 200);
		strcpy(LogMsg, getTime());
		strcat(LogMsg, Msg);
		fprintf(f, LogMsg, Repl);
		fclose(f);
	}
}

/**************************************
 * For DWG 														*
 **************************************/
void status_handler(str_t *gw_ip, dwg_ports_status_t *status)
{
	_connected = 1;
}

void new_sms_handler(str_t *gw_ip, dwg_sms_received_t *sms)
{
	char * incoming_path;
	char * run_prog;
	int int_ret_code;
	char ret_code[10];
	char full_path[200];
	FILE* f;
	
	incoming_path = iniparser_getstring(configs, "config:incoming", "/var/spool/dwg/incoming/");
	strcpy(full_path, incoming_path);
	strcat(full_path, sms->str_number.s);
	strcat(full_path, getsTime());
	
	f = fopen(full_path, "w+");
	if (f != NULL)
	{
		fprintf(f, "From: %.*s\n", sms->str_number.len, sms->str_number.s);
		fprintf(f, "Encoding: %d\n", sms->encoding);
		fprintf(f, "Sent: %s\n", sms->timestamp);
		fprintf(f, "Timezone: %d\n", sms->timezone);
		fprintf(f, "\n");
		fprintf(f, "%.*s", sms->message.len, sms->message.s);
		fclose(f);
		WriteLog("[PROCESS] Receiving SMS from #%s\n", sms->str_number.s);
		
		//Ext program
		run_prog = iniparser_getstring(configs, "config:run", NULL);
		if (run_prog != NULL)
		{
			int_ret_code = system(run_prog);
			sprintf(ret_code, "%d", int_ret_code);
			WriteLog("[PROCESS] Run external program. Return code: %s\n", ret_code);
		}
	}
}

void FinishDaemon(int i)
{
	unlink(pid_file);
	WriteLog("[DAEMON] Stopped\n", "");
	exit(0);
}

void SetPidFile(char* Filename)
{
  FILE* f;
  f = fopen(Filename, "w+");
  if (f)
  {
    fprintf(f, "%u", getpid());
    fclose(f);
  }
}

void SendSMS(char* Filename)
{
	char number[20];
	char message[200] = "";
	char tmp_message[200];
	FILE* f;
	
	f = fopen(Filename, "r");
	if (f == NULL)
	{
		WriteLog("[ERROR] Sending SMS: %s\n", Filename);
	}
	else
	{
		fgets(number, sizeof(number), f); trim(number);
		while(!feof(f))
		{
			fgets(tmp_message, sizeof(tmp_message), f); trim(tmp_message);
			strcat(message, tmp_message);
			strcat(message, "\n");
		}
		trim(message);
		str_t des = { number, strlen(number) };
		str_t msg = { message, strlen(message) };
		dwg_send_sms(&des, &msg, rand()%8);
		WriteLog("[PROCESS] Sending SMS to #%s\n", number);
		fclose(f);
	}
}

void SMSd()
{
	char * send_path;
	char temp_path[200];
	DIR *dir;
	struct dirent *entry;
	struct sigaction sa;
	sigset_t sigset;
	
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGHUP);
	sigprocmask(SIG_BLOCK, &sigset, 0);
	sa.sa_handler = FinishDaemon;
	sigaction(SIGTERM, &sa, 0);
	
	send_path = iniparser_getstring(configs, "config:send", "/var/spool/dwg/send/");
	SetPidFile(pid_file);
	WriteLog("[DAEMON] Started\n", "");
	
	//Prepare dwg
	dwg_message_callback_t callbacks	= {
				.status_callback 		= status_handler,
				.msg_sms_recv_callback	= new_sms_handler
	};
	int port = iniparser_getint(configs, "config:port", 12000);
	dwg_start_server(port, &callbacks);
	do
	{
		sleep(1);
	}
	while(_connected == 0);
	WriteLog("[SYSTEM] Gateway connected\n", "");
	
	for (;;)
	{
		dir = opendir(send_path);
		if (dir == NULL)
		{
			WriteLog("[ERROR] Opening dir: %s\n", send_path);
		}
		else
		{
			while ((entry = readdir(dir)) != NULL)
			{
  			strcpy(temp_path, send_path);
				if (strcmp(entry->d_name, "..") && strcmp(entry->d_name, "."))
				{
					strcat(temp_path, entry->d_name);
					SendSMS(temp_path);
					unlink(temp_path);
				}
			}
			closedir(dir);
		}
		sleep(10);
	}
	unlink(pid_file);
	WriteLog("[DAEMON] Stopped\n", "");
	exit(0);
}

void LoadConfig(char* Filename)
{
	configs = iniparser_load(Filename);
	if (configs==NULL)
	{
		fprintf(stderr, "Error loading config file\n");
		exit(1);
	}
	pid_file = iniparser_getstring(configs, "config:pid", "/var/run/dwgd.pid");
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Invalid usage:\n\t./dwgd </path/to/config_file>\n");
		exit(1);
	}
	
	LoadConfig(argv[1]);
	
	int pid = fork();
	switch (pid)
	{
		case 0:
			umask(0);
			setsid();
			chdir("/");
			close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
			SMSd();
			exit(0);
		case -1:
			printf("Fail: unable to fork!\n");
		default:
			printf("OK: daemon with pid %d is created\n", pid);
			break;
	}
	return 0;
}