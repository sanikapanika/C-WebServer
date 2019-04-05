#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 8096
#define ERROR 42
#define SORRY 43
#define LOG   44

struct {
    char *ext;
    char *filetype;
} extensions[] = {
        {const_cast<char *>("gif"),  const_cast<char *>("image/gif")},
        {const_cast<char *>("jpg"),  const_cast<char *>("image/jpeg")},
        {const_cast<char *>("jpeg"), const_cast<char *>("image/jpeg")},
        {const_cast<char *>("png"),  const_cast<char *>("image/png")},
        {const_cast<char *>("zip"),  const_cast<char *>("image/zip")},
        {const_cast<char *>("gz"),   const_cast<char *>("image/gz")},
        {const_cast<char *>("tar"),  const_cast<char *>("image/tar")},
        {const_cast<char *>("htm"),  const_cast<char *>("text/html")},
        {const_cast<char *>("html"), const_cast<char *>("text/html")},
        {nullptr,                    nullptr}};

void log(int type, char *s1, char *s2, int num) {
    int fd;
    char logbuffer[BUFSIZE * 2];

    switch (type) {
        case ERROR:
            (void) sprintf(logbuffer, "ERROR: %s:%s Errno=%d exiting pid=%d", s1, s2, errno, getpid());
            break;
        case SORRY:
            (void) sprintf(logbuffer, "<HTML><BODY><H1>nweb Web Server Sorry: %s %s</H1></BODY></HTML>\r\n", s1, s2);
            (void) write(num, logbuffer, strlen(logbuffer));
            (void) sprintf(logbuffer, "SORRY: %s:%s", s1, s2);
            break;
        case LOG:
            (void) sprintf(logbuffer, " INFO: %s:%s:%d", s1, s2, num);
            break;
    }
    /* no checks here, nothing can be done a failure anyway */
    if ((fd = open("nweb.log", O_CREAT | O_WRONLY | O_APPEND, 0644)) >= 0) {
        (void) write(fd, logbuffer, strlen(logbuffer));
        (void) write(fd, "\n", 1);
        (void) close(fd);
    }
    if (type == ERROR || type == SORRY) exit(3);
}

