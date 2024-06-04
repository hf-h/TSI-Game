#ifndef WIN_32_DEFS_H
#define WIN_32_DEFS_H

// Terminal char attributes
#define FOREGROUND_BLUE 0x0001                  //Text color contains blue.
#define FOREGROUND_GREEN 0x0002                 //Text color contains green.
#define FOREGROUND_RED 0x0004                   //Text color contains red.
#define FOREGROUND_INTENSITY 0x0008             //Text color is intensified.
#define BACKGROUND_BLUE 0x0010                  //Background color contains blue.
#define BACKGROUND_GREEN 0x0020                 //Background color contains green.
#define BACKGROUND_RED 0x0040                   //Background color contains red.
#define BACKGROUND_INTENSITY 0x0080             //Background color is intensified.
#define COMMON_LVB_LEADING_BYTE 0x0100          //Leading byte.
#define COMMON_LVB_TRAILING_BYTE 0x0200         //Trailing byte.
#define COMMON_LVB_GRID_HORIZONTAL 0x0400       //Top horizontal.
#define COMMON_LVB_GRID_LVERTICAL 0x0800        //Left vertical.
#define COMMON_LVB_GRID_RVERTICAL 0x1000        //Right vertical.
#define COMMON_LVB_REVERSE_VIDEO 0x4000         //Reverse foreground and background attribute.
#define COMMON_LVB_UNDERSCORE 0x8000            //Underscore.
                                                //
#define FOREGROUND_YELLOW FOREGROUND_RED | FOREGROUND_GREEN

#endif
