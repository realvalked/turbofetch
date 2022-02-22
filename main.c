#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <pci/pci.h>

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

// declared strings here for us to copy into
static char hostname[100];
static char os_release[100];
static char kernel[100];
static char packages[100];
static char memory_used[100], memory_total[100];
static char processor[100];
static char gpu[250]; // gpu name can be quite long, so i declared this one with a larger size
static char uptime[100];

// functions to pipe command output into string so we can print it out
void parse(char *dest, char *cmd);
void trim_left(char *dest, int amt);
void trim_right(char *dest, int amt);
void get_hostname(void);
void get_os(void);
void get_kernel(void);
void get_packages(void);
void get_packages_pacman(void);
void get_memory_used(void);
void get_memory_total(void);
void get_processor(void);
void get_gpu(void);
void get_uptime(void);
void get_info(int varg);

int main(void) {
    // number of threads
    pthread_t thr[10];

    // get hostname, os, kernel, etc. simultaneously via multithreading
    for (int i = 0; i < 9; i++) {
        pthread_create(&thr[i],NULL,get_info,i);
    }

    // wait only for the most time-consuming threads, this way we can finish running the script as quickly as possible
    for (int i = 0; i < 9; i++) {
        pthread_join(thr[i],NULL);
    }
    //pthread_join(thr[3],NULL);
    //pthread_join(thr[7],NULL);

    // print it all out
    printf("\n");
    printf(COL_LOGO "    #####   #####       " COL_BORDER "│" COL_TEXT " Hostname: "   COL_RESET    "%s\n",hostname);
    printf(COL_LOGO "     #####   #####      " COL_BORDER "│" COL_TEXT " OS: "         COL_RESET    "%s\n",os_release);
    printf(COL_LOGO "      #####   #####     " COL_BORDER "│" COL_TEXT " Kernel: "     COL_RESET    "%s\n",kernel); 
    printf(COL_LOGO "       #####   #####    " COL_BORDER "│" COL_TEXT " Packages: "   COL_RESET    "%s\n",packages);
    printf(COL_LOGO "       #####   #####    " COL_BORDER "│" COL_TEXT " Memory: "     COL_RESET "%s/%s\n",memory_used,memory_total);
    printf(COL_LOGO "      #####   #####     " COL_BORDER "│" COL_TEXT " CPU: "        COL_RESET    "%s\n",processor);
    printf(COL_LOGO "     #####   #####      " COL_BORDER "│" COL_TEXT " GPU: "        COL_RESET    "%s\n",gpu);
    printf(COL_LOGO "    #####   #####       " COL_BORDER "│" COL_TEXT " Uptime: "     COL_RESET    "%s\n",uptime);
    printf("\n");

    return 0;
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

void get_hostname(void) {
    parse(hostname,"echo $(whoami)@$(cat /etc/hostname)");
}

void get_os(void) {
    parse(os_release,"cat /etc/os-release | grep NAME | head -1");
    trim_left(os_release,6);
    trim_right(os_release,1);
}

void get_kernel(void) {
    parse(kernel,"uname -r");
}

void get_packages_pacman(void) {
    int c=0;
    struct dirent* dp;
    DIR* fd;

    if ((fd = opendir("/var/lib/pacman/local")) == NULL) {
        fprintf(stderr, "Error opening /var/lib/pacman/local\n");
        return;
    }

    while ((dp = readdir(fd)) != NULL) {
        if (!strcmp(dp->d_name,".") || !strcmp(dp->d_name, "..")) continue;
        c++;
    }

    closedir(fd);

    snprintf(packages,10,"%d",c);
}

void get_packages(void) {
    if (access("/bin/pacman",R_OK)==0) {
        get_packages_pacman();
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

void get_memory_used(void) {
    parse(memory_used,"free -mh --si | awk  {'print $3'} | head -n 2 | tail -1");
}

void get_memory_total(void) {
    parse(memory_total,"free -mh --si | awk  {'print $2'} | head -n 2 | tail -1");
}

void get_processor(void) {
    parse(processor,"cat /proc/cpuinfo | grep \"model name\" | tail -1"); trim_left(processor,13);
}

void get_gpu(void) {
    char buf[250], *device_class;

    struct pci_access *pacc;
    struct pci_dev *dev;
    
    pacc = pci_alloc();
    pci_init(pacc);
    pci_scan_bus(pacc);
    dev = pacc->devices;

    while (dev!=NULL) {
        pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_CLASS);
        device_class = pci_lookup_name(pacc, buf, sizeof(buf), PCI_LOOKUP_CLASS, dev->device_class);
        if (strcmp("VGA compatible controller", device_class)==0 || strcmp("3D controller", device_class)==0) {
            strncpy(gpu, pci_lookup_name(pacc, buf, sizeof(buf), PCI_LOOKUP_DEVICE | PCI_LOOKUP_VENDOR, dev->vendor_id, dev->device_id), 250);
            break;
        }
        dev = dev->next;
    }

    pci_cleanup(pacc);

    if (strstr(gpu,"Advanced Micro Devices, Inc. ")!=NULL) {
        trim_left(gpu,29);
    }
}

void get_uptime(void) {
    parse(uptime,"uptime -p"); trim_left(uptime,3);
}

void get_info(int varg) {
    switch(varg) {
        case 0:
            get_hostname();
            break;
        case 1:
            get_os();
            break;
        case 2:
            get_kernel();
            break;
        case 3:
            get_packages();
            break;
        case 4:
            get_memory_used();
            break;
        case 5:
            get_memory_total();
            break;
        case 6:
            get_processor();
            break;
        case 7:
            get_gpu();
            break;
        case 8:
            get_uptime();
            break;
    }
}
