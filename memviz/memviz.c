#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>

#define MAX_MAPS 1000
#define MAX_LINE 512

// Memory mapping entry
typedef struct {
    unsigned long start;
    unsigned long end;
    char perms[5];
    char pathname[256];
    unsigned long size_kb;
} MemMap;

// Process memory statistics
typedef struct {
    unsigned long vm_size;     // Virtual memory size (KB)
    unsigned long vm_rss;      // Resident set size (KB)
    unsigned long vm_data;     // Data segment size (KB)
    unsigned long vm_stk;      // Stack size (KB)
    unsigned long vm_exe;      // Code size (KB)
    unsigned long vm_lib;      // Library size (KB)
} MemStats;

// Memory segment summary
typedef struct {
    unsigned long heap_kb;
    unsigned long stack_kb;
    unsigned long code_kb;
    unsigned long libs_kb;
    unsigned long anon_kb;
    unsigned long other_kb;
} MemSegments;

// Global data
MemStats stats = {0};
MemMap maps[MAX_MAPS];
int map_count = 0;
MemSegments segments = {0};
unsigned long total_system_mem_kb = 0;
int current_pid = 0;

// View levels
enum ViewLevel { VIEW_OVERVIEW, VIEW_SEGMENTS, VIEW_DETAILED };
int current_view = VIEW_OVERVIEW;
int scroll_offset = 0;

// Read process memory statistics from /proc/[pid]/status
int read_mem_stats(int pid) {
    char path[256];
    FILE *fp;
    char line[256];

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line + 7, "%lu", &stats.vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%lu", &stats.vm_rss);
        } else if (strncmp(line, "VmData:", 7) == 0) {
            sscanf(line + 7, "%lu", &stats.vm_data);
        } else if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line + 6, "%lu", &stats.vm_stk);
        } else if (strncmp(line, "VmExe:", 6) == 0) {
            sscanf(line + 6, "%lu", &stats.vm_exe);
        } else if (strncmp(line, "VmLib:", 6) == 0) {
            sscanf(line + 6, "%lu", &stats.vm_lib);
        }
    }

    fclose(fp);
    return 0;
}

// Read detailed memory maps from /proc/[pid]/maps
int read_mem_maps(int pid) {
    char path[256];
    FILE *fp;
    char line[MAX_LINE];

    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    map_count = 0;
    while (fgets(line, sizeof(line), fp) && map_count < MAX_MAPS) {
        unsigned long start, end;
        char perms[5];
        char pathname[256] = "";

        sscanf(line, "%lx-%lx %4s %*s %*s %*s %255[^\n]",
               &start, &end, perms, pathname);

        maps[map_count].start = start;
        maps[map_count].end = end;
        maps[map_count].size_kb = (end - start) / 1024;
        strncpy(maps[map_count].perms, perms, 4);
        maps[map_count].perms[4] = '\0';

        // Clean up pathname
        char *p = pathname;
        while (*p == ' ') p++;
        strncpy(maps[map_count].pathname, p, 255);
        maps[map_count].pathname[255] = '\0';

        // Categorize for segment view
        if (strstr(pathname, "[heap]")) {
            segments.heap_kb += maps[map_count].size_kb;
        } else if (strstr(pathname, "[stack]")) {
            segments.stack_kb += maps[map_count].size_kb;
        } else if (strlen(p) > 0 && p[0] == '/') {
            if (strstr(pathname, ".so")) {
                segments.libs_kb += maps[map_count].size_kb;
            } else {
                segments.code_kb += maps[map_count].size_kb;
            }
        } else if (strlen(p) == 0) {
            segments.anon_kb += maps[map_count].size_kb;
        } else {
            segments.other_kb += maps[map_count].size_kb;
        }

        map_count++;
    }

    fclose(fp);
    return 0;
}

// Read total system memory
int read_system_mem() {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return -1;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line + 9, "%lu", &total_system_mem_kb);
            break;
        }
    }

    fclose(fp);
    return 0;
}

// Draw a horizontal bar graph
void draw_bar(int y, int x, const char *label, unsigned long value,
              unsigned long max_value, int bar_width) {
    mvprintw(y, x, "%s", label);
    int filled = (max_value > 0) ? (value * bar_width / max_value) : 0;
    if (filled > bar_width) filled = bar_width;

    attron(COLOR_PAIR(1));
    mvprintw(y, x + 25, "[");
    for (int i = 0; i < filled; i++) {
        addch('=');
    }
    attroff(COLOR_PAIR(1));

    for (int i = filled; i < bar_width; i++) {
        addch('-');
    }
    addch(']');

    // Show value and percentage
    float pct = (max_value > 0) ? (100.0 * value / max_value) : 0;
    mvprintw(y, x + 25 + bar_width + 3, "%8lu KB (%5.1f%%)", value, pct);
}

