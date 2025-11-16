#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NUM_STARS 100
#define TWINKLE_PROBABILITY 0.1  // 10% chance per frame

typedef struct {
    float x;
    float y;
    float speed;
    int brightness;  // 0-3 (dim to bright)
} Star;

// Star characters by brightness level
const char STAR_CHARS[] = {'.', '*', '+', '@'};

void init_star(Star *star, int max_y, int max_x) {
    star->x = rand() % max_x;
    star->y = rand() % max_y;
    star->speed = 0.1 + (rand() % 10) / 10.0;  // 0.1 to 1.0
    star->brightness = rand() % 4;
}

void update_star(Star *star, int max_y, int max_x) {
    // Move star down
    star->y += star->speed;

    // Wrap around when off screen
    if (star->y >= max_y) {
        star->y = 0;
        star->x = rand() % max_x;
        star->speed = 0.1 + (rand() % 10) / 10.0;
    }

    // Twinkle - randomly change brightness
    if ((rand() % 100) < (TWINKLE_PROBABILITY * 100)) {
        star->brightness = rand() % 4;
    }
}

void draw_star(Star *star) {
    int attr = 0;

    // Different colors for different brightness
    if (has_colors()) {
        switch (star->brightness) {
            case 0:
                attr = COLOR_PAIR(1);  // Dim
                break;
            case 1:
                attr = COLOR_PAIR(2);  // Medium
                break;
            case 2:
                attr = COLOR_PAIR(3);  // Bright
                break;
            case 3:
                attr = COLOR_PAIR(4) | A_BOLD;  // Very bright
                break;
        }
    } else {
        // If no color support, use bold for brighter stars
        if (star->brightness >= 2) {
            attr = A_BOLD;
        }
    }

    attron(attr);
    mvaddch((int)star->y, (int)star->x, STAR_CHARS[star->brightness]);
    attroff(attr);
}

int main() {
    // Initialize ncurses
    initscr();
    cbreak();              // Disable line buffering
    noecho();              // Don't echo keypresses
    curs_set(0);           // Hide cursor
    nodelay(stdscr, TRUE); // Non-blocking getch()

    // Initialize colors if supported
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_BLUE, COLOR_BLACK);     // Dim
        init_pair(2, COLOR_WHITE, COLOR_BLACK);    // Medium
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);   // Bright
        init_pair(4, COLOR_CYAN, COLOR_BLACK);     // Very bright
    }

    // Seed random number generator
    srand(time(NULL));

    // Get screen dimensions
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Initialize stars
    Star stars[NUM_STARS];
    for (int i = 0; i < NUM_STARS; i++) {
        init_star(&stars[i], max_y, max_x);
    }

    // Display instructions
    mvprintw(0, 0, "Star Field Visualization - Press 'q' to quit");
    refresh();
    sleep(2);

    // Main animation loop
    int ch;
    while ((ch = getch()) != 'q') {
        // Clear screen
        clear();

        // Update screen dimensions (in case window resized)
        getmaxyx(stdscr, max_y, max_x);

        // Update and draw all stars
        for (int i = 0; i < NUM_STARS; i++) {
            update_star(&stars[i], max_y, max_x);
            draw_star(&stars[i]);
        }

        // Display quit instruction
        attron(A_DIM);
        mvprintw(max_y - 1, 0, "Press 'q' to quit");
        attroff(A_DIM);

        // Refresh screen
        refresh();

        // Control frame rate (~30 fps)
        usleep(33000);
    }

    // Cleanup
    endwin();

    return 0;
}
