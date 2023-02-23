
#include "AppState.h"
#include <sodium.h>
#include "gui_components.h"
#include "event_receiver.h"

#include "../third-party/glfw/include/GLFW/glfw3.h"

AppState* AppState::gState = nullptr;
AppState::AppState() : opengl_state(nullptr) {
  gState = this;
}
void AppState::start() {
  if (sodium_init() < 0) {
    // TODO handle errror
    return;
  }
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  window =
      glfwCreateWindow(window_width, window_height, "dec", nullptr, nullptr);
  if (window == NULL) {
    const char* description;
    int code = glfwGetError(&description);
    std::cout << "Failed to create GLFW window: " << description << std::endl;
    glfwTerminate();
    return;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetCharCallback(window, character_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetWindowFocusCallback(window, window_focus_callback);
  opengl_state = OpenGLState(window);

  atlas = FontAtlas(30, {"./assets/fonts/FiraCode-Regular.ttf",
                         "./assets/fonts/NotoColorEmoji.ttf",
                         "./assets/fonts/FiraCode-Bold.ttf",
                         "./assets/fonts/NotoSansMath-Regular.ttf"});
  atlas.valid = true;
  float xscale, yscale;
  glfwGetWindowContentScale(window, &xscale, &yscale);
  window_width *= xscale;
  window_height *= yscale;

  auto c = GuiComponents(this);
  c.init();
  components = &c;
  client = create_irc_event_handler();
  client->init(components);
  components->chat_input.client = client;
  runGuiLoop();
  client->closeAll();
}
Vec2f AppState::getPositionAbsolute(float x, float y, float w, float h) {
  return vec2f(x - ((window_width / 2)), (-y) + ((window_height / 2) - h));
}

void AppState::setTextReceiver(TextReceiver* recv) {
  if (current_text_receiver) {
    current_text_receiver->onFocus(false);
  }
  current_text_receiver = recv;
  if (current_text_receiver)
    current_text_receiver->onFocus(true);
}
void AppState::emptyEvent() {
  glfwPostEmptyEvent();
}
void AppState::runGuiLoop() {
  setTextReceiver(&components->chat_input);
  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.15, 0.15, 0.25, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    opengl_state.setResolution(window_width, window_height);

    components->render();

    glfwSwapBuffers(window);
    glfwWaitEvents();
  }
  glfwTerminate();
}
