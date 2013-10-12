/*
* This Shell is ref School5510587,and separate shell function is ref zzz0072
*/
#include "stm32f10x.h"
#include "RTOSConfig.h"

#include "syscall.h"

#include <stddef.h>
#include "systemdef.h"
#include <ctype.h> 

#include "semi.h"


extern struct task_info task_info_t;

#define MAX_CMDNAME 19
#define MAX_ARGC 19
#define MAX_CMDHELP 1023
#define HISTORY_COUNT 20
#define CMDBUF_SIZE 100
#define MAX_ENVCOUNT 30
#define MAX_ENVNAME 15
#define MAX_ENVVALUE 127

#define Backspace 127

char next_line[3] = {'\n','\r','\0'};
char cmd[HISTORY_COUNT][CMDBUF_SIZE];
int cur_his=0;

/* Command handlers. */
void export_envvar(int argc, char *argv[]);
void show_echo(int argc, char *argv[]);
void show_cmd_info(int argc, char *argv[]);
void show_task_info(int argc, char *argv[]);
void show_man_page(int argc, char *argv[]);
void show_history(int argc, char *argv[]);
void semihost_function(int argc, char *argv[]);

/* Enumeration for command types. */
enum {
	CMD_ECHO = 0,
	CMD_EXPORT,
	CMD_HELP,
	CMD_HISTORY,
	CMD_MAN,
	CMD_PS,
	CMD_SYSCALL,
	CMD_COUNT
} CMD_TYPE;
/* Structure for command handler. */
typedef struct {
	char cmd[MAX_CMDNAME + 1];
	void (*func)(int, char**);
	char description[MAX_CMDHELP + 1];
} hcmd_entry;
const hcmd_entry cmd_data[CMD_COUNT] = {
	[CMD_ECHO] = {.cmd = "echo", .func = show_echo, .description = "Show words you input."},
	[CMD_EXPORT] = {.cmd = "export", .func = export_envvar, .description = "Export environment variables."},
	[CMD_HELP] = {.cmd = "help", .func = show_cmd_info, .description = "List all commands you can use."},
	[CMD_HISTORY] = {.cmd = "history", .func = show_history, .description = "Show latest commands entered."}, 
	[CMD_MAN] = {.cmd = "man", .func = show_man_page, .description = "Manual pager."},
	[CMD_PS] = {.cmd = "ps", .func = show_task_info, .description = "List all the processes."},
	[CMD_SYSCALL] = {.cmd = "system", .func = semihost_function, .description = "do Host system call."}
};

/* Structure for environment variables. */
typedef struct {
	char name[MAX_ENVNAME + 1];
	char value[MAX_ENVVALUE + 1];
} evar_entry;
evar_entry env_var[MAX_ENVCOUNT];
int env_count = 0;

void print(char *str){
	write(mq_open("/tmp/mqueue/out", 0), str,strlen(str)+1);
}

void semihost_function(int argc, char *argv[]){
	semishell();
}

char *find_envvar(const char *name)
{
	int i;

	for (i = 0; i < env_count; i++) {
		if (!strcmp(env_var[i].name, name))
			return env_var[i].value;
	}

	return NULL;
}

//export
void export_envvar(int argc, char *argv[])
{
	char *found;
	char *value;
	int i;

	for (i = 1; i < argc; i++) {
		value = argv[i];
		while (*value && *value != '=')
			value++;
		if (*value)
			*value++ = '\0';
		found = find_envvar(argv[i]);
		if (found)
			strcpy(found, value);
		else if (env_count < MAX_ENVCOUNT) {
			strcpy(env_var[env_count].name, argv[i]);
			strcpy(env_var[env_count].value, value);
			env_count++;
		}
	}
}

char *statetran(int i){
	switch(i){
	case 0x0:return	"TASK_READY	";
	case 0x1:return	"TASK_WAIT_READ";
	case 0x2:return	"TASK_WAIT_WRITE";
	case 0x3:return	"TASK_WAIT_INTR";	
	case 0x4:return	"TASK_WAIT_TIME	";
	default:return "Unknow	";
	}
}

