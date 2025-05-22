#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h> // Untuk mkdir, stat
#include <sys/time.h> 
#include <stdlib.h>
#include <time.h>     
#include <ctype.h>    
#include <math.h>     
#include <stdint.h> // Untuk uint16_t, uint32_t

#define MAX_HEX_FILE_SIZE_FOR_CONVERSION (20 * 1024 * 1024) 
#define OUTPUT_IMAGE_SUBDIR "image" 
#define LOG_FILE_NAME "conversion.log" 

// Variabel Global
char original_program_cwd[1024]; 
char anomali_dir_path[2048];     
static const char *dirpath;      

// --- Deklarasi Fungsi ---
void set_anomali_dirpath(); 
void download_and_extract_files();
void ensure_anomali_structure_exists();
void get_current_time_str_for_log(char *datetime_str, size_t len);
void get_current_time_str_for_filename(char *date_str, char *time_str);
int robust_hex_string_to_bytes(const char *raw_hex_input, unsigned char **out_bytes, size_t *out_len);
void log_conversion_entry(const char *source_file_name, const char *output_image_path);
int convert_hex_to_png(const unsigned char *pixel_data, int width, int height, const char *output_path);


// Fungsi untuk mendapatkan direktori kerja saat ini dan mengatur path 'anomali'
void set_anomali_dirpath() {
    if (getcwd(original_program_cwd, sizeof(original_program_cwd)) == NULL) {
        perror("getcwd() untuk original_program_cwd gagal");
        exit(1); 
    }
    snprintf(anomali_dir_path, sizeof(anomali_dir_path), "%s/anomali", original_program_cwd);
    dirpath = anomali_dir_path; 
    printf("Original program CWD: %s\n", original_program_cwd);
    printf("Path direktori 'anomali' (sumber data) diatur ke: %s\n", anomali_dir_path);
}

// Fungsi untuk mendownload dan mengekstrak file ZIP
void download_and_extract_files() {
    printf("Mengunduh file (dari CWD: %s)...\n", getcwd(NULL,0)); 
    if (system("wget --no-check-certificate \"https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download\" -O anomali.zip") != 0) {
        fprintf(stderr, "Error: Gagal mengunduh anomali.zip\n");
    }
    
    printf("Mengekstrak file (ke CWD: %s)...\n", getcwd(NULL,0)); 
    if (system("unzip -o anomali.zip") != 0) { 
         fprintf(stderr, "Error: Gagal mengekstrak anomali.zip.\n");
    }
    
    system("rm -f anomali.zip"); 
    printf("Download dan ekstraksi selesai.\n");
}

// Fungsi untuk memastikan folder 'anomali' dan 'anomali/image' ada
void ensure_anomali_structure_exists() {
    struct stat st = {0};
    if (stat("anomali", &st) == -1) { 
        if (mkdir("anomali", 0755) == -1) {
            perror("Gagal membuat direktori 'anomali' (relatif terhadap CWD program)");
            exit(1); 
        }
        printf("Direktori 'anomali' (relatif terhadap CWD program) dibuat.\n");
    }

    char image_output_path_relatif[1024 + sizeof(OUTPUT_IMAGE_SUBDIR) + 2]; 
    snprintf(image_output_path_relatif, sizeof(image_output_path_relatif), "anomali/%s", OUTPUT_IMAGE_SUBDIR);

    if (stat(image_output_path_relatif, &st) == -1) {
        if (mkdir(image_output_path_relatif, 0755) == -1) {
            char err_msg[1200];
            snprintf(err_msg, sizeof(err_msg), "Gagal membuat direktori output gambar (%s)", image_output_path_relatif);
            perror(err_msg);
        } else {
            printf("Direktori '%s' dibuat.\n", image_output_path_relatif);
        }
    }
}

// Fungsi untuk mendapatkan waktu saat ini dalam format YYYY-MM-DD_HH:MM:SS untuk nama file
void get_current_time_str_for_filename(char *date_str, char *time_str) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(date_str, 11, "%Y-%m-%d", t); 
    strftime(time_str, 9, "%H:%M:%S", t);   
}

// Fungsi untuk mendapatkan waktu saat ini dalam format [YYYY-MM-DD][HH:MM:SS] untuk log
void get_current_time_str_for_log(char *datetime_str, size_t len) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(datetime_str, len, "[%Y-%m-%d][%H:%M:%S]", t);
}

// Fungsi helper: konversi satu karakter heksadesimal ke nilai integer-nya
static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1; 
}

