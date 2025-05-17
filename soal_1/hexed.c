#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>

#define MAX_FILE_SIZE 10000000  // 10MB
#define OUTPUT_FOLDER "anomali/image"
#define LOG_FILE "anomali/conversion.log"

char cwd[1024];  // Variabel untuk menyimpan direktori saat ini
static const char *dirpath;
static const char *image_dir = "/image";  // Folder untuk menyimpan gambar hasil konversi

// Deklarasi fungsi
void download_and_extract_files();
void ensure_output_folder_exists();
void get_current_time_str(char *date_str, char *time_str);
int hex_to_val(char c);
void convert_hex_to_image(const char *hex_string, const char *output_path);
void log_conversion(const char *file_name, const char *output_image_path);
void ensure_anomali_folder_exists();

// Fungsi untuk mendapatkan direktori kerja saat ini dan menambahkan '/anomali'
void set_dirpath() {
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        // Gabungkan dengan folder 'anomali'
        strcat(cwd, "/anomali");
        dirpath = cwd;  // Assign direktori yang sudah digabung ke dirpath
    } else {
        perror("getcwd() error");
        exit(1);  // Keluar jika gagal mendapatkan direktori saat ini
    }
}

// Fungsi untuk mendownload dan mengekstrak file ZIP
void download_and_extract_files() {
    // Unduh file zip dari Google Drive
    system("wget --no-check-certificate \"https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download\" -O anomali.zip");
    
    // Ekstrak file zip
    system("unzip anomali.zip");
    
    // Hapus file zip setelah ekstraksi selesai
    system("rm anomali.zip");
}

// Fungsi untuk memastikan folder output ada
void ensure_output_folder_exists() {
    struct stat st = {0};
    if (stat(OUTPUT_FOLDER, &st) == -1) {
        mkdir(OUTPUT_FOLDER, 0755);
    }
}

// Fungsi untuk memastikan folder anomali ada
void ensure_anomali_folder_exists() {
    struct stat st = {0};
    if (stat("anomali", &st) == -1) {
        mkdir("anomali", 0755);  // Membuat folder 'anomali' jika belum ada
    }

    if (stat(OUTPUT_FOLDER, &st) == -1) {
        mkdir(OUTPUT_FOLDER, 0755);  // Membuat folder 'image' jika belum ada
    }
}

// Fungsi untuk mendapatkan waktu saat ini dalam format YYYY-MM-DD_HH:MM:SS
void get_current_time_str(char *date_str, char *time_str) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(date_str, 11, "%Y-%m-%d", t);   // YYYY-MM-DD
    strftime(time_str, 9, "%H:%M:%S", t);    // HH:MM:SS
}

// Fungsi untuk mengonversi karakter hexadecimal ke nilai byte
int hex_to_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Fungsi untuk mengonversi string hexadecimal ke gambar PNG
void convert_hex_to_image(const char *hex_string, const char *output_path) {
    FILE *fp = fopen(output_path, "wb");
    if (fp == NULL) {
        perror("fopen() error");
        return;
    }

    // Mengonversi string hexadecimal ke byte dan menulisnya ke file
    size_t len = strlen(hex_string);
    for (size_t i = 0; i < len; i += 2) {
        char hex_byte[3] = {hex_string[i], hex_string[i + 1], '\0'};
        unsigned char byte = (unsigned char)strtol(hex_byte, NULL, 16);
        fwrite(&byte, 1, 1, fp);
    }

    fclose(fp);
}

// Fungsi untuk menulis log konversi ke dalam file
void log_conversion(const char *file_name, const char *output_image_path) {
    printf("Inside log_conversion function...\n");

    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Failed to open log file for writing");
        return;
    }

    char date_str[11], time_str[9];
    get_current_time_str(date_str, time_str);

    fprintf(log_file, "[%s][%s]: Successfully converted hexadecimal text %s to %s\n",
            date_str, time_str, file_name, output_image_path);

    // Debug: Check if closing the file was successful
    printf("Log written: %s -> %s\n", file_name, output_image_path);

    if (fclose(log_file) != 0) {
        perror("Error closing log file");
    }
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
    int res;
    char fpath[1000];

    sprintf(fpath, "%s%s", dirpath, path);

    res = lstat(fpath, stbuf);

    if (res == -1) return -errno;

    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    char fpath[1000];

    if (strcmp(path, "/") == 0) {
        path = dirpath;
        sprintf(fpath, "%s", path);
    } else {
        sprintf(fpath, "%s%s", dirpath, path);
    }

    int res = 0;

    DIR *dp;
    struct dirent *de;
    (void) offset;
    (void) fi;

    dp = opendir(fpath);

    if (dp == NULL) return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;

        memset(&st, 0, sizeof(st));

        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        res = (filler(buf, de->d_name, &st, 0));

        if (res != 0) break;
    }

    closedir(dp);

    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("Reading file: %s\n", path);

    char fpath[1000];
    if (strcmp(path, "/") == 0) {
        path = dirpath;
        sprintf(fpath, "%s", path);
    } else {
        sprintf(fpath, "%s%s", dirpath, path);
    }

    int res = 0;
    int fd = 0;

    (void) fi;

    fd = open(fpath, O_RDONLY);

    if (fd == -1) return -errno;

    res = pread(fd, buf, size, offset);

    if (res == -1) res = -errno;

    // Debug: Check if file is a .txt file and attempt conversion
    if (strstr(path, ".txt") != NULL) {
        printf("Converting file: %s\n", path);

        // Baca isi file .txt
        char hex_string[1024];
        ssize_t bytes_read = read(fd, hex_string, sizeof(hex_string) - 1);
        if (bytes_read < 0) {
            perror("read() error");
            close(fd);
            return -errno;
        }

        hex_string[bytes_read] = '\0';  // Null-terminate the string

        // Memisahkan nama file tanpa ekstensi .txt
        char base_name[1024];
        strncpy(base_name, path + 1, strlen(path) - 4);  // Menghapus ekstensi '.txt' dari nama file

        // Format penamaan gambar yang benar
        char output_image_path[1024];
        char date_str[11], time_str[9];
        get_current_time_str(date_str, time_str);
        sprintf(output_image_path, "%s_image_%s_%s.png", base_name, date_str, time_str); // Format yang benar

        // Tentukan direktori image
        char image_folder[1024];
        snprintf(image_folder, sizeof(image_folder), "%s%s", dirpath, image_dir);

        // Buat folder image jika belum ada
        struct stat st = {0};
        if (stat(image_folder, &st) == -1) {
            mkdir(image_folder, 0700);
        }

        // Tentukan path lengkap untuk gambar
        char full_image_path[1024];
        snprintf(full_image_path, sizeof(full_image_path), "%s/%s", image_folder, output_image_path);

        // Konversi hex ke gambar
        convert_hex_to_image(hex_string, full_image_path);

        // Log konversi
        log_conversion(path, full_image_path);
    }

    close(fd);
    return res;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read = xmp_read,
};

int main(int argc, char *argv[]) {
    printf("Starting program...\n");

    // Memastikan folder 'anomali' dan 'anomali/image' ada
    ensure_anomali_folder_exists();

    // Menjalankan fungsi download dan ekstrak file terlebih dahulu
    download_and_extract_files();
    printf("File extraction completed.\n");

    // Mengatur dirpath ke direktori saat ini dan folder anomali
    set_dirpath();
    printf("Directory path set.\n");

    umask(0);

    // Menjalankan FUSE
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
