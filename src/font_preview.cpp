#include "font_preview.h"
#include <fontconfig/fontconfig.h>
#include <stdexcept>

std::vector<FontInfo> get_system_fonts(bool filter_latin_only) {
  std::vector<FontInfo> fonts;

  FcConfig *config = FcInitLoadConfigAndFonts();
  if (!config)
    throw std::runtime_error("Failed to initialize fontconfig");

  FcPattern *pat = FcPatternCreate();
  if (!pat) {
    FcConfigDestroy(config);
    throw std::runtime_error("Failed to create pattern");
  }

  FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, nullptr);
  if (!os) {
    FcPatternDestroy(pat);
    FcConfigDestroy(config);
    throw std::runtime_error("Failed to create object set");
  }

  FcFontSet *fs = FcFontList(config, pat, os);
  if (!fs) {
    FcObjectSetDestroy(os);
    FcPatternDestroy(pat);
    FcConfigDestroy(config);
    throw std::runtime_error("Failed to list fonts");
  }

  for (int i = 0; i < fs->nfont; ++i) {
    FcPattern *font = fs->fonts[i];
    FcChar8 *family_raw, *style_raw, *file_raw;

    if (FcPatternGetString(font, FC_FAMILY, 0, &family_raw) == FcResultMatch &&
        FcPatternGetString(font, FC_STYLE, 0, &style_raw) == FcResultMatch &&
        FcPatternGetString(font, FC_FILE, 0, &file_raw) == FcResultMatch) {

      std::string family = reinterpret_cast<char *>(family_raw);
      std::string style = reinterpret_cast<char *>(style_raw);
      std::string file = reinterpret_cast<char *>(file_raw);

      if (filter_latin_only) {
        // Check for basic Latin support (e.g., 'A')
        FcPattern *test_pat = FcPatternDuplicate(font);
        FcPatternAddString(test_pat, FC_CHARSET, (FcChar8 *)"A");
        FcResult result;
        FcPattern *match = FcFontMatch(config, test_pat, &result);
        FcCharSet *charset = nullptr;
        bool has_latin = false;
        if (FcPatternGetCharSet(match, FC_CHARSET, 0, &charset) ==
            FcResultMatch) {
          has_latin = FcCharSetHasChar(charset, 'A');
        }
        FcPatternDestroy(test_pat);
        FcPatternDestroy(match);
        if (!has_latin)
          continue;
      }

      fonts.emplace_back(FontInfo{family, style, file});
    }
  }

  FcFontSetDestroy(fs);
  FcObjectSetDestroy(os);
  FcPatternDestroy(pat);
  FcConfigDestroy(config);

  return fonts;
}