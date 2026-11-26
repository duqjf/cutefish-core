#include <chrono>
#include <filesystem>
#include <string>

#include <gtest/gtest.h>
#include <unistd.h>

#include "appearance/gtksettings.h"
#include "base/file.h"
#include "base/url.h"

namespace fs = std::filesystem;
using cutefish::base::file::read;
using cutefish::base::file::writeAtomically;

namespace {

const std::string kCssDir = CUTEFISH_GTK_CSS_DIR;
const std::string kImport = "@import url(\"cutefish.css\");";

class GtkCssTest : public testing::Test
{
protected:
    void SetUp() override
    {
        m_dir = fs::temp_directory_path() / ("gtk-css-test-" + std::to_string(::getpid()));
        fs::remove_all(m_dir);
        fs::create_directories(m_dir);
    }
    void TearDown() override { fs::remove_all(m_dir); }

    std::string config() const { return m_dir.string(); }
    std::string file(const std::string &gtkDir, const std::string &name) const
    {
        return read(config() + '/' + gtkDir + '/' + name).value_or("<missing>");
    }

    fs::path m_dir;
};

TEST_F(GtkCssTest, freshConfigGetsStylesheetAndImport)
{
    ensureGtkCssImports(config(), kCssDir, false);
    for (const std::string gtkDir : {"gtk-3.0", "gtk-4.0"}) {
        EXPECT_EQ(file(gtkDir, "gtk.css"), kImport + "\n");
        const std::string css = file(gtkDir, "cutefish.css");
        EXPECT_NE(css.find(*read(kCssDir + '/' + gtkDir + ".css")), std::string::npos);
        EXPECT_EQ(css.find("Coloured window buttons"), std::string::npos);
    }
}

TEST_F(GtkCssTest, coloredWindowControlsComeAndGo)
{
    ensureGtkCssImports(config(), kCssDir, true);
    for (const std::string gtkDir : {"gtk-3.0", "gtk-4.0"}) {
        const std::string css = file(gtkDir, "cutefish.css");
        EXPECT_NE(css.find(*read(kCssDir + "/colored-window-controls-" + gtkDir + ".css")), std::string::npos);
        // The base stylesheet first, so the coloured rules win ties.
        EXPECT_LT(css.find(*read(kCssDir + '/' + gtkDir + ".css")), css.find("Coloured window buttons"));
    }
    ensureGtkCssImports(config(), kCssDir, false);
    EXPECT_EQ(file("gtk-4.0", "cutefish.css").find("Coloured window buttons"), std::string::npos);
}

TEST_F(GtkCssTest, legacyImportIsReplacedAndUserRulesStay)
{
    const std::string legacy =
            "@import url(\"" + cutefish::base::url::fromLocalFile(kCssDir + "/gtk-4.0.css") + "\");";
    fs::create_directories(m_dir / "gtk-4.0");
    ASSERT_TRUE(writeAtomically(config() + "/gtk-4.0/gtk.css",
                                legacy + "\n@import url(\"mine.css\");\nwindow { color: red; }\n"));
    ensureGtkCssImports(config(), kCssDir, false);
    EXPECT_EQ(file("gtk-4.0", "gtk.css"), kImport + "\n@import url(\"mine.css\");\nwindow { color: red; }\n");
}

TEST_F(GtkCssTest, unchangedFilesAreNotRewritten)
{
    ensureGtkCssImports(config(), kCssDir, true);
    const fs::path css = m_dir / "gtk-3.0" / "cutefish.css";
    const fs::path gtk = m_dir / "gtk-3.0" / "gtk.css";
    const auto old = fs::file_time_type::clock::now() - std::chrono::hours(1);
    fs::last_write_time(css, old);
    fs::last_write_time(gtk, old);
    ensureGtkCssImports(config(), kCssDir, true);
    EXPECT_EQ(fs::last_write_time(css), old);
    EXPECT_EQ(fs::last_write_time(gtk), old);
}

TEST_F(GtkCssTest, symlinkedGtkCssStaysALink)
{
    fs::create_directories(m_dir / "dotfiles");
    fs::create_directories(m_dir / "gtk-4.0");
    ASSERT_TRUE(writeAtomically((m_dir / "dotfiles" / "gtk.css").string(), "window { color: red; }\n"));
    fs::create_symlink(m_dir / "dotfiles" / "gtk.css", m_dir / "gtk-4.0" / "gtk.css");
    ensureGtkCssImports(config(), kCssDir, false);
    EXPECT_TRUE(fs::is_symlink(m_dir / "gtk-4.0" / "gtk.css"));
    EXPECT_EQ(*read((m_dir / "dotfiles" / "gtk.css").string()), kImport + "\nwindow { color: red; }\n");
}

} // namespace
