#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <string.h>

void print_file_type(mode_t mode) {
    printf("File type:        ");
    if (S_ISREG(mode))       printf("Regular file\n");
    else if (S_ISDIR(mode))  printf("Directory\n");
    else if (S_ISLNK(mode))  printf("Symbolic link\n");
    else if (S_ISCHR(mode))  printf("Character device\n");
    else if (S_ISBLK(mode))  printf("Block device\n");
    else if (S_ISFIFO(mode)) printf("FIFO/pipe\n");
    else if (S_ISSOCK(mode)) printf("Socket\n");
    else                     printf("Unknown\n");
}

void print_permissions_symbolic(mode_t mode) {
    printf("Permissions:      ");

    // Owner permissions
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");

    // Group permissions
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");

    // Other permissions
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");

    printf("\n");
}

void print_permissions_octal(mode_t mode) {
    printf("Octal:            0%o\n", mode & 0777);
}

void print_owner_group(struct stat *st) {
    struct passwd *pwd = getpwuid(st->st_uid);
    struct group *grp = getgrgid(st->st_gid);

    printf("Owner:            %s (UID: %d)\n",
           pwd ? pwd->pw_name : "unknown", st->st_uid);
    printf("Group:            %s (GID: %d)\n",
           grp ? grp->gr_name : "unknown", st->st_gid);
}

void print_size(struct stat *st) {
    printf("Size:             %ld bytes\n", st->st_size);
}

void print_timestamps(struct stat *st) {
    char timebuf[80];

    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
             localtime(&st->st_mtime));
    printf("Last modified:    %s\n", timebuf);

    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
             localtime(&st->st_atime));
    printf("Last accessed:    %s\n", timebuf);
}

void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s <file>\n", prog);
    fprintf(stderr, "Display file permissions and metadata\n");
}

int main(int argc, char *argv[]) {
    struct stat file_stat;

    // Check command-line arguments
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *filepath = argv[1];

    // Call stat() to get file information
    if (stat(filepath, &file_stat) == -1) {
        perror("stat");
        fprintf(stderr, "Error: Cannot stat '%s'\n", filepath);
        return 1;
    }

    // Print file information
    printf("\nFile: %s\n", filepath);
    printf("=====================================\n");

    print_file_type(file_stat.st_mode);
    print_permissions_symbolic(file_stat.st_mode);
    print_permissions_octal(file_stat.st_mode);
    print_owner_group(&file_stat);
    print_size(&file_stat);
    print_timestamps(&file_stat);

    printf("=====================================\n\n");

    return 0;
}
