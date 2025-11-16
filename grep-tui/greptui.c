#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_RESULTS 10000
#define MAX_LINE 2048
#define MAX_CONTEXT_LINES 5

typedef struct {
    char filename[512];
    int line_number;
    char content[1024];
} GrepResult;

GrepResult results[MAX_RESULTS];
int result_count = 0;
int current_selection = 0;
int scroll_offset = 0;
int show_context = 0;

void parse_grep_line(const char *line, GrepResult *result) {
    // Parse format: filename:line_number:content
    const char *first_colon = strchr(line, ':');
    if (!first_colon) return;

    const char *second_colon = strchr(first_colon + 1, ':');
    if (!second_colon) return;

    // Extract filename
    int filename_len = first_colon - line;
    strncpy(result->filename, line, filename_len);
    result->filename[filename_len] = '\0';

    // Extract line number
    result->line_number = atoi(first_colon + 1);

    // Extract content
    strncpy(result->content, second_colon + 1, sizeof(result->content) - 1);
    result->content[sizeof(result->content) - 1] = '\0';

    // Remove trailing newline
    char *newline = strchr(result->content, '\n');
    if (newline) *newline = '\0';
}

int run_grep(const char *pattern, const char *directory) {
    char command[1024];
    snprintf(command, sizeof(command), "grep -rn '%s' %s 2>/dev/null",
             pattern, directory);

    FILE *fp = popen(command, "r");
    if (!fp) {
        return -1;
    }

    char line[MAX_LINE];
    result_count = 0;

    while (fgets(line, sizeof(line), fp) && result_count < MAX_RESULTS) {
        parse_grep_line(line, &results[result_count]);
        result_count++;
    }

    pclose(fp);
    return result_count;
}

void get_context_lines(const char *filename, int line_num, char context[][1024], int *count) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        *count = 0;
        return;
    }

    int start_line = (line_num - MAX_CONTEXT_LINES > 0) ?
                     line_num - MAX_CONTEXT_LINES : 1;
    int end_line = line_num + MAX_CONTEXT_LINES;

    char line[1024];
    int current_line = 1;
    *count = 0;

    while (fgets(line, sizeof(line), fp) && current_line <= end_line) {
        if (current_line >= start_line) {
            // Remove newline
            char *newline = strchr(line, '\n');
            if (newline) *newline = '\0';

            snprintf(context[*count], 1024, "%4d: %s", current_line, line);
            (*count)++;
        }
        current_line++;
    }

    fclose(fp);
}

void open_in_vim(const char *filename, int line_number) {
    // Suspend ncurses
    def_prog_mode();
    endwin();

    // Fork and exec vim
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        char line_arg[32];
        snprintf(line_arg, sizeof(line_arg), "+%d", line_number);
        execlp("vim", "vim", line_arg, filename, NULL);
        perror("execlp");
        exit(1);
    } else if (pid > 0) {
        // Parent process - wait for vim to finish
        waitpid(pid, NULL, 0);
    }

    // Resume ncurses
    reset_prog_mode();
    refresh();
}

void draw_results(WINDOW *win, const char *pattern, const char *filter) {
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    werase(win);
    box(win, 0, 0);

    // Title
    wattron(win, A_BOLD);
    mvwprintw(win, 0, 2, " Grep TUI - Pattern: '%s' ", pattern);
    wattroff(win, A_BOLD);

    // Status bar
    if (filter && strlen(filter) > 0) {
        mvwprintw(win, max_y - 1, 2, "Filter: %s", filter);
    } else {
        mvwprintw(win, max_y - 1, 2, "q:Quit | Enter:Open | /:Filter | c:Context");
    }

    // Results count
    mvwprintw(win, 1, 2, "Results: %d", result_count);

    if (result_count == 0) {
        mvwprintw(win, 3, 2, "No results found.");
        wrefresh(win);
        return;
    }

    // Display results
    int display_height = max_y - 4;
    int y = 3;

    for (int i = scroll_offset; i < result_count && y < max_y - 1; i++) {
        int is_selected = (i == current_selection);

        if (is_selected) {
            wattron(win, A_REVERSE);
        }

        // Display filename and line number
        mvwprintw(win, y, 2, "%s:%d",
                  results[i].filename,
                  results[i].line_number);

        // Truncate content to fit screen
        int content_start = strlen(results[i].filename) + 10;
        int content_width = max_x - content_start - 3;

        if (content_width > 0) {
            char truncated[1024];
            strncpy(truncated, results[i].content, content_width);
            truncated[content_width] = '\0';
            mvwprintw(win, y, content_start, "%s", truncated);
        }

        if (is_selected) {
            wattroff(win, A_REVERSE);
        }

        y++;

        // Show context if enabled and this is the selected item
        if (show_context && is_selected) {
            char context[MAX_CONTEXT_LINES * 2 + 1][1024];
            int context_count;
            get_context_lines(results[i].filename,
                            results[i].line_number,
                            context, &context_count);

            wattron(win, COLOR_PAIR(1) | A_DIM);
            for (int j = 0; j < context_count && y < max_y - 1; j++) {
                mvwprintw(win, y, 4, "%s", context[j]);
                y++;
            }
            wattroff(win, COLOR_PAIR(1) | A_DIM);
        }
    }

    wrefresh(win);
}

