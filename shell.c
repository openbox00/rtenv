#include "stm32f10x.h"
#include "RTOSConfig.h"

#include "syscall.h"

#include <stddef.h>
#include <ctype.h> 

/* print */
#include "print.h"

#define MAX_ARGC 9
#define MAX_CMDNAME 19
#define MAX_CMDHELP 100
#define HISTORY_COUNT 5
#define CMDBUF_SIZE 50
#define MAX_ENVVALUE 127
#define MAX_ENVNAME 15

extern struct task_info task_info_t;

char next_line[3] = {'\n','\r','\0'};
char cmd[HISTORY_COUNT][CMDBUF_SIZE];
int cur_his=0;

/* Command handlers. */
void show_cmd_info(int argc, char *argv[]);
void show_echo(int argc, char* argv[]);
void show_history(int argc, char *argv[]);
void show_task_info(int argc, char* argv[]);
void show_mem_info(int argc, char* argv[]);

/* Enumeration for command types. */
enum {
	CMD_HELP = 0,
	CMD_HISTORY,
	CMD_ECHO,
	CMD_PS,
	CMD_MEMINFO,
	CMD_COUNT
} CMD_TYPE;

/* Structure for command handler. */
typedef struct {
	char cmd[MAX_CMDNAME + 1];
	void (*func)(int, char**);
	char description[MAX_CMDHELP + 1];
} hcmd_entry;

const hcmd_entry cmd_data[CMD_COUNT] = {
	[CMD_HELP] = {.cmd = "help", .func = show_cmd_info, .description = "List all commands you can use."},	
	[CMD_HISTORY] = {.cmd = "history", .func = show_history, .description = "Show latest commands entered."}, 
	[CMD_ECHO] = {.cmd = "echo", .func = show_echo, .description = "Show words you input."},
	[CMD_PS] = {.cmd = "ps", .func = show_task_info, .description = "List all the processes."},
	[CMD_MEMINFO] = {.cmd = "meminfo", .func = show_mem_info, .description = "Show memory info."}	
};

/* meminfo  Ref PJayChen*/
void show_mem_info(int argc, char* argv[])
{
#if 0	
	char ch;
	/* 38 = & */
	if (argv[1][0] == '&'){
		printf("Maximun size  : %d (0x%x) byte\n\r", configTOTAL_HEAP_SIZE, configTOTAL_HEAP_SIZE);
		printf("Free Heap Size: %d (0x%x) byte\n\r", xPortGetFreeHeapSize(), xPortGetFreeHeapSize());
		while (1){
			if (receive_byte_noblock(&ch) == 1){ 
				printf("DONE\n\r"); 
	       		break;        
			}
		}		
	}
	else {
		printf("Maximun size  : %d (0x%x) byte\n\r", configTOTAL_HEAP_SIZE, configTOTAL_HEAP_SIZE);
		printf("Free Heap Size: %d (0x%x) byte\n\r", xPortGetFreeHeapSize(), xPortGetFreeHeapSize());
	}
#endif
}

/*ref ref zzz0072, PJayChen*/
void show_task_info(int argc, char* argv[])
{
#if 0
	char ch;
	/* 38 = & */
	if (argv[1][0] == '&'){
		/*use hardcoded array*/
		portCHAR buf[MAX_CMDHELP];
		vTaskList(buf);
		printf("Name\t\tState Priority Stack\tNum");
		printf("%s", buf); 
		while (1){
			if (receive_byte_noblock(&ch) == 1){ 
				printf("DONE\n\r"); 
        		break;
			}        
		}		
	}
	else {	
		/*use hardcoded array*/
		portCHAR buf[MAX_CMDHELP];
		vTaskList(buf);
		printf("Name\t\tState Priority Stack\tNum");
		printf("%s", buf); 
	}
#endif
}

//echo but can't solve '' "" situation 
void show_echo(int argc, char* argv[])
{
	int i = 1;
	int j = 0;
	for(;i<argc;i++){
		printf("%s ",argv[i]);	
	}
	printf("\n");
}

//history
void show_history(int argc, char *argv[])
{
	int i;

		for (i = cur_his+1; i <= cur_his + HISTORY_COUNT; i++) {
			if (cmd[i % HISTORY_COUNT][0]) {
				printf("%s", cmd[i % HISTORY_COUNT]);
				printf("%s",next_line);
			}
		}
}


//help
void show_cmd_info(int argc, char* argv[])
{
	char *help_desp = "This system has commands as follow\n\r\0";
	int i;

		printf("%s",help_desp);
		for (i = 0; i < CMD_COUNT; i++) {
			printf("%s",cmd_data[i].cmd);
			printf("\t: ");
			printf("%s",cmd_data[i].description);
			printf("%s",next_line);
		}
		
}

/* ref tim37021 */
int cmdtok(char *argv[], char *cmd)
{
	char tmp[CMDBUF_SIZE];
	int i = 0;
	int j = 0;
	int flag;

	int x = -1;
	
	while (*cmd != '\0'){
		if(*cmd == ' '){
			cmd++;
		}
		else {
			while (1) {
				/* solve "" & '' in echo command*/
				if ((*cmd == '\'') || (*cmd == '\"')){
					cmd++;
				}
				else if ((*cmd != ' ') && (*cmd != '\0')){
					tmp[i++] = *cmd;
					cmd++;
				}
				else { 
				tmp[i] = '\0';
				i = 0;
				break;
				}		
			}
			strcpy(argv[j++],tmp);
		}
	}

	return j;	
}


void check_keyword()
{
	/*use hardcoded array*/	
	char tok[MAX_ARGC + 1][MAX_CMDHELP];
	char *argv[MAX_ARGC + 1];
	int k = 0;
	
	for (k;k<MAX_ARGC + 1;k++){
	argv[k] = &tok[k][0];
	}
	int i;
	int argc;
	
	char cmdstr[CMDBUF_SIZE];
	strcpy(cmdstr, &cmd[cur_his][0]);
	
	argc = cmdtok(argv, cmdstr);

	for (i = 0; i < CMD_COUNT; i++) {
		if (!strcmp(cmd_data[i].cmd, argv[0])) {
			cmd_data[i].func(argc, argv);
			break;
		}
	}

	if (i == CMD_COUNT) {
		printf("%s",argv[0]);
		printf(": command not found");
		printf("%s",next_line);
	}
}

void shell()
{
	char put_ch[2]={'0','\0'};
	char *p = NULL;
	char *str ="\rShell:~$";	

	for (;; cur_his = (cur_his + 1) % HISTORY_COUNT) {
		/* need use & that p can work correct, idk why p = cmd[cur_his] can't work */
		p = &cmd[cur_his][0];

		printf("%s",str);
		
		while (1) {
			//put_ch = receive_byte();			
			read(open("/dev/tty0/in", 0), put_ch, 1);

			if (put_ch[0] == '\r' || put_ch[0] == '\n') {
				*p = '\0';
				printf("%s",next_line);
				break;
			}
			else if (put_ch[0]== 127 || put_ch[0] == '\b') {
				if (p > &cmd[cur_his][0]) {
					p--;
					printf("\b \b");
				}
			}
			else if (p - &cmd[cur_his][0] < CMDBUF_SIZE - 1) {
				*(p++) = put_ch[0];
				printf("%c",put_ch[0]);
			}	
		}
		check_keyword();		
	}
}





