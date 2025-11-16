# print_uid

A simple C program that demonstrates retrieving the current user's ID.

## Purpose

This program prints the User ID (UID) of the user running the program. It's a minimal example showing how to query process identity information using system calls.

## System Calls Used

- **`getuid()`** - Returns the real user ID of the calling process

## Concepts Demonstrated

- **Process Identity** - Every process has an associated user ID that determines its permissions
- **Real UID** - The actual user who started the process (vs effective UID for privilege management)
- **Basic System Call Usage** - Simplest possible example of making a syscall and using its return value

## Usage

```bash
./print_uid
```

**Example output:**
```
Current UID: 1000
```

## Compilation

```bash
gcc -o print_uid print_uid.c
```

## Related Concepts

- `getuid()` returns the **real** user ID
- `geteuid()` would return the **effective** user ID (useful for setuid programs)
- UIDs are integers that identify users on Unix-like systems
- The `id` command shows similar information (UID, GID, groups)

## See Also

- `fileperms/` - Uses `getpwuid()` to convert UIDs to usernames
- `man 2 getuid` - Manual page for the getuid system call
