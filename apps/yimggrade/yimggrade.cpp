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
#include <yocto/yocto_math.h>
#include <yocto_grade/yocto_grade.h>
using namespace yocto::math;
namespace cli = yocto::commonio;
namespace img = yocto::image;
namespace grd = yocto::grade;
using namespace std::string_literals;

#include "ext/filesystem.hpp"
namespace sfs = ghc::filesystem;

int main(int argc, const char* argv[]) {
  // command line parameters
  auto params   = grd::grade_params{};
  auto output   = "out.png"s;
  auto filename = "img.hdr"s;

  // parse command line
  auto cli = cli::make_cli("yimgproc", "Transform images");
  add_option(cli, "--exposure,-e", params.exposure, "Tonemap exposure");
  add_option(cli, "--filmic/--no-filmic,-f", params.filmic,
      "Tonemap uses filmic curve");
  add_option(cli, "--saturation,-s", params.saturation, "Grade saturation");
  add_option(cli, "--contrast,-c", params.contrast, "Grade contrast");
  add_option(cli, "--tint-red,-tr", params.tint.x, "Grade red tint");
  add_option(cli, "--tint-green,-tg", params.tint.y, "Grade green tint");
  add_option(cli, "--tint-blue,-tb", params.tint.z, "Grade blue tint");
  add_option(cli, "--vignette,-v", params.vignette, "Vignette radius");
  add_option(cli, "--grain,-g", params.grain, "Grain strength");
  add_option(cli, "--mosaic,-m", params.mosaic, "Mosaic size (pixels)");
  add_option(cli, "--grid,-G", params.grid, "Grid size (pixels)");
  add_option(cli, "--lowpoly/-l", params.lowpoly, "Low Polify image");
  add_option(cli, "--edge-threshold/-et", params.edge_threshold, "Edge threshold");
  add_option(cli, "--edge-p/-ep", params.edge_p, "Probability for an edge pixel to be choosed as vertex");
  add_option(cli, "--not-edge-p/-nep", params.not_edge_p, "Probability for an non edge pixel to be choosed as vertex");
  add_option(cli, "--draw-triangles/-dt", params.draw_triangles, "Draw triangles");
  add_option(cli, "--outimage,-o", output, "Output image filename", true);
  add_option(cli, "image", filename, "Input image filename", true);
  parse_cli(cli, argc, argv);

  // error buffer
  auto ioerror = ""s;

  // load
  auto img = img::image<vec4f>{};
  if (!load_image(filename, img, ioerror)) cli::print_fatal(ioerror);

  // corrections
  img = grd::grade_image(img, params);

  // save
  if (!save_image(output, float_to_byte(img), ioerror))
    cli::print_fatal(ioerror);

  // done
  return 0;
}
