# Grep TUI - Interactive Grep Browser

A text user interface (TUI) for browsing grep search results with vim integration.

## Features

- **Search with grep** - Runs `grep -rn` to recursively search files
- **Scrollable results** - Navigate with arrow keys and Page Up/Down
- **Open in vim** - Press Enter to open file at exact line number
- **Context preview** - Toggle surrounding lines for selected result
- **Interactive filtering** - Filter results on-the-fly
- **Syntax highlighting** - Color-coded interface (if terminal supports it)

## What It Does

This tool combines the power of grep with an interactive TUI:

1. Runs grep as a subprocess to search for your pattern
2. Parses and displays all results in an ncurses interface
3. Lets you scroll through results with keyboard navigation
4. Shows file context around matches
5. Opens files directly in vim at the matching line
6. Allows filtering results interactively

## How to Use

### Compile
```bash
gcc -Wall -O2 greptui.c -o greptui -lncurses
```

### Run
```bash
./greptui <pattern> [directory]
```

**Examples:**
```bash
# Search for "main" in current directory
./greptui main

# Search for "TODO" in src/ directory
./greptui TODO src/

# Search for a function name
./greptui "void setup"
```

### Keybindings

| Key | Action |
|-----|--------|
| `↑` / `↓` | Navigate up/down through results |
| `Page Up` / `Page Down` | Scroll page at a time |
| `Enter` | Open selected file in vim at line number |
| `/` | Enter interactive filter mode |
| `c` | Toggle context preview (show surrounding lines) |
| `q` | Quit |

### Filter Mode

Press `/` to enter filter mode:
- Type to filter results by filename or content
- Results update in real-time
- Press `Enter` or `Esc` to exit filter mode
- Press `Backspace` to delete filter characters

## System Calls and Functions Used

### Process Management

**`popen()`** - Run grep as subprocess and read output
```c
FILE *fp = popen("grep -rn 'pattern' directory", "r");
```
- Creates a pipe and forks a child process
- Executes the shell command
- Returns a file pointer to read the command's output
- Internally uses `pipe()`, `fork()`, and `exec()`

**`fork()`** - Create child process for vim
```c
pid_t pid = fork();
if (pid == 0) {
    // Child process
    execlp("vim", "vim", "+42", "file.c", NULL);
}
```
- Creates an exact copy of the current process
- Returns 0 in child, child PID in parent

**`execlp()`** - Replace process with vim
```c
execlp("vim", "vim", "+42", "file.c", NULL);
```
- Replaces current process with vim
- `+42` tells vim to jump to line 42
- Searches for `vim` in PATH
- If successful, never returns (process is replaced)

**`waitpid()`** - Wait for vim to finish
```c
waitpid(pid, NULL, 0);
```
- Blocks until child process (vim) exits
- Prevents zombie processes
- Allows parent to reclaim resources

### File I/O

**`fopen()` / `fgets()` / `fclose()`** - Read context lines
```c
FILE *fp = fopen(filename, "r");
fgets(line, sizeof(line), fp);
fclose(fp);
```
- Opens file for reading
- Reads line by line to get context around match
- Closes file when done

**`pclose()`** - Close pipe from popen()
```c
pclose(fp);
```
- Closes pipe and waits for subprocess to finish
- Returns the exit status of the command

### ncurses Functions

**Initialization:**
- `initscr()` - Initialize ncurses
- `cbreak()` - Disable line buffering
- `noecho()` - Don't echo typed characters
- `curs_set(0)` - Hide cursor
- `keypad(stdscr, TRUE)` - Enable arrow keys

**Window management:**
- `newwin()` - Create new window
- `delwin()` - Delete window
- `werase()` - Clear window contents
- `wrefresh()` - Update window on screen

**Drawing:**
- `mvwprintw()` - Move and print to window
- `box()` - Draw border around window
- `wattron()` / `wattroff()` - Turn attributes on/off

**Input:**
- `wgetch()` - Get character from window
- Special keys: `KEY_UP`, `KEY_DOWN`, `KEY_PPAGE`, `KEY_NPAGE`

**ncurses Suspend/Resume:**
- `def_prog_mode()` - Save current terminal state
- `endwin()` - Temporarily exit ncurses mode
- `reset_prog_mode()` - Restore ncurses mode
- `refresh()` - Refresh screen after restore

