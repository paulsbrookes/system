# Star Field Visualization

An animated star field using ncurses, simulating flying through space with twinkling stars.

## What It Does

This program creates a terminal-based star field visualization where:
- **100 stars** are randomly placed across the screen
- Stars **move downward** at different speeds (simulating upward flight)
- Stars **twinkle** by randomly changing brightness
- Different **characters** represent different brightness levels: `.` (dim) → `*` → `+` → `@` (bright)
- Different **colors** are used if your terminal supports them

## How to Use

### Compile
```bash
gcc -Wall -O2 starfield.c -o starfield -lncurses
```

The `-lncurses` flag links the ncurses library.

### Run
```bash
./starfield
```

Press `q` to quit the visualization.

### Requirements
- ncurses library installed: `sudo apt install libncurses-dev` (on Debian/Ubuntu)
- Terminal with color support (optional, but recommended)

## How It Works

### Program Structure

1. **Star struct** - Each star has:
   - `x, y` position (floating point for smooth movement)
   - `speed` (how fast it moves down)
   - `brightness` level (0-3)

2. **Initialization**:
   - Creates 100 stars at random positions
   - Sets random speeds and brightness levels

3. **Animation loop**:
   - Updates star positions (move down)
   - Randomly changes brightness (twinkling effect)
   - Wraps stars to top when they go off bottom
   - Draws all stars
   - Sleeps to maintain ~30 FPS

### Key ncurses Functions Used

**Initialization:**
- **`initscr()`** - Initialize ncurses mode
- **`cbreak()`** - Disable line buffering (get input immediately)
- **`noecho()`** - Don't display typed characters
- **`curs_set(0)`** - Hide the cursor
- **`nodelay(stdscr, TRUE)`** - Make `getch()` non-blocking
- **`start_color()`** - Enable color support
- **`init_pair()`** - Define color pairs (foreground/background)

**Drawing:**
- **`clear()`** - Clear the screen
- **`mvaddch(y, x, ch)`** - Move cursor to (y, x) and draw character
- **`mvprintw(y, x, fmt, ...)`** - Move cursor and print formatted text
- **`attron(attr)` / `attroff(attr)`** - Turn attributes on/off (color, bold, dim)
- **`refresh()`** - Update the actual screen with buffered changes

**Screen info:**
- **`getmaxyx(win, y, x)`** - Get screen dimensions
- **`has_colors()`** - Check if terminal supports colors

**Cleanup:**
- **`endwin()`** - Restore terminal to normal mode

### Animation Technique

The program uses a simple game loop:
```c
while (user hasn't pressed 'q') {
    1. Clear screen
    2. Update all stars (move, twinkle)
    3. Draw all stars
    4. Refresh screen
    5. Sleep to control frame rate (~30 FPS)
}
```

This creates smooth animation by redrawing the entire screen each frame.

### Twinkling Effect

Stars have a 10% chance each frame to randomly change brightness:
```c
if ((rand() % 100) < 10) {
    star->brightness = rand() % 4;
}
```

### Color Scheme

- **Blue** - Dim stars (brightness 0)
- **White** - Medium stars (brightness 1)
- **Yellow** - Bright stars (brightness 2)
- **Cyan + Bold** - Very bright stars (brightness 3)

If your terminal doesn't support colors, brighter stars will be shown in bold.

## Customization Ideas

Try modifying these constants in the code:

```c
#define NUM_STARS 100              // Number of stars
#define TWINKLE_PROBABILITY 0.1    // How often stars twinkle
```

Or experiment with:
- Star movement direction (change which coordinate you update)
- Different speed ranges
- More or fewer brightness levels
- Different characters or colors
- Add horizontal movement for diagonal motion

## Learning Points

This program demonstrates:
1. **ncurses basics** - Screen control, colors, attributes
2. **Animation loops** - Frame-based updates with timing
3. **Structs** - Organizing related data
4. **Random numbers** - Creating variation and unpredictability
5. **Floating-point positions** - Smooth sub-pixel movement
6. **Non-blocking input** - Checking for keypresses without stopping animation

## Next Steps

To extend this program, you could:
- Add keyboard controls to change speed or number of stars
- Implement different star field patterns (spiral, explosion, etc.)
- Add "comets" that leave trails
- Create parallax layers (foreground/background stars at different speeds)
- Save/load different star field configurations
