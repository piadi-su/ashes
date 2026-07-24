/* CONFIG FILE 
 * 
 * Every time you change this file
 * you have to recompile ash
 *
 * */

//number of monitors u have
#define MAX_MONITORS 2

// Bar height in pixel
#define BAR_HEIGHT 24

#define WORKSPACES_PER_MONITOR 10


// Bar background color
#define BACKGROUND_COLOR 0x1d2021
// #define BACKGROUND_COLOR 0xebdbb2



//Bar text color always put 0x and than the rgb code
#define TEXT_COLOR 0xffffff
// #define TEXT_COLOR 0xebdbb2 
// #define TEXT_COLOR 0x8f00ff
// #define TEXT_COLOR 0x6016a2


//Workspace tags
static const char *tags[] __attribute__((unused)) = {
    "1:web",
    "2:code",
    "3:term",
    "4:chat",
    "5:media",
    "6:files",
    "7:misc",
    "8:extra",
    "9:sys",
    "10:dp"
};

// 0 = LEFT, 1 = CENTER, 2 = RIGHT

#define WORKSPACES_POS  0  
#define DATETIME_POS    1  
#define SYSINFO_POS     2  

//Floaitng bar
#define BAR_MARGIN_X 12  // right and left margin of the screen
#define BAR_MARGIN_Y 8   // up/down distance for screen

//bar space define also the space between of the modules
#define BAR_SPACER " | "



//left bracket
#define ACTIVE_WS_L_BRACKET "("
//right bracket
#define ACTIVE_WS_R_BRACKET ")"



// Bottom = 0 -> top | Bottom = 1 -> bottom
#define BOTTOM 0



// font that the bar in going to try to use
#define BAR_FONT "Iosevka Nerd Font:size=12:antialias=true"
// #define BAR_FONT "JetBrains Mono Nerd Font:size=13:antialias=true"