This allows the TUI to suspend when opening vim, then resume afterwards!

## How It Works

### 1. Running Grep

```c
popen("grep -rn 'pattern' directory", "r");
```

The `popen()` call:
1. Creates a pipe for communication
2. Forks a child process
3. In child: redirects stdout to pipe, executes shell with grep command
4. In parent: returns FILE* to read from pipe
5. Parent reads grep output line by line

### 2. Parsing Grep Output

Grep with `-n` outputs format: `filename:line_number:content`

```c
void parse_grep_line(const char *line, GrepResult *result) {
    // Find first and second colons
    // Extract: filename, line_number, content
}
```

### 3. Displaying with ncurses

```c
for (int i = scroll_offset; i < result_count; i++) {
    if (i == current_selection) {
        wattron(win, A_REVERSE);  // Highlight selected
    }
    mvwprintw(win, y, x, "%s:%d", filename, line_num);
    wattroff(win, A_REVERSE);
}
```

### 4. Opening in Vim

```c
def_prog_mode();       // Save ncurses state
endwin();              // Exit ncurses temporarily

pid_t pid = fork();
if (pid == 0) {
    execlp("vim", "vim", "+42", "file.c", NULL);
    exit(1);
}
waitpid(pid, NULL, 0); // Wait for vim to close

reset_prog_mode();     // Restore ncurses
refresh();             // Refresh display
```

**Why suspend ncurses?**
- Vim needs full control of the terminal
- ncurses and vim can't both control terminal simultaneously
- We save state, let vim run, then restore

### 5. Context Display

Reads the source file and displays lines around the match:

```c
void get_context_lines(const char *filename, int line_num, ...) {
    FILE *fp = fopen(filename, "r");
    int start = line_num - 5;
    int end = line_num + 5;

    // Read file line by line
    // Collect lines in range [start, end]
}
```

### 6. Interactive Filtering

```c
while (filtering) {
    // Get input character
    // Update filter string
    // Re-filter results in real-time
    // Redraw display
}
```

## Program Structure

```
main()
  ├─ run_grep()          - Execute grep, parse output
  │   └─ popen()         - Run grep subprocess
  ├─ initscr()           - Initialize ncurses
  └─ Main loop:
      ├─ draw_results()  - Display results
      │   └─ get_context_lines() - Read file for context
      ├─ wgetch()        - Get input
      └─ Handle input:
          ├─ Arrow keys  - Navigation
          ├─ Enter       - open_in_vim()
          │   ├─ fork()
          │   ├─ execlp()
          │   └─ waitpid()
          ├─ '/'         - interactive_filter()
          └─ 'q'         - Exit
```

## Learning Points

This program demonstrates:

1. **Process creation and control** - `fork()`, `exec()`, `wait()`
2. **Inter-process communication** - pipes with `popen()`
3. **ncurses TUI development** - windows, input, display
4. **String parsing** - extracting filename:line:content
5. **File I/O** - reading files for context display
6. **Terminal state management** - suspending/resuming ncurses
7. **Interactive filtering** - real-time search refinement

## Limitations and Improvements

**Current limitations:**
- Maximum 10,000 results
- Simple string filtering (not regex)
- No syntax highlighting in preview
- Can't edit filter after exiting filter mode

**Possible improvements:**
- Add regex filtering
- Show match highlighting in content
- Add case-insensitive search option
- Support different grep options (--include, --exclude)
- Add bookmarks for results
- Multi-file opening
- Export results to file
- Syntax highlighting in context preview

## Example Session

```bash
$ ./greptui "socket" tcp-client-server/

# TUI appears showing all matches
# Press ↓ to navigate to a specific match
# Press 'c' to see context around the match
# Press Enter to open in vim at that line
# Edit in vim, save, exit
# Back in TUI
# Press '/' to filter
# Type "bind" to see only results with "bind"
# Press Enter to exit filter
# Press 'q' to quit
```

## Comparison with Similar Tools

- **grep + less**: Can't open in editor easily
- **vim + grep**: Less interactive browsing
- **IDEs**: This is lightweight, terminal-based
- **fzf + vim**: Similar concept but this shows grep results directly
- **ripgrep**: Faster search but no interactive browsing

This tool combines the best of both worlds: grep's search power + interactive navigation + vim integration!
