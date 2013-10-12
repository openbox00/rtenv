
#include "stm32f10x.h"
#include "RTOSConfig.h"

#include "syscall.h"

#include <stddef.h>
#include <ctype.h> 

#define MAX_CMDNAME_host 19
#define MAX_ARGC_host 19
#define MAX_CMDHELP_host 1023
#define HISTORY_COUNT_host 20
#define CMDBUF_SIZE_host 100
#define MAX_ENVCOUNT_host 30
#define MAX_ENVNAME_host 15
#define MAX_ENVVALUE_host 127


char next_line_host[3] = {'\n','\r','\0'};
char cmd_host[HISTORY_COUNT_host][CMDBUF_SIZE_host];
int cur_his_host=0;
int fdout_host;
int fin_host;

/* Command handlers. */

void show_cmd_info_host(int argc, char *argv[]);
void show_history_host(int argc, char *argv[]);


/* Enumeration for command types. */
enum {
	CMD_HELP_HOST = 0,
	CMD_HISTORY_HOST,
	CMD_COUNT_HOST
} CMD_TYPE_HOST;

/* Structure for command handler. */

typedef struct {
	char cmd_host[MAX_CMDNAME_host + 1];
	void (*func_host)(int, char**);
	char description_host[MAX_CMDHELP_host + 1];
} hcmd_entry_host;

const hcmd_entry_host cmd_data_host[CMD_COUNT_HOST] = {
	[CMD_HELP_HOST] = {.cmd_host = "h", .func_host = show_cmd_info_host, .description_host = "List all commands you can use."},
	[CMD_HISTORY_HOST] = {.cmd_host = "his", .func_host = show_history_host, .description_host = "Show latest commands entered."}
};

/* Structure for environment variables. */
typedef struct {
	char name[MAX_ENVNAME_host + 1];
	char value[MAX_ENVVALUE_host + 1];
} evar_entry;
evar_entry env_var_host[MAX_ENVCOUNT_host];
int env_count_host = 0;

/* SemihostCall */
enum SemihostReasons {
	// Standard ARM Semihosting Commands:
	Semihost_SYS_OPEN   = 0x1,
	Semihost_SYS_CLOSE  = 0x2,
	Semihost_SYS_WRITE  = 0x5,
	Semihost_SYS_READ   = 0x6,
	Semihost_SYS_ISTTY  = 0x9,
	Semihost_SYS_SEEK   = 0xa,
	Semihost_SYS_ENSURE = 0xb,
	Semihost_SYS_FLEN   = 0xc
};

static inline int SemihostCall(enum SemihostReasons reason, void *arg)
{
	#define AngelSWI     0xAB
	#define AngelSWIInsn  "bkpt"
	int value;
	asm volatile ("mov r0, %1; mov r1, %2; " AngelSWIInsn " %a3; mov %0, r0"
	: "=r" (value) // Outputs
	: "r" (reason), "r" (arg), "i" (AngelSWI) // Inputs
	: "r0", "r1", "r2", "r3", "ip", "lr", "memory", "cc");

	return value;
}

void print_host(char *str){
	write(mq_open("/tmp/mqueue/out", 0), str,strlen(str)+1);
}

char *find_envvar_host(const char *name)
{
	int i;

	for (i = 0; i < env_count_host; i++) {
		if (!strcmp(env_var_host[i].name, name))
			return env_var_host[i].value;
	}

	return NULL;
}

//help
void show_cmd_info_host(int argc, char* argv[])
{
	const char help_desp[] = "Host system has commands as follow\n\r\0";
	int i;

	write(fdout_host, &help_desp, sizeof(help_desp));
	for (i = 0; i < CMD_COUNT_HOST; i++) {
		print_host(cmd_data_host[i].cmd_host);
		print_host("	: ");
		print_host(cmd_data_host[i].description_host);		
		write(fdout_host, next_line_host, 3);
	}
}