// Fungsi helper: Mengonversi string yang berisi karakter heksadesimal menjadi array byte.
int robust_hex_string_to_bytes(const char *raw_hex_input, unsigned char **out_bytes, size_t *out_len) {
    size_t raw_len = strlen(raw_hex_input);
    if (raw_len == 0) {
        *out_bytes = NULL; *out_len = 0; return 0;
    }

    char *filtered_hex_str = (char *)malloc(raw_len + 1);
    if (!filtered_hex_str) {
        perror("malloc untuk filtered_hex_str gagal");
        *out_bytes = NULL; *out_len = 0; return 0;
    }

    size_t filtered_idx = 0;
    for (size_t i = 0; i < raw_len; ++i) {
        if (isxdigit((unsigned char)raw_hex_input[i])) {
            filtered_hex_str[filtered_idx++] = raw_hex_input[i];
        }
    }
    filtered_hex_str[filtered_idx] = '\0';

    if (filtered_idx == 0 || filtered_idx % 2 != 0) { 
        free(filtered_hex_str);
        *out_bytes = NULL; *out_len = 0;
        if (filtered_idx % 2 != 0 && filtered_idx != 0) fprintf(stderr, "Error: Jumlah digit heksadesimal ganjil (%zu) setelah filtering.\n", filtered_idx);
        else if (filtered_idx == 0) fprintf(stderr, "Error: Tidak ada digit heksadesimal valid ditemukan.\n");
        return 0;
    }

    *out_len = filtered_idx / 2;
    *out_bytes = (unsigned char *)malloc(*out_len);
    if (!*out_bytes) {
        perror("malloc untuk out_bytes gagal");
        free(filtered_hex_str); return 0;
    }

    for (size_t i = 0; i < *out_len; ++i) {
        int hi = hex_char_to_int(filtered_hex_str[i * 2]);
        int lo = hex_char_to_int(filtered_hex_str[i * 2 + 1]);
        (*out_bytes)[i] = (unsigned char)((hi << 4) + lo);
    }
    free(filtered_hex_str);
    return 1;
}

// Fungsi untuk menulis entri log konversi
void log_conversion_entry(const char *source_file_name, const char *output_image_path) {
    char log_file_full_path[sizeof(anomali_dir_path) + sizeof(LOG_FILE_NAME) + 2];
    snprintf(log_file_full_path, sizeof(log_file_full_path), "%s/%s", anomali_dir_path, LOG_FILE_NAME);

    FILE *log_fp = fopen(log_file_full_path, "a"); 
    if (log_fp == NULL) {
        char err_msg[sizeof(log_file_full_path) + 100];
        snprintf(err_msg, sizeof(err_msg), "Gagal membuka file log '%s' untuk ditulis", log_file_full_path);
        perror(err_msg);
        return;
    }

    char datetime_str[30];
    get_current_time_str_for_log(datetime_str, sizeof(datetime_str));

    fprintf(log_fp, "%s Berhasil mengonversi %s menjadi %s\n",
            datetime_str, source_file_name, output_image_path);
    
    printf("Log ditulis: %s -> %s\n", source_file_name, output_image_path);
    fclose(log_fp);
}

// Fungsi untuk mengonversi Hex ke PNG
int convert_hex_to_png(const unsigned char *pixel_data, int width, int height, const char *output_path) {
    // Simpan data pixel ke dalam file raw
    FILE *raw_file = fopen("raw_data.raw", "wb");
    if (!raw_file) {
        perror("Gagal membuat file raw_data.raw");
        return -1;
    }
    fwrite(pixel_data, sizeof(unsigned char), width * height, raw_file);
    fclose(raw_file);

    // Gunakan perintah ImageMagick `convert` untuk mengonversi file raw menjadi PNG
    char command[1024];
    snprintf(command, sizeof(command), "convert -size %dx%d gray:raw_data.raw %s", width, height, output_path);

    // Jalankan perintah sistem untuk mengonversi ke PNG
    if (system(command) != 0) {
        perror("Konversi ke PNG gagal");
        return -1;
    }

    // Hapus file raw setelah konversi selesai
    remove("raw_data.raw");

    return 0;
}

// Fungsi membaca file .txt berisi hex string
int read_hex_from_file(const char *file_path, char **hex_string) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Gagal membuka file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *hex_string = (char *)malloc(file_size + 1);
    if (!*hex_string) {
        perror("Gagal mengalokasikan memori untuk string hex");
        fclose(file);
        return -1;
    }

    fread(*hex_string, 1, file_size, file);
    (*hex_string)[file_size] = '\0';

    fclose(file);
    return 0;
}

