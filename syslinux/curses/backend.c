// Backend to SYSLINUX C API.

#include <curspriv.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef CHTYPE_LONG
#define A(x) ((chtype)x | A_ALTCHARSET)
chtype asc_map[128] =
{
    A(0),
    A(1),
    A(2),
    A(3),
    A(4),
    A(5),
    A(6),
    A(7),
    A(8),
    A(9),
    A(10),
    A(11),
    A(12),
    A(13),
    A(14),
    A(15),
    A(16),
    A(17),
    A(18),
    A(19),
    A(20),
    A(21),
    A(22),
    A(23),
    A(24),
    A(25),
    A(26),
    A(27),
    A(28),
    A(29),
    A(30),
    A(31),
    32,
    33,
    34,
    35,
    36,
    37,
    38,
    39,
    40,
    41,
    42,
    A(26),
    A(27),
    A(24),
    A(25),
    47,
    219,
    49,
    50,
    51,
    52,
    53,
    54,
    55,
    56,
    57,
    58,
    59,
    60,
    61,
    62,
    63,
    64,
    65,
    66,
    67,
    68,
    69,
    70,
    71,
    72,
    73,
    74,
    75,
    76,
    77,
    78,
    79,
    80,
    81,
    82,
    83,
    84,
    85,
    86,
    87,
    88,
    89,
    90,
    91,
    92,
    93,
    94,
    95,
    A(4),
    177,
    98,
    99,
    100,
    101,
    248,
    241,
    176,
    A(15),
    217,
    191,
    218,
    192,
    197,
    45,
    45,
    196,
    45,
    45,
    195,
    180,
    193,
    194,
    179,
    243,
    242,
    227,
    216,
    156,
    249,
    A(127)
};
#endif

static struct
{
    char *data;
    long size;
} clip;

// TODO: implement syslinux backend for this function
void PDC_gotoyx(int y, int x)
{
}

// TODO: implement syslinux backend for this function
void PDC_transform_line(int lineno, int x, int len, const chtype *srcp)
{
}

// TODO: implement syslinux backend for this function
int PDC_get_columns(void)
{
}

// TODO: implement syslinux backend for this function
int PDC_get_cursor_mode(void)
{
}

// TODO: implement syslinux backend for this function
int PDC_get_rows(void)
{
}

// TODO: implement syslinux backend for this function
bool PDC_check_key(void)
{
}

// TODO: implement syslinux backend for this function
void PDC_flushinp(void)
{
}

// TODO: implement syslinux backend for this function
int PDC_get_key(void)
{
}

// TODO: implement syslinux backend for this function
int PDC_modifiers_set(void)
{
}

// TODO: implement syslinux backend for this function
int PDC_mouse_set(void)
{
}

// TODO: implement syslinux backend for this function
void PDC_set_keyboard_binary(bool on)
{
}

// TODO: implement syslinux backend for this function
bool PDC_can_change_color(void)
{
}

// TODO: implement syslinux backend for this function
int PDC_color_content(short color, short *red, short *green, short *blue)
{
}

// TODO: implement syslinux backend for this function
int PDC_init_color(short color, short red, short green, short blue)
{
}

// TODO: implement syslinux backend for this function
void PDC_init_pair(short pair, short fg, short bg)
{
}

// TODO: implement syslinux backend for this function
int PDC_pair_content(short pair, short *fg, short *bg)
{
}

// TODO: implement syslinux backend for this function
void PDC_reset_prog_mode(void)
{
}

// TODO: implement syslinux backend for this function
void PDC_reset_shell_mode(void)
{
}

// TODO: implement syslinux backend for this function
int PDC_resize_screen(int nlines, int ncols)
{
}

// TODO: implement syslinux backend for this function
void PDC_restore_screen_mode(int i)
{
}

// TODO: implement syslinux backend for this function
void PDC_save_screen_mode(int i)
{
}

// TODO: implement syslinux backend for this function
void PDC_scr_close(void)
{
}

// TODO: implement syslinux backend for this function
void PDC_scr_free(void)
{
}

// TODO: implement syslinux backend for this function
int PDC_scr_open(int argc, char **argv)
{
}

// TODO: implement syslinux backend for this function
int PDC_curs_set(int visibility)
{
}

// TODO: implement syslinux backend for this function
void PDC_beep(void)
{
}

// TODO: implement syslinux backend for this function
void PDC_napms(int ms)
{
}

int PDC_clearclipboard(void)
{
    if(clip.data != 0)
    {
        free(clip.data);
        memset(&clip,0,sizeof(clip));
    }

    return PDC_CLIP_SUCCESS;
}

int PDC_freeclipboard(char *contents)
{
    if(contents != 0)
        free(contents);

    return PDC_CLIP_SUCCESS;
}

int PDC_getclipboard(char **contents,long *length)
{
    if(contents == 0 || length == 0)
        return PDC_CLIP_ACCESS_ERROR;

    if(clip.data == 0 || clip.size <= 0)
        return PDC_CLIP_EMPTY;

    if((*contents = malloc(clip.size)) == 0)
        return PDC_CLIP_MEMORY_ERROR;

    strlcpy(*contents,clip.data,clip.size);

    *length = clip.size - 1;

    return PDC_CLIP_SUCCESS;
}

int PDC_setclipboard(const char *contents,long length)
{
    if(contents == 0 || length <= 0)
        return PDC_CLIP_ACCESS_ERROR;

    PDC_clearclipboard();

    if((clip.data = malloc(length + 1)) == 0)
       return PDC_CLIP_MEMORY_ERROR;

    strlcpy(clip.data,contents,length + 1);

    clip.size = strlen(clip.data) + 1;

    return PDC_CLIP_SUCCESS;
}

// TODO: implement syslinux backend for this function
unsigned long PDC_get_input_fd(void)
{
}

// TODO: implement syslinux backend for this function
int PDC_set_blink(bool blinkon)
{
}

// TODO: implement syslinux backend for this function
void PDC_set_title(const char *title)
{
}
