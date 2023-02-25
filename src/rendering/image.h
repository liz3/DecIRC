#ifndef DEC_IMAGE
#define DEC_IMAGE
#include <string>
#include <vector>
#include "../../third-party/png/lodepng.h"
#include "../../third-party/libwebp/src/webp/decode.h"
#include "../../third-party/libjpeg/jpeglib.h"
#include "../../third-party/gifdec/gifdec.h"

#include "../AppState.h"

class Image {
 public:
  std::vector<unsigned char> data;
  unsigned width, height;
  GLuint tex_id;
  ShaderInstance* m_shader;
  bool valid = false;

  void init() {
    valid = true;
    glGenTextures(1, &tex_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
                    GL_UNSIGNED_BYTE, &data[0]);
  }

  void remove() {
    if (!valid)
      return;
    glDeleteTextures(1, &tex_id);
    std::cout << "destroying\n";
    valid = false;
  }
  Image(std::string path) {
    unsigned error = lodepng::decode(data, width, height, path);
    init();
  }
  Image(std::vector<uint8_t> data, unsigned w, unsigned h) {
    this->data = data;
    width = w;
    height = h;
    init();
  }
  Image() {}

  Image(ImageCacheEntry* entry) {
    switch (entry->type) {
      case Png:
        init_from_mem(entry->data);
        break;
      case Jpg:
        init_from_mem_jpeg(entry->data);
        break;
      case Webp:
        init_from_mem_webp(entry->data);
        break;
      case Gif:
        init_from_mem_gif(entry->data);
      default:
        break;
    }
  }
  void init_from_mem_webp(std::vector<unsigned char>& content) {
    if (valid)
      return;
    uint8_t* decoded = WebPDecodeRGBA(&content[0], content.size(), (int*)&width,
                                      (int*)&height);
    data.insert(data.begin(), decoded, decoded + (width * height * 4));
    delete decoded;
    init();
  }

  void init_from_mem(std::vector<unsigned char>& content) {
    if (valid)
      return;
    unsigned error = lodepng::decode(data, width, height, content);
    init();
  }

  void init_from_mem_gif(std::vector<unsigned char>& content) {
    gd_GIF* gif = gd_open_gif(content.data(), content.size());
    width = gif->width;
    height = gif->height;
    uint8_t* pixel_buff = new uint8_t[width * height * 3];
    gd_get_frame(gif);
    gd_render_frame(gif, pixel_buff);
    uint8_t* color = pixel_buff;
    data.resize(width * height * 4);
    uint8_t* ptr = data.data();

    for (int i = 0; i < gif->height; i++) {
      for (int j = 0; j < gif->width; j++) {
        if (!gd_is_bgcolor(gif, color)) {
          *(ptr++) = color[0];
          *(ptr++) = color[1];
          *(ptr++) = color[2];
          *(ptr++) = 255;
        } else if (((i >> 2) + (j >> 2)) & 1) {
          *(ptr++) = 0x7F;
          *(ptr++) = 0x7F;
          *(ptr++) = 0x7F;
          *(ptr++) = 255;
        } else {
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 255;
        }
        color += 3;
      }
    }
    gd_close_gif(gif);
    delete[] pixel_buff;
    init();
  }

  void init_from_mem_jpeg(std::vector<unsigned char>& content) {
    int rc;
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, content.data(), content.size());

    rc = jpeg_read_header(&cinfo, TRUE);

    jpeg_start_decompress(&cinfo);
    width = cinfo.output_width;
    height = cinfo.output_height;
    int pixel_size = cinfo.output_components;
    if (pixel_size != 3) {
      std::cout << "jpeg pixel size not 3??? " << pixel_size << "\n";
    }

    uint8_t* pixel_buff = new uint8_t[height * width * pixel_size];

    int row_stride = width * pixel_size;

    while (cinfo.output_scanline < cinfo.output_height) {
      uint8_t* dest_buff[1];
      dest_buff[0] = pixel_buff + (cinfo.output_scanline * row_stride);
      jpeg_read_scanlines(&cinfo, dest_buff, 1);
    }

    data.resize(width * height * 4);

    jpeg_finish_decompress(&cinfo);

    uint8_t* ptr = data.data();
    uint8_t* src = pixel_buff;
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        *(ptr++) = *(src++);
        *(ptr++) = *(src++);
        *(ptr++) = *(src++);
        *(ptr++) = 255;
      }
    }

    jpeg_destroy_decompress(&cinfo);
    delete[] pixel_buff;
    init();
  }

  void render(int x, int y, float scale) {
    m_shader = AppState::gState->opengl_state.image_shader;
    SimpleEntry entry = {vec2f(x, y), vec2f(width * scale, height * scale)};
    m_shader->shader.use();
    m_shader->bindVertexArray();
    m_shader->bindBuffer();
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SimpleEntry), &entry);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, 1);
  }
};
#endif