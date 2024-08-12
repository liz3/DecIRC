#include "text_box.h"
#include <math.h>
#include "../AppState.h"
#include "box.h"
#include "../utils/unicode_utils.h"
#include "image.h"
#include "irc_colors.h"

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
      auto adv = cp.type == 1
                            ? (atlas->font_size * scale)
                            : atlas->getAdvance(cp.cp, cp.style, scale);
      if (currentAdvance + adv >= w) {
        size++;
        currentAdvance = 0;
      }
        currentAdvance += adv;
    }
  }

  return size;
}
float TextBox::computeHeightAbsolute(float w, int preclude) {
  int count = computeHeight(w, preclude);
  return count * atlas->effective_atlas_height;
}
std::vector<RichChar>& TextBox::preprocess() {
  if (last_data_point != -1 && last_data_point == text.last_point)
    return rich_cache;
  rich_cache.clear();
  bool bold = false;
  bool italics = false;
  bool underline = false;
  bool strikethrough = false;
  bool monospace = false;
  bool color = false;
  Vec4f fg_color = vec4fs(1);
  for (int i = 0; i < text.data.size(); ++i) {
    auto cp = text.data[i];
    RichChar ch;
    ch.type = 0;
    ch.style = style;
    ch.color = this->color;
    ch.underline = underline;
    ch.strikethrough = strikethrough;
    ch.cp = cp;
    if (richRender) {
      if (cp == 0x02) {
        bold = !bold;
        continue;
      }
       if (cp == 0x04) {
        i += 6;
        continue;
      }
      if(cp == 0x16) {
        continue;
      }
      if (cp == 0x0F) {
        bold = false;
        italics = false;
        underline = false;
        strikethrough = false;
        monospace = false;
        color = false;
        continue;
      }
      if (cp == 0x1D) {
        italics = !italics;
        continue;
      }
      if (cp == 0x1F) {
        underline = !underline;
        continue;
      }
      if (cp == 0x1E) {
        strikethrough = !strikethrough;
        continue;
      }
      if (cp == 0x11) {
        monospace = !monospace;
        continue;
      }
      if (cp == 0x03) {
        auto next = text.data[i + 1];
        if (next == ',') {
          color = false;
          continue;
        }
        if (next < '0' || next > '9')
          continue;
        i += 1;
        std::string fg(1, (char)next);
        next = text.data[i + 1];
        if (next >= '0' && next <= '9') {
          fg += next;
          i += 1;
          next = text.data[i + 1];
        }
        if (next == ',') {
          char after =  text.data[i + 2];
          if (after >= '0' && after <= '9') {
              i += 2;
              std::string bg(1, (char)after);
              after = text.data[i + 1];
              if (after >= '0' && after <= '9') {
                bg += after;
                i += 1;
              }
          }
        }
        size_t cc = std::stoi(fg);
        if(cc == 99) {
          color = false;
        } else {
            color = true;
            fg_color = IRC_COLORS[cc];
        }
        continue;

      }
      if (bold)
        ch.style = "bold";
      else if (italics)
        ch.style = "italic";
      if(bold && italics)
        ch.style = "bold_italic";
      if (color)
        ch.color = fg_color;
    }

    rich_cache.push_back(ch);
  }
  last_data_point = text.last_point;
  return rich_cache;
}
void TextBox::renderLine(float xStart, float xEnd, float y) {
  Box::render(xStart, -y - 4, xEnd - xStart, 2, this->color);

  m_shader->shader.use();
  m_shader->bindVertexArray();
  atlas->bindTexture();
  m_shader->bindBuffer();
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
  Marker underline;
  Marker strikethrough;
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
      if (strikethrough.active) {
        renderLine(strikethrough.startX, x + offset,
                   ((-y) + offsetY) - (atlas->effective_atlas_height * 0.4));

        strikethrough.startX = x;
      }
      if (underline.active) {
        renderLine(underline.startX, x + offset, (-y) + offsetY);
        underline.startX = x;
      }
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
    auto charAdv = cp.type == 1 ? (atlas->font_size * scale)
                               : atlas->getAdvance(cp.cp, cp.style, scale);
    if (w > 0 && offset + charAdv >= w) {
      if (strikethrough.active) {
        renderLine(strikethrough.startX, x + offset,
                   ((-y) + offsetY) - (atlas->effective_atlas_height * 0.4));
        strikethrough.startX = x;
      }
      if (underline.active) {
        renderLine(underline.startX, x + offset, (-y) + offsetY);
        underline.startX = x;
      }
      offsetY += atlas->effective_atlas_height;
      offset = 0;
      remainingY -= atlas->effective_atlas_height;
      if (!allowGrow && remainingY <= 0) {
        break;
      }
    }
    if (cp.underline && !underline.active) {
      underline.active = true;
      underline.startX = x + offset;
    } else if (!cp.underline && underline.active) {
      renderLine(underline.startX, x + offset, (-y) + offsetY);
      underline.active = false;
    }
    if (cp.strikethrough && !strikethrough.active) {
      strikethrough.active = true;
      strikethrough.startX = x + offset;
    } else if (!cp.strikethrough && strikethrough.active) {
      renderLine(strikethrough.startX, x + offset, (-y) + offsetY);
      strikethrough.active = false;
    }

    entries.push_back(atlas->render(cp.cp, x + offset, (-y) + offsetY, cp.color,
                                    cp.style, scale));

    offset += charAdv;
    if (binaryXOffset == text.text_x) {
      lxOffset = offset;
      lOffset = offsetY;
    }
  }
  if (strikethrough.active) {
    renderLine(strikethrough.startX, x + offset,
               ((-y) + offsetY) - (atlas->effective_atlas_height * 0.4));
    strikethrough.startX = x;
  }
  if (underline.active) {
    renderLine(underline.startX, x + offset, (-y) + offsetY);
    underline.startX = x;
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