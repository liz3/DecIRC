#ifndef DEC_BOX
#define DEC_BOX
#include "../AppState.h"
#include "glad.h"
#include "la.h"
class Box {
 public:
  static void render(float x, float y, float w, float h, Vec4f color) {
    ShaderInstance* m_shader = AppState::gState->opengl_state.box_shader;
    m_shader->shader.use();
    m_shader->bindVertexArray();
    m_shader->bindBuffer();

    ColorEntry entry = {vec2f(x, y), vec2f(w, h), color};
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ColorEntry), &entry);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, (GLsizei)1);
  }
  static void renderWithOutline(float x, float y, float w, float h, Vec4f color, float border_width, Vec4f border_color) {
    Box::render(x,y,w,h,color);
    Box::render(x,y,border_width, h, border_color);
    Box::render(x,y,w, border_width, border_color);
    Box::render(x+(w-border_width),y,border_width, h, border_color);
    Box::render(x,y+(h-border_width),w, border_width, border_color);
  }
};
#endif