void show_history_host(int argc, char *argv[])
{
#if 0
	int i;

	for (i = cur_his_host + 1; i <= cur_his_host + HISTORY_COUNT_host; i++) {
		if (cmd_host[i % HISTORY_COUNT_host][0]) {
			write(fdout_host, cmd_host[i % HISTORY_COUNT_host], strlen(cmd_host[i % HISTORY_COUNT_host]) + 1);
			write(fdout_host, next_line_host, 3);
		}
	}
#endif
}

void find_events_host()
{
	char buf[CMDBUF_SIZE_host];
	char *p = cmd_host[cur_his_host];
	char *q;
	int i;

	for (; *p; p++) {
		if (*p == '!') {
			q = p;
			while (*q && !isspace(*q))
				q++;
			for (i = cur_his_host + HISTORY_COUNT_host - 1; i > cur_his_host; i--) {
				if (!strncmp(cmd_host[i % HISTORY_COUNT_host], p + 1, q - p - 1)) {
					strcpy(buf, q);
					strcpy(p, cmd_host[i % HISTORY_COUNT_host]);
					p += strlen(p);
					strcpy(p--, buf);
					break;
				}
			}
		}
	}
}



/* Fill in entire value of argument. */
int fill_arg_host(char *const dest, const char *argv)
{
	char env_name[MAX_ENVNAME_host + 1];
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
				p = find_envvar_host(env_name);
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
		p = find_envvar_host(env_name);
		if (p) {
			strcpy(buf, p);
			buf += strlen(p);
		}
	}
	*buf = '\0';

	return buf - dest;
}
/* Split command into tokens. */
char *cmdtok_host(char *cmd)
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

void check_keyword_host()
{
	char *argv[MAX_ARGC_host + 1] = {NULL};
	char cmdstr[CMDBUF_SIZE_host];
	char buffer[CMDBUF_SIZE_host * MAX_ENVVALUE_host / 2 + 1];
	char *p = buffer;
	int argc = 1;
	int i;

	find_events_host();
	strcpy(cmdstr, cmd_host[cur_his_host]);
	argv[0] = cmdtok_host(cmdstr);
	if (!argv[0])
		return;

	while (1) {
		argv[argc] = cmdtok_host(NULL);
		if (!argv[argc])
			break;
		argc++;
		if (argc >= MAX_ARGC_host)
			break;
	}

	for(i = 0; i < argc; i++) {
		int l = fill_arg_host(p, argv[i]);
		argv[i] = p;
		p += l + 1;
	}

	for (i = 0; i < CMD_COUNT_HOST; i++) {
		if (!strcmp(argv[0], cmd_data_host[i].cmd_host)) {
			cmd_data_host[i].func_host(argc, argv);
			break;
		}
	}
	if (i == CMD_COUNT_HOST) {
		write(fdout_host, argv[0], strlen(argv[0]) + 1);
		write(fdout_host, ": command not found", 20);
		write(fdout_host, next_line_host, 3);
	}
}


void semishell()
{

	char put_ch[2]={'0','\0'};
	char *p = NULL;

	fdout_host = mq_open("/tmp/mqueue/out", 0);
	fin_host = open("/dev/tty0/in", 0);

	for (;; cur_his_host = (cur_his_host + 1) % HISTORY_COUNT_host) {
		p = cmd_host[cur_his_host];
		print_host("\r");
		print_host("Semishell>>");
	

		while (1) {
			read(fin_host, put_ch, 1);

			if (put_ch[0] == '\r' || put_ch[0] == '\n') {
				*p = '\0';
				write(fdout_host, next_line_host, 3);
				break;
			}
			else if (put_ch[0] == 127 || put_ch[0] == '\b') {
				if (p > cmd_host[cur_his_host]) {
					p--;
					write(fdout_host, "\b \b", 4);
				}
			}
			else if (p - cmd_host[cur_his_host] < CMDBUF_SIZE_host - 1) {
				*p++ = put_ch[0];
				write(fdout_host, put_ch, 2);
			}
		}
	
		check_keyword_host();
		break;
	}

}

