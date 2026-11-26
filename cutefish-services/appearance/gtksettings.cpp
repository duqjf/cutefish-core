#include "gtksettings.h"

#include <filesystem>

#include "base/file.h"
#include "base/paths.h"
#include "base/settings.h"
#include "base/strings.h"
#include "base/url.h"
#include "common/gsettings_keyfile.h"
#include "log.h"

using namespace cutefish::base;
using namespace cutefish::base::strings;
namespace gsettings = cutefish::core::gsettings;
namespace fs = std::filesystem;

static std::string gtk3SettingsIniPath()
{
    return paths::configHome() + "/gtk-3.0/settings.ini";
}

void writeGtkSettings(const GtkSettings &gtk)
{
    const std::string fontName = gtk.fontFamily + ' ' + number(gtk.fontPointSize, 'g', -1);
    // No Cutefish GTK theme ships; Adwaita's dark variant is what gtk-theme can switch to.
    const std::string gtkTheme = gtk.darkMode ? "Adwaita-dark" : "Adwaita";
    const bool subpixel = gtk.fontAntialias && gtk.fontRgba != "none";

    // Only GTK clients on Xwayland still read this, for every key the compositor's XSETTINGS
    // leaves out. On Wayland the keyfile below overrides every key the schema covers.
    {
        Settings settings(gtk3SettingsIniPath());
        for (const std::string &key : settings.childKeys())
            settings.remove(key);
        for (const std::string &group : settings.childGroups())
            settings.remove(group);
        settings.setValue("Settings/gtk-font-name", fontName);
        settings.setValue("Settings/gtk-xft-antialias", gtk.fontAntialias ? 1 : 0);
        settings.setValue("Settings/gtk-xft-hinting", gtk.fontHintStyle != "hintnone" ? 1 : 0);
        settings.setValue("Settings/gtk-xft-hintstyle", gtk.fontHintStyle);
        settings.setValue("Settings/gtk-xft-rgba", subpixel ? gtk.fontRgba : std::string("none"));
        settings.setValue("Settings/gtk-application-prefer-dark-theme", gtk.darkMode);
        settings.setValue("Settings/gtk-icon-theme-name", gtk.iconTheme);
        settings.setValue("Settings/gtk-cursor-theme-name", gtk.cursorTheme);
        settings.setValue("Settings/gtk-cursor-theme-size", gtk.cursorSize);
        settings.setValue("Settings/gtk-enable-animations", gtk.animations);
        settings.setValue("Settings/gtk-theme-name", gtkTheme);
        settings.setValue("Settings/gtk-decoration-layout", "close,minimize,maximize:");
        settings.sync();
    }

    // GTK on Wayland ignores settings.ini for keys the schema covers and picks up keyfile
    // changes through GLib's file monitor.
    gsettings::writeGroup("org/gnome/desktop/interface", {
            {"font-name", fontName},
            {"font-antialiasing",
             std::string(!gtk.fontAntialias ? "none" : subpixel ? "rgba" : "grayscale")},
            // The schema's nicks are fontconfig's hintstyle constants without "hint".
            {"font-hinting", gtk.fontHintStyle.size() > 4 ? gtk.fontHintStyle.substr(4) : std::string()},
            {"font-rgba-order", subpixel ? gtk.fontRgba : std::string("rgb")},
            // "automatic" makes GTK 4 ignore the three keys above.
            {"font-rendering", std::string("manual")},
            {"gtk-theme", gtkTheme},
            {"color-scheme", std::string(gtk.darkMode ? "prefer-dark" : "default")},
            {"icon-theme", gtk.iconTheme},
            {"cursor-theme", gtk.cursorTheme},
            {"cursor-size", gtk.cursorSize},
            {"enable-animations", gtk.animations},
    });
    gsettings::writeGroup("org/gnome/desktop/wm/preferences", {
            {"button-layout", std::string("close,minimize,maximize:")},
    });
}

// The file a symlinked path stands for (dotfile managers), so writing keeps the link.
static std::string throughSymlink(const std::string &path)
{
    std::error_code error;
    if (fs::is_symlink(path, error) && fs::exists(path, error)) {
        const fs::path target = fs::canonical(path, error);
        if (!error)
            return target.string();
    }
    return path;
}

static void writeIfChanged(const std::string &path, const std::string &contents)
{
    if (const auto current = file::read(path); current && *current == contents)
        return;
    std::error_code error;
    fs::create_directories(fs::path(path).parent_path(), error);
    if (!file::writeAtomically(path, contents))
        CF_WARNING(services::log()) << "Cannot write" << path;
}

static void ensureGtkCss(const std::string &configDir, const std::string &cssDir, const std::string &gtkDir,
                         bool coloredWindowControls)
{
    // A copy in the config directory instead of an import from the system data directory:
    // sandboxed GTK apps (Flatpak) given the config directory see it, the data one they don't.
    std::string css = "/* Written by cutefish-services from " + cssDir + "; changes here are lost. */\n";
    std::vector<std::string> parts {gtkDir + ".css"};
    if (coloredWindowControls)
        parts.push_back("colored-window-controls-" + gtkDir + ".css");
    for (const std::string &part : parts) {
        const auto contents = file::read(cssDir + '/' + part);
        if (!contents) {
            CF_WARNING(services::log()) << "Cannot read" << cssDir + '/' + part;
            continue;
        }
        css += *contents;
        if (!css.empty() && css.back() != '\n')
            css += '\n';
    }
    writeIfChanged(throughSymlink(configDir + '/' + gtkDir + "/cutefish.css"), css);

    // GTK resolves the import next to gtk.css. Earlier versions imported the system file.
    const std::string importLine = "@import url(\"cutefish.css\");";
    const std::string legacyImport =
            "@import url(\"" + url::fromLocalFile(cssDir + '/' + gtkDir + ".css") + "\");";

    const std::string path = throughSymlink(configDir + '/' + gtkDir + "/gtk.css");
    std::vector<std::string> lines;
    if (const auto contents = file::read(path))
        lines = split(*contents, '\n');
    while (!lines.empty() && lines.back().empty())
        lines.pop_back();

    // GTK drops an @import that follows any rule, so ours has to lead the file.
    std::erase(lines, legacyImport);
    std::erase(lines, importLine);
    lines.insert(lines.begin(), importLine);
    writeIfChanged(path, join(lines, "\n") + "\n");
}

void ensureGtkCssImports(const std::string &configDir, const std::string &cssDir, bool coloredWindowControls)
{
    ensureGtkCss(configDir, cssDir, "gtk-3.0", coloredWindowControls);
    ensureGtkCss(configDir, cssDir, "gtk-4.0", coloredWindowControls);
}

void ensureGtkCssImports(bool coloredWindowControls)
{
    ensureGtkCssImports(paths::configHome(), CUTEFISH_GTK_CSS_DIR, coloredWindowControls);
}
