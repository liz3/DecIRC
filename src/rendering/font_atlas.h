#ifndef DEC_FONT_ATLAS
#define DEC_FONT_ATLAS
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "../../third-party/freetype2/include/ft2build.h"
#include "glad.h"
#include "opengl_state.h"
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

struct FontPair {
  std::string path;
  std::string type;
};
struct FontEntry {
  FT_Face face;
  std::string type;
  bool hasColor;
};
struct CharacterEntry {
  float width;
  float height;
  float top;
  float left;
  float advance;
  float offset;
  std::vector<uint8_t> data;
  int xPos;
  int32_t c;
  std::string type;
  bool hasColor = false;
};
class CharacterEntryMap {
 public:
  std::map<std::string, CharacterEntry> entries;

  int32_t c;
};

class FontAtlas {
 private:
  FT_Library ft;
  std::vector<FontEntry*> fonts;
  std::map<int32_t, CharacterEntryMap> characters;
  FontEntry* mainFont = nullptr;
  bool wasGenerated = false;
  GLuint texture_id;
  int xOffset;
  void init();
  FontEntry* init_font(std::string path, bool preload, std::string style);
  void genTexture();
  bool isColorEmojiFont(FT_Face face);
  void lazyLoadCharacter(FontEntry* entry, int32_t cp, std::string type);
  FontEntry* getFontByType(const std::string& type);
  std::vector<FontEntry*> getFontsByType(const std::string& type);
 public:
  bool valid = false;
  int n_count = 0;

  uint32_t font_size = 25;
  FT_UInt atlas_width, atlas_height, smallest_top, effective_atlas_height;
  FontAtlas(uint32_t fontSize, std::vector<FontPair> fonts);
  FontAtlas();
  ~FontAtlas() {
    if (!valid)
      return;
    for (auto* e : fonts) {
      FT_Done_Face(e->face);
      delete e;
    }
    FT_Done_FreeType(ft);
  }
  RenderChar render(int32_t cp,
                    float x,
                    float y,
                    Vec4f color,
                    std::string type = "normal",
                    float scale = 1.0);
  void bindTexture();
  float getAdvance(int32_t cp, std::string type = "normal", float scale = 1.0);
  float getAdvance(std::vector<int32_t>& points,
                   std::string type = "normal",
                   float scale = 1.0);
};

#endif