//this function helps to show int
void itoa(int n, char *buffer)
{
	if (n == 0)
		*(buffer++) = '0';
	else {
		int f = 10000;

		if (n < 0) {
			*(buffer++) = '-';
			n = -n;
		}

		while (f != 0) {
			int i = n / f;
			if (i != 0) {
				*(buffer++) = '0'+(i%10);;
			}
			f/=10;
		}
	}
	*buffer = '\0';
}

//ps
void show_task_info(int argc, char* argv[])
{
	int task_i = 0;

	for (task_i; task_i < *(task_info_t.task_count); task_i++) {
		char task_info_pid[2];
		char task_info_priority[3];
		print("Task no = ");
		itoa(task_info_t.tasks[task_i].pid,task_info_pid);
		print(&task_info_pid);
		print("\t;Status = ");
		print(statetran(task_info_t.tasks[task_i].status));
		print("\t;Priority = ");
		itoa(task_info_t.tasks[task_i].priority,task_info_priority);
		print(&task_info_priority);
		write(mq_open("/tmp/mqueue/out", 0), next_line , 3);
	}
}

//help
void show_cmd_info(int argc, char* argv[])
{
	const char help_desp[] = "This system has commands as follow\n\r\0";
	int i;

	write(mq_open("/tmp/mqueue/out", 0), &help_desp, sizeof(help_desp));
	for (i = 0; i < CMD_COUNT; i++) {
		write(mq_open("/tmp/mqueue/out", 0), cmd_data[i].cmd, strlen(cmd_data[i].cmd) + 1);
		write(mq_open("/tmp/mqueue/out", 0), ": ", 3);
		write(mq_open("/tmp/mqueue/out", 0), cmd_data[i].description, strlen(cmd_data[i].description) + 1);
		write(mq_open("/tmp/mqueue/out", 0), next_line, 3);
	}
}

//echo
void show_echo(int argc, char* argv[])
{
	const int _n = 1; /* Flag for "-n" option. */
	int flag = 0;
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-n"))
			flag |= _n;
		else
			break;
	}

	for (; i < argc; i++) {
		write(mq_open("/tmp/mqueue/out", 0), argv[i], strlen(argv[i]) + 1);
		if (i < argc - 1)
			write(mq_open("/tmp/mqueue/out", 0), " ", 2);
	}

	if (~flag & _n)
		write(mq_open("/tmp/mqueue/out", 0), next_line, 3);
}

//man
void show_man_page(int argc, char *argv[])
{
	int i;

	if (argc < 2)
		return;

	for (i = 0; i < CMD_COUNT && strcmp(cmd_data[i].cmd, argv[1]); i++)
		;

	if (i >= CMD_COUNT)
		return;

	write(mq_open("/tmp/mqueue/out", 0), "NAME: ", 7);
	write(mq_open("/tmp/mqueue/out", 0), cmd_data[i].cmd, strlen(cmd_data[i].cmd) + 1);
	write(mq_open("/tmp/mqueue/out", 0), next_line, 3);
	write(mq_open("/tmp/mqueue/out", 0), "DESCRIPTION: ", 14);
	write(mq_open("/tmp/mqueue/out", 0), cmd_data[i].description, strlen(cmd_data[i].description) + 1);
	write(mq_open("/tmp/mqueue/out", 0), next_line, 3);
}

void show_history(int argc, char *argv[])
{
	int i;

	for (i = cur_his + 1; i <= cur_his + HISTORY_COUNT; i++) {
		if (cmd[i % HISTORY_COUNT][0]) {
			write(mq_open("/tmp/mqueue/out", 0), cmd[i % HISTORY_COUNT], strlen(cmd[i % HISTORY_COUNT]) + 1);
			write(mq_open("/tmp/mqueue/out", 0), next_line, 3);
		}
	}
}

int write_blank(int blank_num)
{
	char blank[] = " ";
	int blank_count = 0;

	while (blank_count <= blank_num) {
		write(mq_open("/tmp/mqueue/out", 0), blank, sizeof(blank));
		blank_count++;
	}
}

