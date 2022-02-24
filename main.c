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
void get_memory(void);
void get_processor(void);
void get_gpu(void);
void get_uptime(void);
void get_info(int varg);

int main(void) {
    // number of threads
    pthread_t thr[8];

    // get hostname, os, kernel, etc. simultaneously via multithreading
    for (int i = 0; i < 8; i++) {
        pthread_create(&thr[i],NULL,get_info,i);
    }

    // wait only for the most time-consuming threads, this way we can finish running the script as quickly as possible
    for (int i = 0; i < 8; i++) {
        pthread_join(thr[i],NULL);
    }

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

    fclose(tmp);
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
    FILE* f_os_release = fopen("/etc/os-release", "r");
    char str[100], *ptr = str;

    while (fgets(str,100,f_os_release)!=NULL) {
        if (strstr(ptr,"NAME=\"")!=NULL) {
            ptr += 6;
            char *end = strchr(ptr,0);
            end-=2;
            *end = 0;
            strcpy(os_release,ptr);
            fclose(f_os_release);
            return;
        }
    }

    fclose(f_os_release);
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

void get_memory(void) {
    FILE* meminfo = fopen("/proc/meminfo","r");

    int mem_used_mb, mem_total_mb;
    char str[100];

    while (fgets(str,99,meminfo)!=NULL) {
        char *ptr = str;
        if (strstr(str,"MemTotal")!=NULL) {
            while (!(*ptr>='0'&&*ptr<='9')) ++ptr;
            trim_right(ptr,4);
            mem_total_mb = atoi(ptr)/1024;
            snprintf(memory_total,100,"%dMB\0",mem_total_mb);
        }
        if (strstr(str,"MemAvailable")!=NULL) {
            while (!(*ptr>='0'&&*ptr<='9')) ++ptr;
            trim_right(ptr,4);
            mem_used_mb = mem_total_mb-atoi(ptr)/1024;
            snprintf(memory_used,100,"%dMB\0",mem_used_mb);
        }
    }

    fclose(meminfo);
}

void get_processor(void) {
    FILE* cpuinfo = fopen("/proc/cpuinfo","r");

    char str[100], *ptr = str;
    while (fgets(str,100,cpuinfo)!=NULL) {
        if (strstr(str,"model name")!=NULL) {
            ptr += 10;
            while (*ptr==' '||*ptr==':'||*ptr=='\t') ++ptr;
            char* end = strchr(ptr,0);
            --end;
            *end = 0;
            strncpy(processor,ptr,100);
            fclose(cpuinfo);
            return;
        }
    }

    fclose(cpuinfo);
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
    FILE* f_uptime = fopen("/proc/uptime","r");
    double total_seconds_up;

    fscanf(f_uptime,"%lf",&total_seconds_up);

    if (total_seconds_up>=86400) {
        snprintf(uptime,100,"%d days, %d hours, %d minutes",
                             (int)total_seconds_up/60/60/24,(int)total_seconds_up/60/60,(int)total_seconds_up/60%60);
    }

    else {
        snprintf(uptime,100,"%d hours, %d minutes",
                             (int)total_seconds_up/60/60,(int)total_seconds_up/60%60);
    }

    fclose(f_uptime);
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
            get_memory();
            break;
        case 5:
            get_processor();
            break;
        case 6:
            get_gpu();
            break;
        case 7:
            get_uptime();
            break;
    }
}
