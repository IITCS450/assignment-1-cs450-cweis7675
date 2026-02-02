#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

static void usage(const char *a) { 
    fprintf(stderr, "Usage: %s <pid>\n", a); 
    exit(1); 
}

static int isnum(const char* s) { 
    for (; *s; s++) if (!isdigit(*s)) return 0; 
    return 1; 
}

int main(int c, char** v) 
{
    if (c != 2 || !isnum(v[1])) usage(v[0]);

    char path[256];
    FILE *fp;

    // 1. Read /proc/[pid]/stat
    snprintf(path, sizeof(path), "/proc/%s/stat", v[1]);
    fp = fopen(path, "r");
    if (!fp) 
    {
        DIE("Could not open stat file (PID may not exist or permission denied)");
    }

    long ppid, utime, stime;
    char state;

    if (fscanf(fp, "%*d (%*[^)]) %c %ld", &state, &ppid) != 2) 
    {
        fclose(fp);
        DIE_MSG("Failed to parse stat: unexpected format");
    }

    // Skip fields
    for (int i = 0; i < 9; i++) 
    {
        if (fscanf(fp, "%*s") == EOF) break;
    }

    // Read Fields 14 and  15
    if (fscanf(fp, "%ld %ld", &utime, &stime) != 2) 
    {
        fclose(fp);
        DIE_MSG("Failed to parse CPU times from stat");
    }
    fclose(fp);

    // Calculate CPU time
    long total_ticks = utime + stime;
    double cpu_seconds = (double)total_ticks / sysconf(_SC_CLK_TCK);

    // 2. Read /proc/[pid]/status
    // Provided: VmRSS
    snprintf(path, sizeof(path), "/proc/%s/status", v[1]);
    fp = fopen(path, "r");
    long vmrss = 0;
    if (fp) 
    {
        char line[256];
        while (fgets(line, sizeof(line), fp)) 
        {
            if (strncmp(line, "VmRSS:", 6) == 0) 
            {
                sscanf(line + 6, "%ld", &vmrss);
                break;
            }
        }
        fclose(fp);
    }

    // 3. Read /proc/[pid]/cmdline
    // Provided: Original command line arguments
    snprintf(path, sizeof(path), "/proc/%s/cmdline", v[1]);
    fp = fopen(path, "r");
    char cmdline[1024] = {0};
    if (fp) 
    {
        size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, fp);
        fclose(fp);
        if (n > 0) 
        {
            for (size_t i = 0; i < n - 1; i++) {
                if (cmdline[i] == '\0') cmdline[i] = ' ';
            }
        } 
        else 
        {
            strncpy(cmdline, "N/A", sizeof(cmdline));
        }
    } 
    else 
    {
        strncpy(cmdline, "N/A", sizeof(cmdline));
    }

    // print the output
    printf("PID:%s\n", v[1]);
    printf("State: %c\n", state);
    printf("PPID:%ld\n", ppid);
    printf("Cmd:%s\n", cmdline);
    printf("CPU:%ld %.3f\n", total_ticks, cpu_seconds);
    printf("VmRSS:%ld\n", vmrss);

    return 0;
}