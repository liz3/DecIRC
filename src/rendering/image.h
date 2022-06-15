#ifndef DEC_IMAGE
#define DEC_IMAGE
#include <string>
#include <vector>
#include "../../third-party/png/lodepng.h"
#include "../../third-party/libwebp/src/webp/decode.h"
#include "../../third-party/libjpeg/jpeglib.h"

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

    jpeg_finish_decompress(&cinfo);

    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        uint32_t offset = (y * width + x) * 3;
        data.push_back(pixel_buff[offset]);
        data.push_back(pixel_buff[offset + 1]);
        data.push_back(pixel_buff[offset + 2]);
        data.push_back(255);
      }
    }

    jpeg_destroy_decompress(&cinfo);
    delete[] pixel_buff;

    init();
  }

  void render(int x, int y, float scale) {
    m_shader = AppState::gState->opengl_state.image_shader;
    std::vector<SimpleEntry> selectionBoundaries = {
        {vec2f(x, y), vec2f(width * scale, height * scale)}};
    m_shader->shader.use();
    m_shader->bindVertexArray();
    m_shader->bindBuffer();
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    sizeof(SimpleEntry) * selectionBoundaries.size(),
                    &selectionBoundaries[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6,
                          (GLsizei)selectionBoundaries.size());
  }
};
#endif