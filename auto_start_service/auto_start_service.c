#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <sys/wait.h>
#include <dirent.h>

int service_quit;
static void sig_handler_func(int sig)
{
    printf("sig_handler_func : %d\n", sig);
    service_quit = 1;
}

static void sig_handler_ignore(int sig)
{
    printf("sig_handler_ignore : %d\n", sig);
}

/* copy from shairport */
void signal_setup(void) {
    // mask off all signals before creating threads.
    // this way we control which thread gets which signals.
    // for now, we don't care which thread gets the following.
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGINT);
    sigdelset(&set, SIGTERM);
    sigdelset(&set, SIGHUP);
    sigdelset(&set, SIGSTOP);
    sigdelset(&set, SIGPIPE);
    //sigdelset(&set, SIGCHLD);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    //setting this to SIG_IGN would prevent signalling any threads.
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = (void *)sig_handler_ignore;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sa.sa_sigaction = (void *)sig_handler_func;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_sigaction = (void *)sig_handler_ignore;
    sigaction(SIGHUP, &sa, NULL);

    signal(SIGPIPE,SIG_IGN);
}


int main(int argc,char *argv[])
{
	int ret;
	DIR *dir;
	struct dirent *ptr;
	int file_num = 0;
	char *dir_path;
	char cmd_buf[256];

	signal_setup();

	if (argc != 2) {
		printf("wrong argv\n");
		return -1;
	}

	dir_path = argv[1];

	while (1) {
		if ((dir = opendir(dir_path)) == NULL) {
			printf("open dir(%s) error...\n", dir_path);
			return -1;
		}

		while ((ptr = readdir(dir)) != NULL) {
			if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
				continue;
			} else if (ptr->d_type == 8) {
				sprintf(cmd_buf, "ps -e | grep %s | grep -v grep >/dev/null", ptr->d_name);
				ret = system(cmd_buf);
				if (ret != 0) {
					sprintf(cmd_buf, "cd %s && chmod 777 %s && ./%s &", dir_path, ptr->d_name, ptr->d_name);
					ret = system(cmd_buf);
					if (ret == 0) {
						printf("%s service start success \n", ptr->d_name);
					} else {
						printf("%s service start failed\n", ptr->d_name);
					}
				}
			}
		}

		closedir(dir);
		sleep(1);
	}

	return 0;
}
