#ifndef DEC_CHAT_INPUT
#define DEC_CHAT_INPUT
#include "message_list.h"
#include "text_field.h"
#include "../../third-party/glfw/include/GLFW/glfw3.h"

#include <map>
class ChatInput : public TextField {
 public:
  MessageList* list = nullptr;
  DiscordClient* client;
  std::map<HttpFileEntry*, Image*> images;
  ChatInput() {}
    void onKey(GLFWwindow* window,
             int key,
             int scancode,
             int action,
             int mods) override {
        bool isPress = action == GLFW_PRESS || action == GLFW_REPEAT;
        if(isPress && key == GLFW_KEY_ESCAPE) {
          if(images.size()) {
            auto it = images.begin();
            std::advance(it, images.size()-1);
            if (it == images.end())
                return;
            it->second->remove();
            delete it->second;
            auto e = std::find(client->sendFiles.begin(), client->sendFiles.end(), it->first);
            client->sendFiles.erase(e);
            delete it->first;
            images.erase(it->first);
            return;
          }
        }
      TextField::onKey(window, key, scancode, action, mods);
    }
  void render(float x, float y, float w, float h) override {
    TextField::render(x,y,w,h);
    if(client->sendFiles.size()) {
      std::vector<HttpFileEntry*> existing;
      for(auto& e : client->sendFiles) {
        existing.push_back(e);
        if(!images.count(e)) {
          Image* img = new Image();
          img->init_from_mem(e->data);
          images[e] = img;
          std::cout << "adding new image\n";
        }
      }
      for(auto& e : images) {
        if(std::find(existing.begin(), existing.end(), e.first) == existing.end()) {
          delete e.second;
          images.erase(e.first);
        }
      }
      auto height = box.computeHeightAbsolute(w);
        auto absolute = AppState::gState->getPositionAbsolute(x, y, w, 200);
       Box::render(absolute.x, absolute.y+height+185, w, 200, vec4f(0,0,0,1));
       float xx = absolute.x;
      for(auto& e : images) {
        Image* img = e.second;
       
          float scale = (float)190 / img->height;
          img->render(xx, -(absolute.y+(height)+185 + 190), scale);
          xx += img->width * scale;
        
      }
    }
  }
  void onFocus(bool focus) override {
    box.renderCursor = focus;
    if (list)
      list->hasFocus = focus;
  }
};
#endif