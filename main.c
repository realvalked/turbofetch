#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// colors
#define COL_CYAN    "\x1b[36m"
#define COL_MAGENTA "\x1b[35m"
#define COL_BLUE    "\x1b[94m"
#define COL_RED     "\x1b[31m"
#define COL_GREEN   "\x1b[32m"
#define COL_YELLOW  "\x1b[33m"
#define COL_RESET   "\x1b[0m"
#define COL_TEXT    COL_BLUE // used for text
#define COL_LOGO    COL_RED // used for logo
#define COL_BORDER  COL_RED // used for border

// functions to pipe command output into string so we can print it out
void parse(char *dest, char *cmd);
void trim_left(char *dest, int amt);
void trim_right(char *dest, int amt);
void get_os(char *os_release);
void get_packages(char *packages);

int main(void) {
    // declared strings here for us to copy into
    static char hostname[100];
    static char os_release[100];
    static char kernel[100];
    static char packages[100];
    static char memory_used[100], memory_total[100];
    static char processor[100];
    static char gpu[250]; // gpu name can be quite long, so i declared this one with a larger size
    static char uptime[100];

    // get hostname, os, kernel, etc.
    parse(hostname,"echo $(whoami)@$(cat /etc/hostname)");
    get_os(os_release);
    parse(kernel,"uname -r");
    get_packages(packages);
    parse(memory_used,"free -mh --si | awk  {'print $3'} | head -n 2 | tail -1");
    parse(memory_total,"free -mh --si | awk  {'print $2'} | head -n 2 | tail -1");
    parse(processor,"cat /proc/cpuinfo | grep \"model name\" | tail -1"); trim_left(processor,13);
    parse(gpu,"lspci | grep VGA"); trim_left(gpu,35);
    parse(uptime,"uptime -p"); trim_left(uptime,3);
    
    // print it all out
    printf(COL_LOGO"                         "COL_BORDER"                                "COL_RESET"\n");
    printf(COL_LOGO"     #####     #####     "COL_BORDER"│"COL_TEXT" Hostname: "COL_RESET"%s\n",hostname);
    printf(COL_LOGO"      #####   #####      "COL_BORDER"│"COL_TEXT" OS: "COL_RESET"%s\n",os_release);
    printf(COL_LOGO"       ##### #####       "COL_BORDER"│"COL_TEXT" Kernel: "COL_RESET"%s\n",kernel); 
    printf(COL_LOGO"        #########        "COL_BORDER"│"COL_TEXT" Packages: "COL_RESET"%s\n",packages); //FIXME not everyone uses arch
    printf(COL_LOGO"        #########        "COL_BORDER"│"COL_TEXT" Memory: "COL_RESET"%s/%s\n",memory_used,memory_total);
    printf(COL_LOGO"       ##### #####       "COL_BORDER"│"COL_TEXT" CPU: "COL_RESET"%s\n",processor);
    printf(COL_LOGO"      #####   #####      "COL_BORDER"│"COL_TEXT" GPU: "COL_RESET"%s\n",gpu);
    printf(COL_LOGO"     #####     #####     "COL_BORDER"│"COL_TEXT" Uptime: "COL_RESET"%s\n",uptime);
    printf(COL_LOGO"                         "COL_BORDER"                                "COL_RESET"\n");
}

// function definitions
void parse(char *dest, char *cmd) {
    char c = 0;
    FILE* tmp;

    // popen exception
    if (0 == (tmp = (FILE*)popen(cmd,"r"))) {
        fprintf(stderr,"ERROR: popen(\"%s\",\"r\") failed.\n",cmd);
        return;
    }

    // pipe everything into the string
    int i;
    for (i = 0; fread(&dest[i], sizeof(char), 1, tmp); i++) {
        if (dest[i]=='\n') {
            dest[i] = '\0';
            return;
        }
    }
    dest[i] = '\0';
}

void trim_left(char *dest, int amt) {
    char *ptr = dest;
    ptr += amt;
    strcpy(dest,ptr);
}

void trim_right(char *dest, int amt) {
    char *ptr = strchr(dest,0);
    ptr -= amt;
    *ptr = 0;
}

void get_os(char *os_release) {
    parse(os_release,"cat /etc/os-release | grep NAME | head -1");
    trim_left(os_release,6);
    trim_right(os_release,1);
}

void get_packages(char *packages) {
    if (access("/bin/pacman",R_OK)==0) {
        parse(packages,"pacman -Q | wc -l");
        return;
    }
    if (access("/bin/dpkg",R_OK)==0) {
        parse(packages,"dpkg-query -f '${binary:Package}\\n' -W | wc -l");
        return;
    }
    if (access("/bin/rpm",R_OK)==0) {
        parse(packages,"rpm -qa | wc -l");
        return;
    }
    if (access("/bin/yum",R_OK)==0) {
        parse(packages,"yum list installed | wc -l");
        return;
    }
    if (access("/bin/dnf",R_OK)==0) {
        parse(packages,"dnf list installed | wc -l");
        return;
    }
    if (access("/bin/eopkg",R_OK)==0) {
        parse(packages,"eopkg li -i | wc -l");
        return;
    }
}