// View 1: Overview
void draw_overview() {
    int row = 2;

    attron(A_BOLD | COLOR_PAIR(2));
    mvprintw(row++, 2, "MEMORY OVERVIEW - Process %d", current_pid);
    attroff(A_BOLD | COLOR_PAIR(2));
    row++;

    mvprintw(row++, 2, "System Total Memory: %lu MB", total_system_mem_kb / 1024);
    row++;

    // Virtual memory
    attron(A_BOLD);
    mvprintw(row++, 2, "Virtual Memory (VmSize):");
    attroff(A_BOLD);
    draw_bar(row++, 2, "  Total Virtual", stats.vm_size, total_system_mem_kb, 40);
    row++;

    // Physical memory (RSS)
    attron(A_BOLD);
    mvprintw(row++, 2, "Physical Memory (RSS):");
    attroff(A_BOLD);
    draw_bar(row++, 2, "  Resident Set Size", stats.vm_rss, total_system_mem_kb, 40);
    row++;

    // Breakdown from /proc/[pid]/status
    attron(A_BOLD);
    mvprintw(row++, 2, "Memory Breakdown:");
    attroff(A_BOLD);
    draw_bar(row++, 2, "  Data Segment", stats.vm_data, stats.vm_size, 40);
    draw_bar(row++, 2, "  Stack", stats.vm_stk, stats.vm_size, 40);
    draw_bar(row++, 2, "  Code (Executable)", stats.vm_exe, stats.vm_size, 40);
    draw_bar(row++, 2, "  Libraries", stats.vm_lib, stats.vm_size, 40);

    row += 2;
    attron(COLOR_PAIR(3));
    mvprintw(row++, 2, "Press '2' for Segment View | '3' for Detailed Maps | 'q' to quit");
    attroff(COLOR_PAIR(3));
}

// View 2: Memory Segments
void draw_segments() {
    int row = 2;

    attron(A_BOLD | COLOR_PAIR(2));
    mvprintw(row++, 2, "MEMORY SEGMENTS - Process %d", current_pid);
    attroff(A_BOLD | COLOR_PAIR(2));
    row++;

    mvprintw(row++, 2, "Total Virtual Memory: %lu MB (%d mappings)",
             stats.vm_size / 1024, map_count);
    row++;

    attron(A_BOLD);
    mvprintw(row++, 2, "Segment Breakdown:");
    attroff(A_BOLD);

    draw_bar(row++, 2, "  Heap", segments.heap_kb, stats.vm_size, 40);
    draw_bar(row++, 2, "  Stack", segments.stack_kb, stats.vm_size, 40);
    draw_bar(row++, 2, "  Code/Executables", segments.code_kb, stats.vm_size, 40);
    draw_bar(row++, 2, "  Shared Libraries", segments.libs_kb, stats.vm_size, 40);
    draw_bar(row++, 2, "  Anonymous", segments.anon_kb, stats.vm_size, 40);
    draw_bar(row++, 2, "  Other", segments.other_kb, stats.vm_size, 40);

    row += 2;
    attron(COLOR_PAIR(3));
    mvprintw(row++, 2, "Press '1' for Overview | '3' for Detailed Maps | 'q' to quit");
    attroff(COLOR_PAIR(3));
}