void find_events()
{
	char buf[CMDBUF_SIZE];
	char *p = cmd[cur_his];
	char *q;
	int i;

	for (; *p; p++) {
		if (*p == '!') {
			q = p;
			while (*q && !isspace(*q))
				q++;
			for (i = cur_his + HISTORY_COUNT - 1; i > cur_his; i--) {
				if (!strncmp(cmd[i % HISTORY_COUNT], p + 1, q - p - 1)) {
					strcpy(buf, q);
					strcpy(p, cmd[i % HISTORY_COUNT]);
					p += strlen(p);
					strcpy(p--, buf);
					break;
				}
			}
		}
	}
}



/* Fill in entire value of argument. */
int fill_arg(char *const dest, const char *argv)
{
	char env_name[MAX_ENVNAME + 1];
	char *buf = dest;
	char *p = NULL;

	for (; *argv; argv++) {
		if (isalnum(*argv) || *argv == '_') {
			if (p)
				*p++ = *argv;
			else
				*buf++ = *argv;
		}
		else { /* Symbols. */
			if (p) {
				*p = '\0';
				p = find_envvar(env_name);
				if (p) {
					strcpy(buf, p);
					buf += strlen(p);
					p = NULL;
				}
			}
			if (*argv == '$')
				p = env_name;
			else
				*buf++ = *argv;
		}
	}
	if (p) {
		*p = '\0';
		p = find_envvar(env_name);
		if (p) {
			strcpy(buf, p);
			buf += strlen(p);
		}
	}
	*buf = '\0';

	return buf - dest;
}
/* Split command into tokens. */
char *cmdtok(char *cmd)
{
	static char *cur = NULL;
	static char *end = NULL;
	if (cmd) {
		char quo = '\0';
		cur = cmd;
		for (end = cmd; *end; end++) {
			if (*end == '\'' || *end == '\"') {
				if (quo == *end)
					quo = '\0';
				else if (quo == '\0')
					quo = *end;
				*end = '\0';
			}
			else if (isspace(*end) && !quo)
				*end = '\0';
		}
	}
	else
		for (; *cur; cur++)
			;

	for (; *cur == '\0'; cur++)
		if (cur == end) return NULL;
	return cur;
}

void check_keyword()
{
	char *argv[MAX_ARGC + 1] = {NULL};
	char cmdstr[CMDBUF_SIZE];
	char buffer[CMDBUF_SIZE * MAX_ENVVALUE / 2 + 1];
	char *p = buffer;
	int argc = 1;
	int i;

	find_events();
	strcpy(cmdstr, cmd[cur_his]);
	argv[0] = cmdtok(cmdstr);
	if (!argv[0])
		return;

	while (1) {
		argv[argc] = cmdtok(NULL);
		if (!argv[argc])
			break;
		argc++;
		if (argc >= MAX_ARGC)
			break;
	}

	for(i = 0; i < argc; i++) {
		int l = fill_arg(p, argv[i]);
		argv[i] = p;
		p += l + 1;
	}

	for (i = 0; i < CMD_COUNT; i++) {
		if (!strcmp(argv[0], cmd_data[i].cmd)) {
			cmd_data[i].func(argc, argv);
			break;
		}
	}
	if (i == CMD_COUNT) {
		write(mq_open("/tmp/mqueue/out", 0), argv[0], strlen(argv[0]) + 1);
		write(mq_open("/tmp/mqueue/out", 0), ": command not found", 20);
		write(mq_open("/tmp/mqueue/out", 0), next_line, 3);
	}
}

void shell()
{
	char put_ch[2]={'0','\0'};
	char *p = NULL;

	for (;; cur_his = (cur_his + 1) % HISTORY_COUNT) {
		p = cmd[cur_his];
		print("Shell>>");

		while (1) {
			read(open("/dev/tty0/in", 0), put_ch, 1);

			if (put_ch[0] == '\r' || put_ch[0] == '\n') {
				*p = '\0';
				print(next_line);
				break;
			}
			else if (put_ch[0] == Backspace || put_ch[0] == '\b') {
				if (p > cmd[cur_his]) {
					p--;
					print("\b \b");
				}
			}
			else if (p - cmd[cur_his] < CMDBUF_SIZE - 1) {
				*p++ = put_ch[0];
				print(put_ch);
			}
		}
		check_keyword();	
	}
}


