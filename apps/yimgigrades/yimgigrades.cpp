//
// LICENSE:
//
// Copyright (c) 2016 -- 2020 Fabio Pellacini
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#include <yocto/yocto_commonio.h>
#include <yocto/yocto_image.h>
#include <yocto_grade/yocto_grade.h>
#include <yocto_gui/yocto_gui.h>
using namespace yocto::math;
namespace cli = yocto::commonio;
namespace img = yocto::image;
namespace grd = yocto::grade;
namespace gui = yocto::gui;

#include <future>
using namespace std::string_literals;

struct app_state {
  // original data
  std::string filename = "image.png";
  std::string outname  = "out.png";

  // image data
  img::image<vec4f> source = {};

  // diplay data
  img::image<vec4f> display = {};
  grd::grade_params params  = {};

  // viewing properties
  gui::image*       glimage  = new gui::image{};
  gui::image_params glparams = {};

  ~app_state() {
    if (glimage) delete glimage;
  }
};

void update_display(app_state* app) {
  if (app->display.size() != app->source.size()) app->display = app->source;
  app->display = grd::grade_image(app->source, app->params);
}

int main(int argc, const char* argv[]) {
  // prepare application
  auto app_guard = std::make_unique<app_state>();
  auto app       = app_guard.get();
  auto filenames = std::vector<std::string>{};

  // command line options
  auto cli = cli::make_cli("yimgigrades", "view images");
  add_option(cli, "--output,-o", app->outname, "image output");
  add_option(cli, "image", app->filename, "image filename", true);
  parse_cli(cli, argc, argv);

  // load image
  auto ioerror = ""s;
  if (!load_image(app->filename, app->source, ioerror)) {
    cli::print_fatal(ioerror);
    return 1;
  }

  // update display
  update_display(app);

  // callbacks
  auto callbacks    = gui::ui_callbacks{};
  callbacks.draw_cb = [app](gui::window* win, const gui::input& input) {
    app->glparams.window      = input.window_size;
    app->glparams.framebuffer = input.framebuffer_viewport;
    if (!is_initialized(app->glimage)) {
      init_image(app->glimage);
      set_image(app->glimage, app->display, false, false);
    }
    update_imview(app->glparams.center, app->glparams.scale,
        app->display.size(), app->glparams.window, app->glparams.fit);
    draw_image(app->glimage, app->glparams);
  };
  callbacks.widgets_cb = [app](gui::window* win, const gui::input& input) {
    auto edited = 0;
    if (gui::begin_header(win, "grade")) {
      auto& params = app->params;
      edited += draw_slider(win, "exposure", params.exposure, -5, 5);
      edited += draw_checkbox(win, "filmic", params.filmic);
      continue_line(win);
      edited += draw_checkbox(win, "srgb", params.srgb);
      edited += draw_coloredit(win, "tint", params.tint);
      edited += draw_slider(win, "contrast", params.contrast, 0, 1);
      edited += draw_slider(win, "saturation", params.saturation, 0, 1);
      edited += draw_slider(win, "vignette", params.vignette, 0, 1);
      edited += draw_slider(win, "grain", params.grain, 0, 1);
      edited += draw_slider(win, "mosaic", params.mosaic, 0, 64);
      edited += draw_slider(win, "grid", params.grid, 0, 64);
      gui::end_header(win);
    }
    if(gui::begin_header(win, "low poly"))
    {
        auto& params = app->params;
        edited += draw_checkbox(win, "low poly", params.lowpoly);
        edited += draw_slider(win, "edge threshold", params.edge_threshold, 0, 1);
        edited += draw_slider(win, "edge probability", params.edge_p, 0.f, 0.5f);
        edited += draw_slider(win, "not edge probability", params.not_edge_p, 0.f, 0.1f);
        edited += draw_checkbox(win, "draw triangles", params.draw_triangles);
        
    }
    if (begin_header(win, "inspect")) {
      draw_slider(win, "zoom", app->glparams.scale, 0.1, 10);
      draw_checkbox(win, "fit", app->glparams.fit);
      auto ij = get_image_coords(input.mouse_pos, app->glparams.center,
          app->glparams.scale, app->source.size());
      draw_dragger(win, "mouse", ij);
      auto img_pixel = zero4f, display_pixel = zero4f;
      if (ij.x >= 0 && ij.x < app->source.size().x && ij.y >= 0 &&
          ij.y < app->source.size().y) {
        img_pixel     = app->source[{ij.x, ij.y}];
        display_pixel = app->display[{ij.x, ij.y}];
      }
      draw_coloredit(win, "image", img_pixel);
      draw_coloredit(win, "display", display_pixel);
      end_header(win);
    }
    if (edited) {
      update_display(app);
      if (!is_initialized(app->glimage)) init_image(app->glimage);
      set_image(app->glimage, app->display, false, false);
    }
  };
  callbacks.uiupdate_cb = [app](gui::window* win, const gui::input& input) {
    // handle mouse
    if (input.mouse_left && !input.widgets_active) {
      app->glparams.center += input.mouse_pos - input.mouse_last;
    }
    if (input.mouse_right && !input.widgets_active) {
      app->glparams.scale *= powf(
          2, (input.mouse_pos.x - input.mouse_last.x) * 0.001f);
    }
  };

  // run ui
  run_ui({1280, 720}, "yimggrades", callbacks);

  // done
  return 0;
}