// --- Implementasi FUSE ---
static int xmp_getattr(const char *path, struct stat *stbuf) {
    char fpath[sizeof(anomali_dir_path) + strlen(path) + 2]; 
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path); 
    int res = lstat(fpath, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void)offset; (void)fi;
    char fpath[sizeof(anomali_dir_path) + strlen(path) + 2];
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);

    // Jika direktori yang diminta adalah /mnt/image, pastikan folder image ada
    if (strcmp(path, "/image") == 0) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_mode = S_IFDIR | 0755;
        filler(buf, ".", &st, 0);
        filler(buf, "..", &st, 0);

        DIR *dp = opendir(fpath);
        if (dp == NULL) return -errno;

        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            struct stat st;
            memset(&st, 0, sizeof(st));

            // Set path file
            char entry_full_path[sizeof(fpath) + NAME_MAX + 2];
            snprintf(entry_full_path, sizeof(entry_full_path), "%s/%s", fpath, de->d_name);
            if(lstat(entry_full_path, &st) == -1) {
                st.st_ino = de->d_ino;
                if (de->d_type == DT_DIR) st.st_mode = S_IFDIR | 0755;
                else if (de->d_type == DT_REG) st.st_mode = S_IFREG | 0644;
                else if (de->d_type == DT_LNK) st.st_mode = S_IFLNK | 0777;
                else st.st_mode = S_IFREG | 0644;
            }
            // Isi direktori dengan file yang ada di dalamnya
            if (filler(buf, de->d_name, &st, 0) != 0) break;
        }
        closedir(dp);
        return 0;
    }

    // Lanjutkan untuk direktori lainnya
    DIR *dp = opendir(fpath);
    if (dp == NULL) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));

        char entry_full_path[sizeof(fpath) + NAME_MAX + 2];
        snprintf(entry_full_path, sizeof(entry_full_path), "%s/%s", fpath, de->d_name);
        if(lstat(entry_full_path, &st) == -1) {
            st.st_ino = de->d_ino; 
            if (de->d_type == DT_DIR) st.st_mode = S_IFDIR | 0755;
            else if (de->d_type == DT_REG) st.st_mode = S_IFREG | 0644;
            else if (de->d_type == DT_LNK) st.st_mode = S_IFLNK | 0777;
            else st.st_mode = S_IFREG | 0644; 
        }
        if (filler(buf, de->d_name, &st, 0) != 0) break;
    }
    closedir(dp);
    return 0;
}

