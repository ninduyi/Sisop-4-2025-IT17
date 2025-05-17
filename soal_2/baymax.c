#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

#define RELICS_DIR "/home/baallqiisss/sisop_4/relics"
#define LOG_FILE "/home/baallqiisss/sisop_4/activity.log"
#define MAX_FRAGMENT_SIZE 1024

void write_log(const char *format, ...) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, format);
    vfprintf(log, format, args);
    va_end(args);

    fprintf(log, "\\n");
    fclose(log);
}

static int baymax_getattr(const char *path, struct stat *stbuf,
                          struct fuse_file_info *fi) {
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(path, "/Baymax.jpeg") == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 14 * MAX_FRAGMENT_SIZE;
    } else {
        return -ENOENT;
    }
    return 0;
}

static int baymax_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, "Baymax.jpeg", NULL, 0, 0);
    return 0;
}

static int baymax_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, "/Baymax.jpeg") != 0)
        return -ENOENT;
    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;
    write_log("READ: Baymax.jpeg");
    return 0;
}

static int baymax_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    if (strcmp(path, "/Baymax.jpeg") != 0)
        return -ENOENT;

    size_t total_read = 0;
    for (int i = 0; i < 14; i++) {
        char frag_path[512];
        snprintf(frag_path, sizeof(frag_path), "%s/Baymax.jpeg.%03d", RELICS_DIR, i);
        FILE *frag = fopen(frag_path, "rb");
        if (!frag) continue;

        fseek(frag, 0, SEEK_END);
        size_t fsize = ftell(frag);
        fseek(frag, 0, SEEK_SET);

        if (offset < total_read + fsize) {
            size_t frag_offset = offset > total_read ? offset - total_read : 0;
            fseek(frag, frag_offset, SEEK_SET);
            size_t to_read = fsize - frag_offset;
            if (to_read > size) to_read = size;
            fread(buf, 1, to_read, frag);
            fclose(frag);
            return to_read;
        }

        total_read += fsize;
        fclose(frag);
    }

    return 0;
}

static int baymax_create(const char *path, mode_t mode,
                         struct fuse_file_info *fi) {
    char fullpath[512];
    snprintf(fullpath, sizeof(fullpath), "%s%s", RELICS_DIR, path);
    FILE *fp = fopen(fullpath, "wb");
    if (!fp) return -EACCES;
    fclose(fp);
    return 0;
}

static int baymax_write(const char *path, const char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi) {
    char filename[256];
    sscanf(path, "/%[^/\\n]", filename);

    int count = 0;
    for (size_t i = 0; i < size; i += MAX_FRAGMENT_SIZE) {
        char frag_path[512];
        snprintf(frag_path, sizeof(frag_path), "%s/%s.%03d", RELICS_DIR, filename, count);
        FILE *frag = fopen(frag_path, "wb");
        if (!frag) return -EIO;
        size_t to_write = size - i > MAX_FRAGMENT_SIZE ? MAX_FRAGMENT_SIZE : size - i;
        fwrite(buf + i, 1, to_write, frag);
        fclose(frag);
        count++;
    }

    write_log("WRITE: %s ->", filename);
    for (int i = 0; i < count; i++)
        write_log("%s.%03d", filename, i);
    return size;
}

static int baymax_unlink(const char *path) {
    char filename[256];
    sscanf(path, "/%[^/\\n]", filename);

    char deleted[1024] = "";
    for (int i = 0;; i++) {
        char frag_path[512];
        snprintf(frag_path, sizeof(frag_path), "%s/%s.%03d", RELICS_DIR, filename, i);
        if (access(frag_path, F_OK) == -1) break;
        remove(frag_path);
        strcat(deleted, frag_path);
        strcat(deleted, " ");
    }
    write_log("DELETE: %s", deleted);
    return 0;
}

static struct fuse_operations baymax_oper = {
    .getattr = baymax_getattr,
    .readdir = baymax_readdir,
    .open = baymax_open,
    .read = baymax_read,
    .create = baymax_create,
    .write = baymax_write,
    .unlink = baymax_unlink,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &baymax_oper, NULL);
}
k
