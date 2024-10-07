#pragma once
class dwm_render
{
public:
  dwm_render();

  bool init();

  void render();

private:
  void* commer_;
  void* render_buffer_;
  size_t render_buffer_size_;
};

