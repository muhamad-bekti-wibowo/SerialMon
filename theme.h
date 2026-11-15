#pragma once
// Tema mengikuti mbektiw.my.id (krem hangat + oranye, Plus Jakarta Sans + JetBrains Mono), terang & gelap.
#include <QString>

struct Theme {
    bool dark = false;
    QString bg, card, cardAlt, line, orange, orangeHover, orangeSoft, text, muted, dim, ok, bad, warn;
};

Theme makeTheme(bool dark);
QString buildStyleSheet(const Theme& t);
