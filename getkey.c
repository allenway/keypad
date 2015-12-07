#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/input.h>

int main (int argc,char *argv[])  
{  
    int keys_fd;  
    char ret[2];  
    struct input_event t;  
    if(argc<2)
    {
        printf("Please input dev path,such as:./getkey /dev/input/eventX\n");
        return -1;
    }
    keys_fd = open (argv[1], O_RDONLY);  
    if (keys_fd <= 0)  
    {  
        printf ("open %s device error!\n",argv[1]);  
        return -1;  
    }
    else
        printf("open %s device successful!\n",argv[1]);

    while (1)  
    {  
        if (read (keys_fd, &t, sizeof (t)) == sizeof (t))  
        {  
            if (t.type == EV_KEY)  
                if (t.value == 0 || t.value == 1)  
                {  
                    printf ("key %d %s\n", t.code,  
                            (t.value) ? "Pressed" : "Released");  
                    if(t.code==KEY_ESC)  
                        break;  
                }  
        }  
    }  
    close (keys_fd);  
    return 0;  
}  


