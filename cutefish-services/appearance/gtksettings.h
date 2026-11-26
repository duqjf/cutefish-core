#ifndef GTKSETTINGS_H
#define GTKSETTINGS_H

#include <string>

struct GtkSettings
{
    std::string fontFamily;
    double fontPointSize = 9;
    // fontconfig's values, as written by writeFontConfig().
    bool fontAntialias = true;
    std::string fontHintStyle = "hintfull";
    std::string fontRgba = "rgb";
    bool darkMode = false;
    std::string iconTheme;
    std::string cursorTheme;
    int cursorSize = 24;
    bool animations = true;
};

// Mirrors the system appearance into GTK's config: settings.ini for Xwayland clients and the
// GSettings keyfile for Wayland clients.
void writeGtkSettings(const GtkSettings &settings);

// Writes the Cutefish stylesheets for GTK 3 and 4 into the user's config as cutefish.css, with
// the coloured window buttons when asked, and puts its import at the top of gtk.css, keeping the
// rest of the file. The import is re-added at every login, even after the user removed it. GTK
// reads these files when an app starts, so running apps keep what they had.
void ensureGtkCssImports(bool coloredWindowControls);
// The same, from `cssDir` into `configDir`.
void ensureGtkCssImports(const std::string &configDir, const std::string &cssDir, bool coloredWindowControls);

#endif