// View 3: Detailed Memory Maps
void draw_detailed() {
    int row = 2;
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    attron(A_BOLD | COLOR_PAIR(2));
    mvprintw(row++, 2, "DETAILED MEMORY MAPS - Process %d", current_pid);
    attroff(A_BOLD | COLOR_PAIR(2));
    row++;

    mvprintw(row++, 2, "Total Mappings: %d | Scroll: ↑↓ or PgUp/PgDn", map_count);
    row++;

    // Header
    attron(A_BOLD);
    mvprintw(row++, 2, "%-18s %-18s %8s %5s  %s",
             "START", "END", "SIZE", "PERM", "PATHNAME");
    attroff(A_BOLD);

    // Display maps with scrolling
    int visible_lines = max_y - row - 2;
    int start_idx = scroll_offset;
    int end_idx = start_idx + visible_lines;
    if (end_idx > map_count) end_idx = map_count;

    for (int i = start_idx; i < end_idx && row < max_y - 2; i++) {
        char size_str[16];
        if (maps[i].size_kb < 1024) {
            snprintf(size_str, sizeof(size_str), "%lu KB", maps[i].size_kb);
        } else {
            snprintf(size_str, sizeof(size_str), "%lu MB", maps[i].size_kb / 1024);
        }

        // Truncate pathname if too long
        char display_path[100];
        if (strlen(maps[i].pathname) > 60) {
            snprintf(display_path, sizeof(display_path), "...%s",
                     maps[i].pathname + strlen(maps[i].pathname) - 57);
        } else {
            strncpy(display_path, maps[i].pathname, sizeof(display_path) - 1);
        }

        mvprintw(row++, 2, "%016lx - %016lx %8s %4s  %s",
                 maps[i].start, maps[i].end, size_str, maps[i].perms, display_path);
    }

    // Scrollbar indicator
    if (map_count > visible_lines) {
        mvprintw(max_y - 2, 2, "Showing %d-%d of %d",
                 start_idx + 1, end_idx, map_count);
    }

    attron(COLOR_PAIR(3));
    mvprintw(max_y - 1, 2, "Press '1' for Overview | '2' for Segments | 'q' to quit");
    attroff(COLOR_PAIR(3));
}

// Main display function
void display() {
    clear();

    switch (current_view) {
        case VIEW_OVERVIEW:
            draw_overview();
            break;
        case VIEW_SEGMENTS:
            draw_segments();
            break;
        case VIEW_DETAILED:
            draw_detailed();
            break;
    }

    refresh();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        fprintf(stderr, "  Visualize memory allocation for a running process\n");
        return 1;
    }

    current_pid = atoi(argv[1]);
    if (current_pid <= 0) {
        fprintf(stderr, "Error: Invalid PID\n");
        return 1;
    }

    // Read all memory data
    if (read_mem_stats(current_pid) != 0) {
        fprintf(stderr, "Error: Cannot read memory stats for PID %d\n", current_pid);
        fprintf(stderr, "Make sure the process exists and you have permissions.\n");
        return 1;
    }

    if (read_mem_maps(current_pid) != 0) {
        fprintf(stderr, "Error: Cannot read memory maps for PID %d\n", current_pid);
        return 1;
    }

    if (read_system_mem() != 0) {
        fprintf(stderr, "Error: Cannot read system memory info\n");
        return 1;
    }

    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_CYAN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    }

    // Main loop
    int running = 1;
    display();

    while (running) {
        int ch = getch();

        switch (ch) {
            case 'q':
            case 'Q':
                running = 0;
                break;

            case '1':
                current_view = VIEW_OVERVIEW;
                scroll_offset = 0;
                display();
                break;

            case '2':
                current_view = VIEW_SEGMENTS;
                scroll_offset = 0;
                display();
                break;

            case '3':
                current_view = VIEW_DETAILED;
                scroll_offset = 0;
                display();
                break;

            case KEY_UP:
                if (current_view == VIEW_DETAILED && scroll_offset > 0) {
                    scroll_offset--;
                    display();
                }
                break;

            case KEY_DOWN:
                if (current_view == VIEW_DETAILED) {
                    int max_y, max_x;
                    getmaxyx(stdscr, max_y, max_x);
                    int visible_lines = max_y - 7;
                    if (scroll_offset < map_count - visible_lines) {
                        scroll_offset++;
                        display();
                    }
                }
                break;

            case KEY_PPAGE:  // Page Up
                if (current_view == VIEW_DETAILED && scroll_offset > 0) {
                    scroll_offset = (scroll_offset > 10) ? scroll_offset - 10 : 0;
                    display();
                }
                break;

            case KEY_NPAGE:  // Page Down
                if (current_view == VIEW_DETAILED) {
                    int max_y, max_x;
                    getmaxyx(stdscr, max_y, max_x);
                    int visible_lines = max_y - 7;
                    scroll_offset += 10;
                    if (scroll_offset > map_count - visible_lines) {
                        scroll_offset = map_count - visible_lines;
                    }
                    if (scroll_offset < 0) scroll_offset = 0;
                    display();
                }
                break;
        }
    }

    endwin();

    printf("Memory visualization complete for PID %d\n", current_pid);
    printf("Virtual Memory: %lu KB, RSS: %lu KB\n", stats.vm_size, stats.vm_rss);

    return 0;
}