/* this is a child web server process, so we can exit on errors */
void web(int fd, int hit) {
    int j, file_fd, buflen, len;
    long i, ret;
    char *fstr;
    static char buffer[BUFSIZE + 1]; /* static so zero filled */

    ret = read(fd, buffer, BUFSIZE);    /* read Web request in one go */

    if (ret == 0 || ret == -1) {    /* read failure stop now */
        log(SORRY, const_cast<char *>("failed to read browser request"), const_cast<char *>(""), fd);
    }

    if (ret > 0 && ret < BUFSIZE)    /* return code is valid chars */
        buffer[ret] = 0;        /* terminate the buffer */
    else
        buffer[0] = 0;

    for (i = 0; i < ret; i++)    /* remove CF and LF characters */
        if (buffer[i] == '\r' || buffer[i] == '\n')
            buffer[i] = '*';

    log(LOG, const_cast<char *>("request"), buffer, hit);

    if (strncmp(buffer, "GET ", 4) != 0 && strncmp(buffer, "get ", 4) != 0)
        log(SORRY, const_cast<char *>("Only simple GET operation supported"), buffer, fd);

    for (i = 4; i < BUFSIZE; i++) { /* null terminate after the second space to ignore extra stuff */
        if (buffer[i] == ' ') { /* string is "GET URL " +lots of other stuff */
            buffer[i] = 0;
            break;
        }
    }

    for (j = 0; j < i - 1; j++)    /* check for illegal parent directory use .. */
        if (buffer[j] == '.' && buffer[j + 1] == '.')
            log(SORRY, const_cast<char *>("Parent directory (..) path names not supported"), buffer, fd);

    if (!strncmp(&buffer[0], "GET /\0", 6) ||
        !strncmp(&buffer[0], "get /\0", 6)) /* convert no filename to index file */
        (void) strcpy(buffer, "GET /index.html");

    /* work out the file type and check we support it */
    buflen = static_cast<int>(strlen(buffer));
    fstr = (char *) nullptr;
    for (i = 0; extensions[i].ext != nullptr; i++) {
        len = static_cast<int>(strlen(extensions[i].ext));
        if (!strncmp(&buffer[buflen - len], extensions[i].ext, static_cast<size_t>(len))) {
            fstr = extensions[i].filetype;
            break;
        }
    }
    if (fstr == nullptr) log(SORRY, const_cast<char *>("file extension type not supported"), buffer, fd);

    if ((file_fd = open(&buffer[5], O_RDONLY)) == -1) /* open the file for reading */
        log(SORRY, const_cast<char *>("failed to open file"), &buffer[5], fd);

    log(LOG, const_cast<char *>("SEND"), &buffer[5], hit);

    (void) sprintf(buffer, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
    (void) write(fd, buffer, strlen(buffer));

    /* send file in 8KB block - last block may be smaller */
    while ((ret = read(file_fd, buffer, BUFSIZE)) > 0) {
        (void) write(fd, buffer, static_cast<size_t>(ret));
    }
#ifdef LINUX
    sleep(1);	/* to allow socket to drain */
#endif
    exit(1);
}


int main(int argc, char **argv) {
    int i, port, pid, listenfd, socketfd, hit;
    /*	size_t length; */
    unsigned int length;
    static struct sockaddr_in cli_addr; /* static = initialised to zeros */
    static struct sockaddr_in serv_addr; /* static = initialised to zeros */

    if (argc < 3 || argc > 3 || !strcmp(argv[1], "-?")) {
        (void) printf("hint: nweb Port-Number Top-Directory\n\n"
                      "\tnweb is a small and very safe mini web server\n"
                      "\tnweb only servers out file/web pages with extensions named below\n"
                      "\t and only from the named directory or its sub-directories.\n"
                      "\tThere is no fancy features = safe and secure.\n\n"
                      "\tExample: nweb 8181 /home/nwebdir &\n\n"
                      "\tOnly Supports:");
        for (i = 0; extensions[i].ext != nullptr; i++)
            (void) printf(" %s", extensions[i].ext);

        (void) printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
                      "\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
                      "\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n"
        );
        exit(0);
    }
    if (!strncmp(argv[2], "/", 2) || !strncmp(argv[2], "/etc", 5) ||
        !strncmp(argv[2], "/bin", 5) || !strncmp(argv[2], "/lib", 5) ||
        !strncmp(argv[2], "/tmp", 5) || !strncmp(argv[2], "/usr", 5) ||
        !strncmp(argv[2], "/dev", 5) || !strncmp(argv[2], "/sbin", 6)) {
        (void) printf("ERROR: Bad top directory %s, see nweb -?\n", argv[2]);
        exit(3);
    }
    if (chdir(argv[2]) == -1) {
        (void) printf("ERROR: Can't Change to directory %s\n", argv[2]);
        exit(4);
    }

    /* Become deamon + unstopable and no zombies children (= no wait()) */
    if (fork() != 0)
        return 0; /* parent returns OK to shell */
    (void) signal(SIGCLD, SIG_IGN); /* ignore child death */
    (void) signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */
    for (i = 0; i < 32; i++)
        (void) close(i);        /* close open files */
    (void) setpgrp();        /* break away from process group */

    log(LOG, const_cast<char *>("nweb starting"), argv[1], getpid());

    /* setup the network socket */

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        log(ERROR, const_cast<char *>("system call"), const_cast<char *>("socket"), 0);

    log(LOG, const_cast<char *>("listenfd"), nullptr, listenfd);

    port = static_cast<int>(strtol(argv[1], nullptr, 10));
    if (port < 0 || port > 60000)
        log(ERROR, const_cast<char *>("Invalid port number (try 1->60000)"), argv[1], 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        log(ERROR, const_cast<char *>("system call"), const_cast<char *>("bind"), 0);

    if (listen(listenfd, 64) < 0)
        log(ERROR, const_cast<char *>("system call"), const_cast<char *>("listen"), 0);

    for (hit = 1;; hit++) {
        length = sizeof(cli_addr);
        if ((socketfd = accept(listenfd, (struct sockaddr *) &cli_addr, &length)) < 0)
            log(ERROR, const_cast<char *>("system call"), const_cast<char *>("accept"), 0);

        log(LOG, const_cast<char *>("socketfd"), nullptr, socketfd);

        if ((pid = fork()) < 0) {
            log(ERROR, const_cast<char *>("system call"), const_cast<char *>("fork"), 0);
        } else {
            if (pid == 0) {    /* child */
                (void) close(listenfd);
                web(socketfd, hit); /* never returns */
            } else {    /* parent */
                (void) close(socketfd);
            }
        }
    }
}