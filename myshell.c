#include "stm32f10x.h"
#include "RTOSConfig.h"

#include "syscall.h"

#include <stddef.h>
#include "systemdef.h"

extern struct task_info task_info_t;

struct command_history {
   char command_history1[100];
   char command_history2[100];
};

void print(char *str){
	write(mq_open("/tmp/mqueue/out", 0), str,strlen(str)+1);
}

void int2char(int x, char *str){
	if(x<10){
	str[0] = 48+x;
	}else{
	str[1] =  48+x%10;
	str[0] =  48+x/10;
	}
}

char *statetran(int i){
	switch(i){
	case 0x0:return	"	TASK_READY";
	case 0x1:return	"	TASK_WAIT_READ";
	case 0x2:return	"	TASK_WAIT_WRITE";
	case 0x3:return	"	TASK_WAIT_INTR";	
	case 0x4:return	"	TASK_WAIT_TIME";
	default:return "	Unknow";
	}
}
void scan(char *x)
{
	read(open("/dev/tty0/in", 0), x, 1);
}

void ps_task_info()
{
	int i = 0;
	char str[2] = {0,0,0};
	char str1[2] = {0,0,0};

	for(i;i<=*(task_info_t.task_count);i++){
		print("Task no =	");
		int2char(i,str);
		print(str);
		print("	;state = ");
		print(statetran(task_info_t.tasks[i].status));
		print("	;priority = ");	
		int2char((task_info_t.tasks[i].priority),str1);
		print(str1);
		print("\n\r\0");
	}
}

void shell()
{
	char str[100];
	char ch;
	char ins[100];
	int curr_char=0;
	int curr_ins=0;
	int init = 0;
	int history_count =1;
	struct command_history history;
	int a=0;
	int b=0;
	int a1=0;
	int b1=0;	

	while(1){	
		switch (init){
		case 0x0:
			print("shell>>");
			init = 1;
		break;
		case 0x1:
			curr_char=0;
			scan(&ch);
		if((ch==32)&&(curr_ins <= 0)){		//fix <space><command>
			str[curr_char++] = ch;
			ins[curr_ins++] = ch;
			curr_ins--;
		}	
		else{	
			//fix <backspace> but <up> <down> still fault
			if((ch==127)&&(curr_ins <= 0)){  
					str[curr_char++] = '\0';
			}
			else {
				if((ch==127)&&(curr_ins > 0)){		//fix <space> ...  <backspace> can work success
					str[curr_char++] = '\b';
					str[curr_char++] = ' ';
					str[curr_char++] = '\b';
					curr_ins--;
				}
				else{
					str[curr_char++] = ch;
					ins[curr_ins++] = ch;
				}
			}
		}
			print(str);

			if(ch=='\r'){
				history_count++;
				if(history_count%2 == 0){
					for(a1;a1<100;a1++)
					history.command_history1 [a1] = 0;
					for(a;a<curr_ins;a++)
					history.command_history1[a] = ins[a];
				}
				else{
					for(b1;b1<100;b1++)
					history.command_history2[b1] = 0;
					for(b;b<curr_ins;b++)
					history.command_history2[b] = ins[b];
				}
				
				
				//history
				if(((ins[0]=='h') || (ins[0]=='H')) &&
					((ins[1]=='i') || (ins[1]=='I')) &&
					((ins[2]=='s') || (ins[2]=='S')) &&
					((ins[3]=='t') || (ins[3]=='T')) &&
					((ins[4]=='o') || (ins[4]=='O')) &&
					((ins[5]=='r') || (ins[5]=='R')) &&
					((ins[6]=='y') || (ins[6]=='Y')) &&
					(ins[7]=='\r'))
					{
					print("\n\0");
					print(&history.command_history2);
					print("\n");
					print(&history.command_history1);
					print("\n\r\0");
					}
				//help HELP
				else if(((ins[0]=='h') || (ins[0]=='H')) &&
					((ins[1]=='e') || (ins[1]=='E')) &&
					((ins[2]=='l') || (ins[2]=='L')) &&
					((ins[3]=='p') || (ins[3]=='P')) &&
					(ins[4]=='\r'))
					{
						print("\n hello          -- show welcome.\n\r");
						print("\n echo <message> -- show the message you tape.\n\r");
						print("\n ps             -- show system threads info. \n\r");
						print("\n history        -- show tape before. \n\r");
						print("\n ^_____^\n\r");
						print("\0");
					}
					//hello	HELLO
				else if(((ins[0]=='h') || (ins[0]=='H')) &&
					((ins[1]=='e') || (ins[1]=='E')) &&
					((ins[2]=='l') || (ins[2]=='L')) &&
					((ins[3]=='l') || (ins[3]=='L')) &&
					((ins[4]=='o') || (ins[4]=='O')) &&
					(ins[5]=='\r'))
					{
						print("\n welcome !\n\r\0");
					}
					//echo	ECHO
				else if(((ins[0]=='e') || (ins[0]=='E')) &&
					((ins[1]=='c') || (ins[1]=='C')) &&
					((ins[2]=='h') || (ins[2]=='H')) &&
					((ins[3]=='o') || (ins[3]=='O')) &&
					(ins[4]==32))
					{
							int i = 5;
							int echocunt = 0;
							char echo[100];
							print("\n\0");
							for (i;i<curr_ins;i++){
								echo[echocunt++]=ins[i];
							}
							print(echo);
							print("\n\0");
					}
					//ps	PS
				else if(((ins[0]=='p') || (ins[0]=='P')) &&
					((ins[1]=='s') || (ins[1]=='S')) &&
					(ins[2]=='\r'))
					{
							print("\n");
							ps_task_info();	
					}	
				else {
					//when echo not echo<space>
					if(((ins[0]=='e') || (ins[0]=='E')) &&
					((ins[1]=='c') || (ins[1]=='C')) &&
					((ins[2]=='h') || (ins[2]=='H')) &&
					((ins[3]=='o') || (ins[3]=='O')))
					{
					print("\n you should tape echo <message> \n\r");
					}
					else	//no command
					{
					print("\n");
					ins[curr_ins-1] = '\0';
					print(ins);
					print(": command not found\n\r");
					}	
					}
				curr_ins=0;
				init = 0;
				a = 0;
				b = 0;
				a1=0;
				b1=0;	
			}
			
		break;	
		default:
			curr_ins=0;
			init = 0;
			a = 0;
			b = 0;
			a1=0;
			b1=0;
		
		}
	}
}	
