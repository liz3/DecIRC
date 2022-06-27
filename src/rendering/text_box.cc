#include "text_box.h"
#include <math.h>
#include "../AppState.h"
#include "box.h"
#include "../utils/unicode_utils.h"
#include "image.h"

TextBox::TextBox(TextWithState& text) : text(text) {
  atlas = &AppState::gState->atlas;
  m_shader = AppState::gState->opengl_state.text_shader;
}
int TextBox::computeHeight(float w, int preclude) {
  int size = preclude;
  int skip = 0;
  float currentAdvance = 0;
  auto preproc = preprocess();
  for (int i = 0; i < preproc.size(); i++) {
    if (skip > 0) {
      skip--;
      continue;
    }
    auto cp = preproc[i];
    if (cp.cp == '\n') {
      size++;
      currentAdvance = 0;
    } else {
      currentAdvance += cp.type == 1
                            ? (atlas->font_size * scale)
                            : atlas->getAdvance(cp.cp, cp.style, scale);
      if (currentAdvance >= w) {
        size++;
        currentAdvance = 0;
      }
    }
  }
 
  return size;
}
float TextBox::computeHeightAbsolute(float w, int preclude) {
  int count = computeHeight(w, preclude);
  return  count * atlas->effective_atlas_height;
}
std::vector<RichChar>& TextBox::preprocess() {
  if (last_data_point != -1 && last_data_point == text.last_point)
    return rich_cache;
  rich_cache.clear();
  for (int i = 0; i < text.data.size(); ++i) {
    auto cp = text.data[i];
    if (discordMode && cp == '<') {
      std::string discord_entry = "";
      for (int x = i + 1; x < text.data.size(); ++x) {
        if (text.data[x] == '>')
          break;
        discord_entry += (char)text.data[x];
      }
      std::cout << discord_entry << "\n";
      if (discord_entry.find("@") == 0) {
        std::string id = discord_entry.substr(1);
        if (msg_ref) {
          if (msg_ref->mentions.count(id)) {
            DiscordUser& u = msg_ref->mentions[id];
            auto vec = UnicodeUtils::utf8_to_codepoint("@" + u.username);

            for (auto cpp : vec) {
              RichChar ch;
              ch.cp = cpp;
              ch.style = "bold";
              ch.type = 0;
              rich_cache.push_back(ch);
            }
            i += 1 + discord_entry.size();
            continue;
          }
        }
      } else if (discord_entry.find(":") == 0 || discord_entry.find("a") == 0) {
        std::string id =
            discord_entry.substr(discord_entry.find_last_of(":") + 1);
        RichChar ch;
        ch.type = 1;
        ch.emote = id;
        rich_cache.push_back(ch);
        i += 1 + discord_entry.size();
        continue;
      }
    }
    RichChar ch;
    ch.type = 0;
    ch.style = style;
    ch.cp = cp;
    rich_cache.push_back(ch);
  }
  last_data_point = text.last_point;
  return rich_cache;
}
void TextBox::render(float x, float y, float w, float h) {

  int amount = h / atlas->effective_atlas_height;
  h = amount * atlas->effective_atlas_height;
  std::vector<RenderChar> entries;
  float offset = 0;
  float offsetY = 0;
  float lOffset = 0;
  float lxOffset = 0;
  int yCount = 0;
  int binaryXOffset = 0;
  auto pos = text.getCursorPosition();
  float remainingX = w;
  float remainingY = h;
  auto preproc = preprocess();
  if (allowGrow) {
    int addedLines = text.countChar('\n');
    for (auto cp : preproc) {
      if (cp.cp == '\n') {
        int needed = std::floor(offset / w);
        addedLines += needed;
        offset = 0;
        continue;
      }
      offset += cp.type == 1 ? (atlas->font_size * scale)
                             : atlas->getAdvance(cp.cp, cp.style, scale);
    }
    int needed = std::floor(offset / w);
    addedLines += needed;
    offset = 0;
    float res = (atlas->effective_atlas_height * addedLines) -
                (h - atlas->effective_atlas_height);
    if (growDown) {
      if (res > 0) {
        remainingY += res;
        h += res;
      }
    } else {
      remainingY -= atlas->effective_atlas_height * addedLines;
      if (res > 0) {
        y += res;
        h += res;
      }
    }
  }
  if (background.w > 0) {
    Box::render(x, y + atlas->effective_atlas_height + 10, w, -(h + 20),
                background);
  }
  m_shader->shader.use();
  m_shader->bindVertexArray();
  atlas->bindTexture();
  m_shader->bindBuffer();
  for (auto cp : preproc) {
    binaryXOffset++;
    if (cp.cp == '\n') {
      yCount++;
      offsetY += atlas->effective_atlas_height;
      offset = 0;
      remainingY -= atlas->effective_atlas_height;
      if (!allowGrow && remainingY <= 0) {
        break;
      }
      if (binaryXOffset == text.text_x) {
        lxOffset = offset;
        lOffset = offsetY;
      }
      continue;
    }

    if (offset + atlas->font_size >= w && w > 0) {
      offsetY += atlas->effective_atlas_height;
      offset = 0;
      remainingY -= atlas->effective_atlas_height;
      if (!allowGrow && remainingY <= 0) {
        break;
      }
    }
    if (cp.type == 0) {
      entries.push_back(atlas->render(cp.cp, x + offset, (-y) + offsetY, color,
                                      cp.style, scale));
    } else if (cp.type == 1) {
      auto* res = AppState::gState->client->image_cache.getEmote(cp.emote);
      if (res) {
        float imageScale =
            (atlas->effective_atlas_height - 5) / (float)res->width;
        imageScale *= scale;
        res->render(x + offset,
                    (-y) + (offsetY - (atlas->effective_atlas_height * 0.7)),
                    imageScale);
        // need to reactivate text shader
        m_shader->shader.use();
        m_shader->bindVertexArray();
        atlas->bindTexture();
        m_shader->bindBuffer();
      }
    }

    offset += cp.type == 1 ? (atlas->font_size * scale)
                           : atlas->getAdvance(cp.cp, cp.style, scale);
    if (binaryXOffset == text.text_x) {
      lxOffset = offset;
      lOffset = offsetY;
    }
  }
  if (entries.size()) {
      glBufferSubData(
          GL_ARRAY_BUFFER, 0, sizeof(RenderChar) * entries.size(),
          &entries[0]);  // be sure to use glBufferSubData and not glBufferData
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, (GLsizei)entries.size());
  }
  if (renderCursor) {
    auto offsetY = y + -(lOffset);
    auto offsetX = x + lxOffset;
    Box::render(offsetX, offsetY, 4, atlas->effective_atlas_height,
                vec4fs(1.0));
  }
}