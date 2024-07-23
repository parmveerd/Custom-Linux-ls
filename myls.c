#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define MAX_PATH_LENGTH 1024

#define FILE_TYPE_MASK S_IFMT
#define FILE_PERM_MASK (S_IRWXU | S_IRWXG | S_IRWXO)

int finished = 0;

// Function prototypes
void list_directory(const char *path, int is_recursive, int show_index, int show_details);
void print_file_info(const char *path, int show_index, int show_details);
void sort_lexicographically(char *arr[], int size);
char *format_date(time_t t);

int main(int argc, char *argv[]) {
    // Parse command line arguments
    int show_index = 0;
    int show_details = 0;
    int is_recursive = 0;
    int path_start_index = argc;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'i':
                        show_index = 1;
                        break;
                    case 'l':
                        show_details = 1;
                        break;
                    case 'R':
                        is_recursive = 1;
                        break;
                    default:
                        printf("Error: Unsupported Option\n");
                        return 1;
                }
            }
        } else {
            path_start_index = i;
            break;
        }
    }

    // If no paths provided, list the current directory
    if (path_start_index == argc) {
        list_directory(".", is_recursive, show_index, show_details);
    } else {
        // List provided directories and files
        for (int i = path_start_index; i < argc; i++) {
            list_directory(argv[i], is_recursive, show_index, show_details);
            if (finished == 1) {
                return 1;
            }
            printf("\n");
        }
    }
    return 0;
}



void list_directory(const char *path, int is_recursive, int show_index, int show_details) {
    DIR *dir;
    struct dirent *entry;
    char *entries[1024];
    char *subdirectories[1024];
    int num_entries = 0;
    int num_subdirectories = 0;
    int is_main_directory = 1;

    // Open the directory
    if ((dir = opendir(path)) == NULL) {
        if (access(path, F_OK) == -1) {
            printf("Error: Nonexistent files or directories\n");
            finished = 1;
            return;
        } else {
            print_file_info(path, show_index, show_details);
            return;
        }
    }

    // Read entries from the directory
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char full_path[MAX_PATH_LENGTH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

            struct stat st;
            if (lstat(full_path, &st) == -1) {
                perror("Error");
                continue;
            }

            if (S_ISDIR(st.st_mode)) {
                // Store subdirectories separately for later processing
                subdirectories[num_subdirectories] = strdup(entry->d_name);
                num_subdirectories++;
                entries[num_entries] = strdup(entry->d_name);
                num_entries++;
            } else {
                // Store regular files in the main directory
                entries[num_entries] = strdup(entry->d_name);
                num_entries++;
            }
        }
    }

    closedir(dir);

    // Sort the entries lexicographically
    sort_lexicographically(entries, num_entries);
    sort_lexicographically(subdirectories, num_subdirectories);

    // Print the entries in the main directory
    for (int i = 0; i < num_entries; i++) {
        if (is_main_directory) {
            is_main_directory = 0;
            printf("%s:\n", path); // Print the main directory name
        }

        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entries[i]);

        struct stat st;
        if (lstat(full_path, &st) == -1) {
            perror("Error");
            continue;
        }

        print_file_info(full_path, show_index, show_details);
        free(entries[i]);
    }

    // Print the subdirectories after all regular files
    for (int i = 0; i < num_subdirectories; i++) {
        if (is_main_directory) {
            is_main_directory = 0;
            printf("%s:\n", path); // Print the main directory name
        }

        char subdirectory_path[MAX_PATH_LENGTH];
        snprintf(subdirectory_path, sizeof(subdirectory_path), "%s/%s", path, subdirectories[i]);


        // If recursive is enabled, list the subdirectory contents
        if (is_recursive) {
            printf("\n");
            list_directory(subdirectory_path, is_recursive, show_index, show_details);
        }
        free(subdirectories[i]);
    }
}



void print_file_info(const char *path, int show_index, int show_details) {    
    struct stat st;
    if (lstat(path, &st) == -1) {
        perror("Error");
        return;
    }

    const char *temp_name = strrchr(path, '/');
    if (temp_name && temp_name[1] == '.') {
        return;
    }

    // Print index number if requested
    if (show_index) {
        printf("%-10lu ", (unsigned long)st.st_ino);
    }

    // Print detailed information if requested
    if (show_details) {
        char file_mode[11]; // File mode buffer

        // Determine the file type
        switch (st.st_mode & S_IFMT) {
            case S_IFREG:
                file_mode[0] = '-';
                break;
            case S_IFDIR:
                file_mode[0] = 'd';
                break;
            case S_IFLNK:
                file_mode[0] = 'l';
                break;
            default:
                file_mode[0] = '?';
        }

        // Set file permissions for owner, group, and others
        file_mode[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        file_mode[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        file_mode[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
        file_mode[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
        file_mode[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
        file_mode[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
        file_mode[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
        file_mode[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
        file_mode[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
        file_mode[10] = '\0'; // Null-terminate the file mode string

        printf("%-11s ", file_mode); // Print the file mode
        printf("%-6ld ", (long)st.st_nlink);
        printf("%-15s ", getpwuid(st.st_uid)->pw_name);
        printf("%-15s ", getgrgid(st.st_gid)->gr_name);
        printf("%-10lld ", (long long)st.st_size);
        printf("%s ", format_date(st.st_mtime));


        const char *filename = strrchr(path, '/');
        if (filename) {
            printf("%s ", filename + 1);
        } else {
            printf("%s ", path);
        }
        // Check if it's a symbolic link
        if (S_ISLNK(st.st_mode)) {
            char target_path[MAX_PATH_LENGTH];
            ssize_t target_len = readlink(path, target_path, sizeof(target_path) - 1);
            if (target_len != -1) {
                target_path[target_len] = '\0'; // Null-terminate the target path string
                printf("-> %s", target_path);
            }
        }
        printf("\n");
    } else {
        const char *filename = strrchr(path, '/');
        if (filename) {
            printf("%s\n", filename + 1);
        } else {
            printf("%s\n", path);
        }
    }
}


void sort_lexicographically(char *arr[], int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (strcmp(arr[j], arr[j + 1]) > 0) {
                char *temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}


char *format_date(time_t t) {
    static char buffer[30];
    struct tm *timeinfo = localtime(&t);
    strftime(buffer, sizeof(buffer), "%b %d %Y %H:%M", timeinfo);
    return buffer;
}

