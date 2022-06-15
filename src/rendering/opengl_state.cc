#include "opengl_state.h"
#include "../../third-party/glfw/include/GLFW/glfw3.h"

OpenGLState::OpenGLState(GLFWwindow* window) : m_window(window) {
  if (window == nullptr)
    return;
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return;
  }

  // OpenGL state
  // ------------
  glEnable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  image_shader = new ShaderInstance(
      "../assets/simple_image.vert", "../assets/simple_image.frag", 16,
      {{2, sizeof(SimpleEntry), GL_FLOAT, (void*)offsetof(SimpleEntry, pos)},
       {2, sizeof(SimpleEntry), GL_FLOAT, (void*)offsetof(SimpleEntry, size)}});

  text_shader = new ShaderInstance(
      "../assets/text.vert", "../assets/text.frag",
      sizeof(RenderChar) * 400 * 1000,
      {{2, sizeof(RenderChar), GL_FLOAT, (void*)offsetof(RenderChar, pos)},
       {2, sizeof(RenderChar), GL_FLOAT, (void*)offsetof(RenderChar, size)},
       {2, sizeof(RenderChar), GL_FLOAT, (void*)offsetof(RenderChar, uv_pos)},
       {2, sizeof(RenderChar), GL_FLOAT, (void*)offsetof(RenderChar, uv_size)},
       {4, sizeof(RenderChar), GL_FLOAT, (void*)offsetof(RenderChar, fg_color)},
       {4, sizeof(RenderChar), GL_FLOAT, (void*)offsetof(RenderChar, bg_color)},
       {1, sizeof(RenderChar), GL_FLOAT,
        (void*)offsetof(RenderChar, hasColor)}});

  box_shader = new ShaderInstance(
      "../assets/box.vert", "../assets/box.frag", sizeof(ColorEntry) * 16,
      {{2, sizeof(ColorEntry), GL_FLOAT, (void*)offsetof(ColorEntry, pos)},
       {2, sizeof(ColorEntry), GL_FLOAT, (void*)offsetof(ColorEntry, size)},
       {4, sizeof(ColorEntry), GL_FLOAT, (void*)offsetof(ColorEntry, color)}});

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}
void OpenGLState::setResolution(uint32_t w, uint32_t h) {
  image_shader->shader.use();
  image_shader->shader.set2f("resolution", (float)w, (float)h);

  text_shader->shader.use();
  text_shader->shader.set2f("resolution", (float)w, (float)h);

  box_shader->shader.use();
  box_shader->shader.set2f("resolution", (float)w, (float)h);
}
ShaderInstance::ShaderInstance(std::string vert_path,
                               std::string frag_path,
                               uint32_t size,
                               std::vector<ShaderVar> entries) {
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
  int count = 0;
  for (auto& entry : entries) {
    glEnableVertexAttribArray(count);
    glVertexAttribPointer(count, entry.count, entry.type, GL_FALSE, entry.size,
                          entry.offset);
    glVertexAttribDivisor(count, 1);
    count++;
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  std::string vert_content = FileUtils::file_to_string(vert_path);
  std::string frag_content = FileUtils::file_to_string(frag_path);
  shader = Shader(vert_content, frag_content, {});
}
void ShaderInstance::bindVertexArray() {
  glBindVertexArray(vao);
}
void ShaderInstance::bindBuffer() {
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
}