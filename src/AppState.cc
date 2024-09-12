
#include "AppState.h"
#include "components/mouse_receiver.h"
#include "gui_components.h"
#include "event_receiver.h"
#include "./utils/notifications.h"

#include "../third-party/glfw/include/GLFW/glfw3.h"

AppState* AppState::gState = nullptr;
AppState::AppState(std::filesystem::path cwd)
    : cwd(cwd), opengl_state(nullptr, cwd) {
  gState = this;
}
void AppState::start() {
  Notifications::init();
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  window =
      glfwCreateWindow(window_width, window_height, "DecIRC", nullptr, nullptr);
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
  glfwSetCursorPosCallback(window, mouse_position_callback);
  glfwSetScrollCallback(window, scroll_callback);
  opengl_state = OpenGLState(window, cwd);

  std::vector<std::pair<std::string, std::string>> fs = {{"MapleMono-Regular.ttf", "normal"}, {"NotoColorEmoji.ttf", "emoji"},
                        {"MapleMono-Bold.ttf", "bold"}, {"MapleMono-Italic.ttf", "italic"}, {"MapleMono-BoldItalic.ttf", "bold_italic"}, {"NotoSansMath-Regular.ttf", "normal"},{"FiraCode-Regular.ttf", "normal"}, {"NotoSansJP-Regular.ttf", "normal"}, {"NotoSansJP-Bold.ttf", "bold"}};
  std::vector<FontPair> fontPaths;
  for (const auto& r : fs) {
    std::filesystem::path p = cwd / "assets" / "fonts" / r.first;
    fontPaths.push_back({p.generic_string(), r.second});
  }
  atlas = FontAtlas(config.getFontSize(), fontPaths);
  atlas.valid = true;
  float xscale, yscale;
  glfwGetWindowContentScale(window, &xscale, &yscale);
  window_width *= xscale;
  window_height *= yscale;
  window_scale = xscale;

  auto c = GuiComponents(this);
  c.init();
  components = &c;
  client = create_irc_event_handler();
  client->init(components);
  components->chat_input.client = client;
  runGuiLoop();
  client->persistChannels();
  client->closeAll();
}
Vec2f AppState::getPositionAbsolute(float x, float y, float w, float h) {
  return vec2f(x - ((window_width / 2)), (-y) + ((window_height / 2) - h));
}

void AppState::setTextReceiver(TextReceiver* recv, MouseReceiver* mouse_recv) {
  if (current_text_receiver) {
    current_text_receiver->onFocus(false);
  }
  current_text_receiver = recv;
  if(!mouse_recv)
     current_mouse_receiver = &components->message_list;
  else
     current_mouse_receiver = mouse_recv;
  if (current_text_receiver)
    current_text_receiver->onFocus(true);
}
void AppState::emptyEvent() {
  glfwPostEmptyEvent();
}
void AppState::runGuiLoop() {
  setTextReceiver(&components->chat_input);
  current_mouse_receiver = &components->message_list;
  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.1, 0.1, 0.1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    opengl_state.setResolution(window_width, window_height);

    components->render();

    glfwSwapBuffers(window);
    glfwWaitEventsTimeout(1);
  }
  glfwTerminate();
}