static int xmp_read(const char *path, char *read_buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void)fi;

    char fpath_to_read[sizeof(anomali_dir_path) + strlen(path) + 2]; 
    snprintf(fpath_to_read, sizeof(fpath_to_read), "%s%s", dirpath, path);

    int fd = open(fpath_to_read, O_RDONLY);
    if (fd == -1) {
        return -errno;
    }

    ssize_t res_read_op = pread(fd, read_buf, size, offset);
    if (res_read_op == -1) {
        int pread_errno = errno; close(fd); return -pread_errno;
    }

    // Mengecek jika file adalah .txt dan konversi ke PNG
    if (offset == 0 && strstr(path, ".txt") != NULL) {
        struct stat st_for_type;
        if (fstat(fd, &st_for_type) == 0 && S_ISREG(st_for_type.st_mode)) {
            long file_size_for_conv = st_for_type.st_size;
            if (file_size_for_conv > 0 && file_size_for_conv < MAX_HEX_FILE_SIZE_FOR_CONVERSION) {
                char *hex_content_buffer = (char *)malloc(file_size_for_conv + 1);
                if (hex_content_buffer) {
                    lseek(fd, 0, SEEK_SET); 
                    ssize_t full_read_bytes = read(fd, hex_content_buffer, file_size_for_conv);
                    if (full_read_bytes == file_size_for_conv) {
                        hex_content_buffer[file_size_for_conv] = '\0';

                        unsigned char *image_data_bytes = NULL;
                        size_t image_data_len = 0; 
                        if (robust_hex_string_to_bytes(hex_content_buffer, &image_data_bytes, &image_data_len) && image_data_len > 0) {
                            int img_width = 0, img_height = 0;
                            double sqrt_len = sqrt((double)image_data_len);
                            int side = (int)round(sqrt_len);

                            if ((size_t)side * side == image_data_len) { 
                                img_width = img_height = side;
                            } else { 
                                img_width = (int)image_data_len; 
                                img_height = 1;
                            }

                            if (img_width > 0 && img_height > 0) {
                                const char *leaf_name_start = strrchr(path, '/');
                                leaf_name_start = leaf_name_start ? leaf_name_start + 1 : path;
                                 
                                char base_file_name[256]; 
                                strncpy(base_file_name, leaf_name_start, sizeof(base_file_name) - 1);
                                base_file_name[sizeof(base_file_name) - 1] = '\0';
                                char *dot_ext = strrchr(base_file_name, '.');
                                if (dot_ext && strcmp(dot_ext, ".txt") == 0) *dot_ext = '\0';

                                char date_str_fn[11], time_str_fn[9];
                                get_current_time_str_for_filename(date_str_fn, time_str_fn);
                                 
                                char output_image_leaf_name[sizeof(base_file_name) + 50]; 
                                snprintf(output_image_leaf_name, sizeof(output_image_leaf_name),
                                         "%s_image_%s_%s.png", base_file_name, date_str_fn, time_str_fn); 

                                char full_image_path_to_save[sizeof(anomali_dir_path) + sizeof(OUTPUT_IMAGE_SUBDIR) + sizeof(output_image_leaf_name) + 4]; 
                                snprintf(full_image_path_to_save, sizeof(full_image_path_to_save),
                                         "%s/%s/%s", anomali_dir_path, OUTPUT_IMAGE_SUBDIR, output_image_leaf_name);
                                 
                                if (convert_hex_to_png(image_data_bytes, img_width, img_height, full_image_path_to_save) == 0) {
                                    log_conversion_entry(path, full_image_path_to_save); 
                                } else {
                                    fprintf(stderr, "Error: Gagal menulis PNG %s\n", full_image_path_to_save);
                                }
                            }
                            free(image_data_bytes);
                        }
                    } else {
                        char err_msg[sizeof(fpath_to_read) + 100]; 
                        snprintf(err_msg, sizeof(err_msg), "read seluruh konten file '%s' untuk konversi gagal", fpath_to_read);
                    }
                    free(hex_content_buffer);
                }
            }
        }
    }
    close(fd);
    return res_read_op;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read = xmp_read,
};

int main(int argc, char *argv[]) {
    set_anomali_dirpath(); 
    printf("Memulai program FUSE...\n");

    // Periksa apakah mountpoint diberikan sebagai argumen
    if (argc < 2) {
        fprintf(stderr, "Penggunaan: %s <mountpoint> [opsi FUSE...]\n", argv[0]);
        return 1;
    }
    char *mountpoint_arg = argv[1]; // Asumsikan argumen kedua adalah mountpoint

    struct stat st_mountpoint = {0};
    if (stat(mountpoint_arg, &st_mountpoint) == -1) {
        if (errno == ENOENT) { 
            printf("Mount point '%s' tidak ditemukan, mencoba membuat...\n", mountpoint_arg);
            if (mkdir(mountpoint_arg, 0755) == -1) { 
                char err_buf[strlen(mountpoint_arg) + 100];
                snprintf(err_buf, sizeof(err_buf), "Gagal membuat direktori mount point '%s'", mountpoint_arg);
                perror(err_buf);
                return 1;
            }
            printf("Direktori mount point '%s' berhasil dibuat.\n", mountpoint_arg);
        } else { 
            char err_buf[strlen(mountpoint_arg) + 100];
            snprintf(err_buf, sizeof(err_buf), "Error saat memeriksa mount point '%s'", mountpoint_arg);
            perror(err_buf);
            return 1;
        }
    } else { 
        if (!S_ISDIR(st_mountpoint.st_mode)) {
            fprintf(stderr, "Error: Path mount point '%s' ada tetapi bukan direktori.\n", mountpoint_arg);
            return 1;
        }
        printf("Mount point '%s' sudah ada dan merupakan direktori.\n", mountpoint_arg);
    }

    ensure_anomali_structure_exists();
    download_and_extract_files();
    
    printf("Menjalankan FUSE filesystem di '%s'.\n", mountpoint_arg); // Menggunakan mountpoint_arg
    printf("Gambar yang dikonversi akan disimpan di: %s/%s/\n", dirpath, OUTPUT_IMAGE_SUBDIR);
    printf("Log konversi akan disimpan di: %s/%s\n", dirpath, LOG_FILE_NAME);

    umask(0); 
    // Panggil fuse_main dengan argc dan argv asli.
    // FUSE akan mem-parse opsi dan mountpoint dari sini.
    int fuse_ret = fuse_main(argc, argv, &xmp_oper, NULL);
    
    return fuse_ret;
}
