#include <stdarg.h>

#include "stm32f10x.h"
#include "RTOSConfig.h"

void xtoa(int n, char *buffer)
{
	char print_buf[9];
	char *s;
	int t;
	int i = 0;
	int j = 0;

	s = print_buf + 8;
	*s = '\0';	
	
	while (n) {
		t = n % 16;
		if( t >= 10 )
			t += 'A' - '0' - 10;
		*--s = t + '0';
		n /= 16;
		i++;
	}

	while(*s!='\0'){
	*buffer = *s;
	buffer++;
	s++;		
	}
	*buffer = '\0';
}

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


/* ref PJayChen */
void printf(const char *format, ...)
{
        va_list ap;
        va_start(ap, format);
        int curr_ch = 0;
        char out_ch[2] = {'\0', '\0'};
        char percentage[] = "%";
        char *str;
        char str_num[12];
        int out_int;

        while( format[curr_ch] != '\0' ){
            if(format[curr_ch] == '%'){
                if(format[curr_ch + 1] == 's'){
                    str = va_arg(ap, char *);
					puts(str);
				}else if(format[curr_ch + 1] == 'd'){
		            itoa(va_arg(ap, int), str_num);
					puts(str_num);
				}else if(format[curr_ch + 1] == 'x'){
					xtoa(va_arg(ap, int), str_num);
					puts(str_num);
				}else if(format[curr_ch + 1] == 'c'){
					/* char are converted to int then pushed on the stack */
					out_ch[0] =(char) va_arg(ap, int);
					out_ch[1] = '\0';					
					puts(out_ch); 
				}else if(format[curr_ch + 1] == '%'){
					puts(percentage); 
				} 				
				curr_ch++;
            }
			else{
                out_ch[0] = format[curr_ch];
				puts(&out_ch);
            }
            curr_ch++;
        }

		va_end(ap);
}