void interactive_filter(WINDOW *win, const char *pattern) {
    char filter[256] = "";
    int filter_pos = 0;
    int original_count = result_count;
    GrepResult original_results[MAX_RESULTS];

    memcpy(original_results, results, sizeof(results));

    while (1) {
        draw_results(win, pattern, filter);

        int ch = wgetch(win);

        if (ch == '\n' || ch == 27) {  // Enter or Esc
            break;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (filter_pos > 0) {
                filter[--filter_pos] = '\0';
            }
        } else if (ch >= 32 && ch < 127 && filter_pos < 255) {
            filter[filter_pos++] = ch;
            filter[filter_pos] = '\0';
        } else {
            continue;
        }

        // Apply filter
        result_count = 0;
        for (int i = 0; i < original_count; i++) {
            if (strlen(filter) == 0 ||
                strstr(original_results[i].filename, filter) ||
                strstr(original_results[i].content, filter)) {
                results[result_count++] = original_results[i];
            }
        }

        current_selection = 0;
        scroll_offset = 0;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pattern> [directory]\n", argv[0]);
        fprintf(stderr, "Search for pattern in files using grep\n");
        return 1;
    }

    const char *pattern = argv[1];
    const char *directory = (argc > 2) ? argv[2] : ".";

    // Run grep
    printf("Searching for '%s' in '%s'...\n", pattern, directory);
    if (run_grep(pattern, directory) < 0) {
        fprintf(stderr, "Error running grep\n");
        return 1;
    }

    if (result_count == 0) {
        printf("No results found.\n");
        return 0;
    }

    printf("Found %d results. Starting TUI...\n", result_count);
    sleep(1);

    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
    }

    WINDOW *win = newwin(LINES, COLS, 0, 0);
    keypad(win, TRUE);

    int running = 1;
    while (running) {
        int max_y, max_x;
        getmaxyx(win, max_y, max_x);
        int visible_lines = max_y - 4;

        draw_results(win, pattern, NULL);

        int ch = wgetch(win);
        switch (ch) {
            case 'q':
            case 'Q':
                running = 0;
                break;

            case KEY_UP:
                if (current_selection > 0) {
                    current_selection--;
                    if (current_selection < scroll_offset) {
                        scroll_offset = current_selection;
                    }
                }
                break;

            case KEY_DOWN:
                if (current_selection < result_count - 1) {
                    current_selection++;
                    if (current_selection >= scroll_offset + visible_lines) {
                        scroll_offset = current_selection - visible_lines + 1;
                    }
                }
                break;

            case KEY_PPAGE:  // Page Up
                current_selection -= visible_lines;
                if (current_selection < 0) current_selection = 0;
                scroll_offset = current_selection;
                break;

            case KEY_NPAGE:  // Page Down
                current_selection += visible_lines;
                if (current_selection >= result_count) {
                    current_selection = result_count - 1;
                }
                scroll_offset = current_selection - visible_lines + 1;
                if (scroll_offset < 0) scroll_offset = 0;
                break;

            case '\n':  // Enter - open in vim
                if (result_count > 0) {
                    open_in_vim(results[current_selection].filename,
                              results[current_selection].line_number);
                }
                break;

            case '/':  // Filter
                interactive_filter(win, pattern);
                break;

            case 'c':
            case 'C':
                show_context = !show_context;
                break;
        }
    }

    // Cleanup
    delwin(win);
    endwin();

    return 0;
}
