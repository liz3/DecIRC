#include "font_atlas.h"
#include "../utils/unicode_utils.h"

FontAtlas::FontAtlas() {
  valid = false;
}

FontAtlas::FontAtlas(uint32_t fontSize, std::vector<FontPair> m_fonts) {
  font_size = fontSize;
  init();
  for (int i = 0; i < m_fonts.size(); ++i) {
    std::string type = m_fonts[i].type;

    fonts.push_back(init_font(m_fonts[i].path, i == 0, type));
  }
}
void FontAtlas::init() {
  if (FT_Init_FreeType(&ft)) {
    std::cout << "ERROR::FREETYPE: Could not init FreeType Library"
              << std::endl;
    return;
  }
}
std::vector<FontEntry*> FontAtlas::getFontsByType(const std::string& type) {
  std::vector<FontEntry*> temp;
  for (auto* e : fonts) {
    if (e->type == type)
      temp.push_back(e);
  }
  return temp;
}
FontEntry* FontAtlas::getFontByType(const std::string& type) {
  for (auto* e : fonts) {
    if (e->type == type)
      return e;
  }
  return nullptr;
}
void FontAtlas::genTexture() {
  if (wasGenerated)
    glDeleteTextures(1, &texture_id);
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  wasGenerated = true;
}
RenderChar FontAtlas::render(int32_t cp,
                             float x,
                             float y,
                             Vec4f color,
                             std::string type,
                             float scale) {
  std::string target_type = type;
  std::string backup_key;
  bool loaded = true;
  if (!characters.count(cp) || !characters[cp].entries.count(type)) {
    loaded = false;
    if (getFontByType(type)) {
      FontEntry* e = getFontByType(type);
      uint32_t glyph_index = FT_Get_Char_Index(e->face, cp);
      if (glyph_index != 0) {
        lazyLoadCharacter(e, cp, type);
        loaded = true;
      }
    }

    if (!loaded) {
      std::vector<std::string> order;
      if (type == "normal") {
        order = {"normal", "bold", "italic", "bold_italic", "emoji"};
      } else if (type == "bold") {
        order = {"bold", "normal", "italic", "bold_italic", "emoji"};
      } else if (type == "bold_italic") {
        order = {"bold_italic", "italic", "bold", "normal", "emoji"};
      } else if (type == "italic") {
        order = {"italic", "normal", "bold", "bold_italic", "emoji"};
      } else {
        order = {"normal", "bold", "italic", "bold_italic", "emoji"};
      }

      for (auto& s : order) {
        if (characters.count(cp) && characters[cp].entries.count(s)){
          if(!backup_key.size())
            backup_key =s;
          continue;
        }
        auto entries = getFontsByType(s);
        if (!entries.size())
          continue;
        for (auto* e : entries) {
          uint32_t glyph_index = FT_Get_Char_Index(e->face, cp);
          if (glyph_index != 0) {
            lazyLoadCharacter(e, cp, s);
            target_type = s;
            loaded = true;
            break;
          }
        }
        if (loaded)
          break;
      }
    }
  }

  RenderChar r;
  if(!loaded) {
    if(backup_key.size())
      target_type = backup_key;
    else
      return r;
  }
  auto* entry = &characters[cp].entries[target_type];

  float x2 = x + (entry->left * scale);

  float y2 = y - (entry->top * scale);
  if (entry->hasColor) {
    float height = entry->height * (font_size / entry->height);
    y2 +=
        (entry->top * scale) - ((height * scale) - (font_size * scale) * 0.15);
  }
  r.pos = vec2f(x2, (-y2));
  if (entry->hasColor) {
    float height = entry->height * (font_size / entry->height);
    r.size = vec2f((font_size)*scale, (-height) * scale);
  } else {
    r.size = vec2f((entry->width) * scale, (-entry->height) * scale);
  }

  r.uv_pos = vec2f(entry->offset, 0.0f);
  r.uv_size =
      vec2f(entry->width / (float)atlas_width, entry->height / atlas_height);
  r.fg_color = color;
  r.hasColor = entry->hasColor ? 1 : 0;
  return r;
}
FontEntry* FontAtlas::init_font(std::string path,
                                bool preload,
                                std::string type) {
  FontEntry* entry = new FontEntry();
  FT_Face face = entry->face;
  if (FT_New_Face(ft, path.c_str(), 0, &face)) {
    delete entry;
    return nullptr;
  }
  entry->type = type;
  entry->hasColor = isColorEmojiFont(face);
  if (!entry->hasColor)
    FT_Set_Pixel_Sizes(face, 0, font_size);
  else
    FT_Select_Size(face, 0);

  if (preload) {
    atlas_width = 0;
    atlas_height = 0;
    smallest_top = 1e9;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (int i = 0; i < 128; i++) {
      if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
        std::cout << "Failed to load char: " << (char)i << "\n";
        return nullptr;
      }
      auto bm = face->glyph->bitmap;
      atlas_width += bm.width;
      atlas_height = bm.rows > atlas_height ? bm.rows : atlas_height;
    }
    effective_atlas_height = atlas_height + 5;
    genTexture();
    atlas_width *= 2;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)atlas_width,
                 (GLsizei)atlas_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    xOffset = 0;
    for (int32_t i = 0; i < 128; i++) {
      if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
        std::cout << "Failed to load char: " << (char)i << "\n";
        return nullptr;
      }

      CharacterEntry entry;
      auto bm = face->glyph->bitmap;
      entry.width = bm.width;
      entry.height = bm.rows;

      entry.top = face->glyph->bitmap_top;
      entry.left = face->glyph->bitmap_left;
      entry.advance = face->glyph->advance.x >> 6;
      entry.xPos = xOffset;
      entry.hasColor = false;
      entry.type = type;
      entry.c = (char16_t)i;
      auto width = bm.width;
      auto height = bm.rows;
      for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
          int index = ((y * width) + x);
          int val = bm.buffer[index];
          entry.data.push_back(bm.buffer[index]);
          entry.data.push_back(bm.buffer[index]);
          entry.data.push_back(bm.buffer[index]);
          entry.data.push_back(bm.buffer[index]);
        }
      }

      if (smallest_top == 0 && entry.top > 0)
        smallest_top = entry.top;
      else
        smallest_top = entry.top < smallest_top && entry.top != 0
                           ? entry.top
                           : smallest_top;

      entry.offset = (float)xOffset / (float)atlas_width;
      if (!characters.count(entry.c)) {
        CharacterEntryMap map;
        map.c = entry.c;
        map.entries[type] = entry;
        characters.insert(std::pair<int32_t, CharacterEntryMap>(entry.c, map));
      } else {
        CharacterEntryMap& mp = characters[entry.c];
        mp.entries[type] = entry;
      }

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      if (entry.data.size())
        glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, 0, entry.width, entry.height,
                        GL_RGBA, GL_UNSIGNED_BYTE, &(entry.data[0]));

      xOffset += entry.width;
    }
  }
  std::cout << "init" << path << ":" << characters.size() << "\n";
  entry->face = face;
  return entry;
}
void FontAtlas::lazyLoadCharacter(FontEntry* fe, int32_t cp, std::string type) {
  std::vector<int32_t> xx = {cp};
  FT_Face face = fe->face;
  CharacterEntry entry;
  auto f = FT_LOAD_RENDER;
  if (fe->hasColor)
    f |= FT_LOAD_COLOR;
  if (FT_Load_Char(face, cp, f)) {
    std::cout << "Failed to load char: " << (int32_t)cp << "\n";
    return;
  }
  bool increased_size = false;
  auto bm = face->glyph->bitmap;
  entry.width = bm.width;
  entry.height = bm.rows;
  entry.top = face->glyph->bitmap_top;
  entry.left = face->glyph->bitmap_left;
  entry.advance = face->glyph->advance.x >> 6;
  entry.xPos = xOffset;
  entry.hasColor = fe->hasColor;
  entry.c = cp;
  entry.type = type;
  auto old_width = atlas_width;
  auto old_height = atlas_height;
  for (int y = 0; y < entry.height; ++y) {
    for (int x = 0; x < entry.width; ++x) {
      int index = ((y * entry.width) + x);
      if (entry.hasColor) {
        index *= 4;
        entry.data.push_back(bm.buffer[index + 2]);
        entry.data.push_back(bm.buffer[index + 1]);
        entry.data.push_back(bm.buffer[index]);
        entry.data.push_back(bm.buffer[index + 3]);
      } else {
        entry.data.push_back(bm.buffer[index]);
        entry.data.push_back(bm.buffer[index]);
        entry.data.push_back(bm.buffer[index]);
        entry.data.push_back(bm.buffer[index]);
      }
    }
  }

  atlas_width += bm.width;
  increased_size = bm.rows > atlas_height;
  atlas_height = bm.rows > atlas_height ? bm.rows : atlas_height;
  if (!entry.hasColor && increased_size)
    effective_atlas_height = atlas_height + 5;

  entry.offset = (float)xOffset / (float)atlas_width;
  genTexture();
  // params
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)atlas_width,
               (GLsizei)atlas_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  for (std::map<int32_t, CharacterEntryMap>::iterator mapIter =
           characters.begin();
       mapIter != characters.end(); ++mapIter) {
    for (std::map<std::string, CharacterEntry>::iterator it =
             mapIter->second.entries.begin();
         it != mapIter->second.entries.end(); ++it) {
      it->second.offset = (float)it->second.xPos / (float)atlas_width;
      if (it->second.data.size())
        glTexSubImage2D(GL_TEXTURE_2D, 0, it->second.xPos, 0, it->second.width,
                        it->second.height, GL_RGBA, GL_UNSIGNED_BYTE,
                        &it->second.data[0]);
    }
  }
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, 0, entry.width, entry.height,
                  GL_RGBA, GL_UNSIGNED_BYTE, entry.data.data());

  xOffset += entry.width;
  if (!characters.count(entry.c)) {
    CharacterEntryMap map;
    map.c = entry.c;
    map.entries[type] = entry;
    characters.insert(std::pair<int32_t, CharacterEntryMap>(entry.c, map));
  } else {
    CharacterEntryMap& mp = characters[entry.c];
    mp.entries[type] = entry;
  }
}
void FontAtlas::bindTexture() {
  glBindTexture(GL_TEXTURE_2D, texture_id);
}
float FontAtlas::getAdvance(int32_t cp, std::string type, float scale) {
  std::string target_type = type;
  std::string backup_key;
  if (!characters.count(cp) || !characters[cp].entries.count(type)) {
    bool loaded = false;
    if (getFontByType(type)) {
      FontEntry* e = getFontByType(type);
      uint32_t glyph_index = FT_Get_Char_Index(e->face, cp);
      if (glyph_index != 0) {
        lazyLoadCharacter(e, cp, type);
        loaded = true;
      }
    }

    if (!loaded) {
      std::vector<std::string> order;
      if (type == "normal") {
        order = {"normal", "bold", "italic", "bold_italic", "emoji"};
      } else if (type == "bold") {
        order = {"bold", "normal", "italic", "bold_italic", "emoji"};
      } else if (type == "bold_italic") {
        order = {"bold_italic", "italic", "bold", "normal", "emoji"};
      } else if (type == "italic") {
        order = {"italic", "normal", "bold", "bold_italic", "emoji"};
      } else {
        order = {"normal", "bold", "italic", "bold_italic", "emoji"};
      }
      for (auto& s : order) {
        if (characters.count(cp) && characters[cp].entries.count(s)){
          if(!backup_key.length())
            backup_key = s;
          continue;
        }
        auto entries = getFontsByType(s);
        if (!entries.size())
          continue;
        for (auto* e : entries) {
          uint32_t glyph_index = FT_Get_Char_Index(e->face, cp);
          if (glyph_index != 0) {
            lazyLoadCharacter(e, cp, s);
            target_type = s;
            loaded = true;
            break;
          }
        }
        if (loaded)
          break;
      }
    }
    if (!loaded) {
      if(backup_key.length()) {
        target_type = backup_key;
      } else {
      std::cout << "load failed: " << std::hex << cp << "\n";
      return 0;
      }
    }
  }
  auto e = &characters[cp].entries[target_type];

  if (e->hasColor) {
    return font_size * scale;
  }
  return e->advance * scale;
}
float FontAtlas::getAdvance(std::vector<int32_t>& points,
                            std::string type,
                            float scale) {
  float advance = 0;
  for (auto e : points) {
    advance += getAdvance(e, type, scale);
  }
  return advance;
}
bool FontAtlas::isColorEmojiFont(FT_Face face) {
  static const uint32_t tag = FT_MAKE_TAG('C', 'B', 'D', 'T');
  unsigned long length = 0;
  FT_Load_Sfnt_Table(face, tag, 0, nullptr, &length);
  if (length)
    return true;

  return false;
}