#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

static const char *source_dir = "/it24_host";
static const char *log_file = "/var/log/it24.log";

// logging
void log_event(const char *event, const char *details) {
    int fd = open(log_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) return;

    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // remove newline

    char log_entry[512];
    snprintf(log_entry, sizeof(log_entry), "[%s] %s: %s\n", time_str, event, details);
    write(fd, log_entry, strlen(log_entry));
    close(fd);
}

// detect dangerous files
int is_dangerous(const char *name) {
    return (strstr(name, "nafis") != NULL || strstr(name, "kimcun") != NULL);
}

void reverse_name(const char *name, char *reversed) {
    int len = strlen(name);
    for (int i = 0; i < len; i++) {
        reversed[i] = name[len - 1 - i];
    }
    reversed[len] = '\0';
}

void rot13(char *text, int len) {
    for (int i = 0; i < len; i++) {
        if ((text[i] >= 'a' && text[i] <= 'z')) {
            text[i] = 'a' + (text[i] - 'a' + 13) % 26;
        } else if ((text[i] >= 'A' && text[i] <= 'Z')) {
            text[i] = 'A' + (text[i] - 'A' + 13) % 26;
        }
    }
}

// getattr (implementasi fuse)
static int antink_getattr(const char *path, struct stat *stbuf) {
    char real_path[256];
    snprintf(real_path, sizeof(real_path), "%s%s", source_dir, path);

    int res = lstat(real_path, stbuf);
    if (res == -1)  return -errno;
    return 0;
}

// readdir (implementasi fuse)
static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char real_path[256];
    snprintf(real_path, sizeof(real_path), "%s%s", source_dir, path);

    DIR *dp = opendir(real_path);
    if (dp == NULL) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char display_name[256];
        if (is_dangerous(de->d_name)) {
            reverse_name(de->d_name, display_name);
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Anomaly detected in file: /%s", de->d_name);
            log_event("ALERT", log_msg);
        } else {
            strcpy(display_name, de->d_name);
        }
        if (filler(buf, display_name, &st, 0)) break;
    }
    closedir(dp);
    return 0;
}

// open (implementasi fuse)
static int antink_open(const char *path, struct fuse_file_info *fi) {
   char real_path[256];
   snprintf(real_path, sizeof(real_path), "%s%s", source_dir, path);

   int fd = open(real_path, fi->flags);
   if (fd == -1) return -errno;

   fi->fh = fd;
   return 0;
}

// read (implementasi fuse)
static int antink_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char real_path[256];
    snprintf(real_path, sizeof(real_path), "%s%s", source_dir, path);

    int fd = fi->fh;
    if (fd == -1) return -errno;

    lseek(fd, offset, SEEK_SET);
    int res = read(fd, buf, size);
    if (res == -1) return -errno;

    int is_text = (strstr(path, ".txt") != NULL);
    int is_danger = is_dangerous(path);

    if (is_text && !is_danger) {
        rot13(buf, res);
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "File %s has been encrypted", path);
        log_event("ENCRYPT", log_msg);
    } else if (is_danger) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "File %s has been reversed", path);
        log_event("REVERSE", log_msg);
    }
    return res;
}

// operasi FUSE
static struct fuse_operations antink_oper = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .open = antink_open,
    .read = antink_read,
};

int main(int argc, char *argv[]) {
    umask(0);
    printf("Starting FUSE filesystem at %s...\n", argv[1]);
    int res = fuse_main(argc, argv, &antink_oper, NULL);
    printf("fuse_main returned: %d\n", res);
    if (res == 0) {
        printf("FUSE filesystem mounted successfully\n");
    } else {
        printf("FUSE filesystem failed to mount\n");
    }
    return res;
}