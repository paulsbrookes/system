# File Permissions Tool

A CLI tool that displays file permissions and metadata using the `stat()` system call.

## What It Does

This program takes a file path as an argument and displays:
- **File type** (regular file, directory, symlink, device, etc.)
- **Permissions** in symbolic format (rwxrwxrwx)
- **Permissions** in octal format (0755)
- **Owner** and user ID
- **Group** and group ID
- **File size** in bytes
- **Timestamps** (last modified, last accessed)

## How to Use

### Compile
```bash
gcc -Wall -O2 fileperms.c -o fileperms
```

### Run
```bash
./fileperms <file>
```

### Examples
```bash
# Check permissions of a file
./fileperms fileperms.c

# Check permissions of a directory
./fileperms /home/paul

# Check permissions of a binary
./fileperms /bin/ls
```

### Example Output
```
File: fileperms.c
=====================================
File type:        Regular file
Permissions:      rw-------
Octal:            0600
Owner:            paul (UID: 1001)
Group:            paul (GID: 1001)
Size:             3139 bytes
Last modified:    2025-11-16 22:33:28
Last accessed:    2025-11-16 22:33:39
=====================================
```

## System Calls and Functions Used

### Main System Call
- **`stat()`** - Gets file status/metadata
  ```c
  struct stat file_stat;
  stat(filepath, &file_stat);
  ```

  The `stat()` system call fills a `struct stat` with information about the file:
  - `st_mode` - File type and permissions
  - `st_uid` - Owner user ID
  - `st_gid` - Owner group ID
  - `st_size` - File size in bytes
  - `st_mtime` - Last modification time
  - `st_atime` - Last access time
  - And more...

### Helper Functions
- **`getpwuid()`** - Converts UID to username (from `/etc/passwd`)
- **`getgrgid()`** - Converts GID to group name (from `/etc/group`)
- **`localtime()`** - Converts timestamp to local time
- **`strftime()`** - Formats time as string

### File Type Macros
The program uses these macros to check file types:
- **`S_ISREG(mode)`** - Regular file
- **`S_ISDIR(mode)`** - Directory
- **`S_ISLNK(mode)`** - Symbolic link
- **`S_ISCHR(mode)`** - Character device
- **`S_ISBLK(mode)`** - Block device
- **`S_ISFIFO(mode)`** - Named pipe (FIFO)
- **`S_ISSOCK(mode)`** - Socket

### Permission Bits
The program checks these permission bits:
- **Owner**: `S_IRUSR` (read), `S_IWUSR` (write), `S_IXUSR` (execute)
- **Group**: `S_IRGRP` (read), `S_IWGRP` (write), `S_IXGRP` (execute)
- **Others**: `S_IROTH` (read), `S_IWOTH` (write), `S_IXOTH` (execute)

## Understanding File Permissions

### Symbolic Format (rwxrwxrwx)
Permissions are divided into three groups of three:
```
rwx  rwx  rwx
│    │    └─ Others (everyone else)
│    └────── Group (users in file's group)
└─────────── Owner (file owner)
```

Each group has:
- **r** - Read permission (4 in octal)
- **w** - Write permission (2 in octal)
- **x** - Execute permission (1 in octal)

### Octal Format (0755)
Permissions can be represented as a 3-digit octal number:
- Each digit is the sum of: read(4) + write(2) + execute(1)
- Example: `0755` means:
  - Owner: 7 = 4+2+1 = rwx
  - Group: 5 = 4+0+1 = r-x
  - Others: 5 = 4+0+1 = r-x

Common permission values:
- `0644` - rw-r--r-- (typical file: owner can write, others can read)
- `0755` - rwxr-xr-x (typical executable: owner can write, all can execute)
- `0777` - rwxrwxrwx (everyone can do everything - usually bad!)
- `0600` - rw------- (private file: only owner can read/write)

## How stat() Works

The `stat()` system call:
1. Takes a file path
2. Queries the file system
3. Reads the file's **inode** (metadata structure on disk)
4. Fills a `struct stat` with the information
5. Returns 0 on success, -1 on error

It does **not** require read permission on the file itself, only execute permission on the directory containing it.

### Related System Calls
- **`fstat()`** - Same as stat(), but takes file descriptor instead of path
- **`lstat()`** - Like stat(), but doesn't follow symbolic links
- **`fstatat()`** - stat() relative to a directory file descriptor

## Comparison with ls -l

This tool shows similar information to `ls -l`:
```bash
ls -l fileperms.c
# Output: -rw------- 1 paul paul 3139 Nov 16 22:33 fileperms.c

./fileperms fileperms.c
# Shows the same info but in a more detailed format
```

Both use the `stat()` system call internally!

## Learning Points

This program demonstrates:
1. **stat() system call** - Getting file metadata
2. **Bit manipulation** - Checking permission bits with bitwise AND
3. **User/group lookup** - Converting IDs to names
4. **Time formatting** - Working with timestamps
5. **Command-line arguments** - argc/argv handling
6. **Error handling** - Checking return values and using perror()

## Try It Out

```bash
# Compare output with ls -l
ls -l /bin/ls
./fileperms /bin/ls

# Check a directory
./fileperms /tmp

# Check a file that doesn't exist (will show error)
./fileperms /nonexistent
```

This tool is similar to the Unix `stat` command, which also displays file metadata!
