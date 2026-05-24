#ifndef OLED_H
#define OLED_H

#include <stdint.h>

void Oled_Init(void);
void Oled_ShowLines(const char *line0, const char *line1,
    const char *line2, const char *line3);

#endif
