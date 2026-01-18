/*
    AnbUI Miniature Text UI Lib for Burger Enjoyers(tm)

    ad_hal: Platform abstraction layer

    Tip of the day: Burgers with burgering on your taste are
    best served with burger burgering beverages. This ensures
    that the taste of burger cheese and burger bacon on burger
    can freely unfold.

    (C) 2024 E. Voirin (oerg866) */

#ifndef _AD_HAL_H_
#define _AD_HAL_H_

#include <stdint.h>

#include "anbui.h"

#if defined(__unix__) || defined(__APPLE__) || defined(_WIN32)
# define AD_HAL_HAS_POPEN
#endif

/* This is a set of functions that a platform implementation needs to implement */

/* Initializes console */
void        ad_initConsole      (ad_ConsoleConfig *cfg);
/* Restores AnbUI console to its expected state after console was used externally */
void        ad_restoreConsole   (void);
/* Deinitializes, e.g. sets screen back to original state */
void        ad_deinitConsole    (void);

/* Set background and foreground color */
void        ad_setColor         (uint8_t bg, uint8_t fg);
/* Set cursor position */
void        ad_setCursorPosition(uint16_t x, uint16_t y);
/* Flush output */
void        ad_flush            (void);

/* Print formatted string (printf-style) */
void        ad_print            (const char *format, ...);
/* Print string */
void        ad_putString        (const char *str);
/* Fill with character*/
void        ad_putChar          (char c, size_t count);

/* Get key. Special keys need to return the codes specified in anbui_priv.h */
uint32_t    ad_getKey           (void);


#endif
