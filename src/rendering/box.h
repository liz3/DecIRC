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
};
#endif