#pragma once

#include <TF1.h>
#include <TFile.h>
#include <TSpline.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <ranges>

struct spline_params {
  double x1{}, y1{}, x2{}, y2{}, factor{};
  int leg_off{};
  std::string root_input, out_title, extra_text;
  std::vector<std::string> plot_names;
};

inline spline_params read_options(int argc, char **argv) {
  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()("x1", po::value<double>()->required(),
                     "x1")("y1", po::value<double>()->required(),
                           "y1")("x2", po::value<double>()->required(), "x2")(
      "y2", po::value<double>()->required(),
      "y2")("factor", po::value<double>()->required(),
            "factor")("leg-off", po::value<int>()->required(), "leg_off")(
      "root-input", po::value<std::string>()->required(), "root_input")(
      "out-title", po::value<std::string>()->required(), "out_title")(
      "extra-text", po::value<std::string>()->required(), "extra_text")(
      "plot-names", po::value<std::vector<std::string>>(), "plot-names");
  po::variables_map vm;
  po::positional_options_description p;
  p.add("plot-names", -1);
  try {
    po::store(
        po::command_line_parser(argc, argv).options(desc).positional(p).run(),
        vm);
    po::notify(vm);
    spline_params params;
    params.x1 = vm["x1"].as<double>();
    params.y1 = vm["y1"].as<double>();
    params.x2 = vm["x2"].as<double>();
    params.y2 = vm["y2"].as<double>();
    params.factor = vm["factor"].as<double>();
    params.leg_off = vm["leg-off"].as<int>();
    params.root_input = vm["root-input"].as<std::string>();
    params.out_title = vm["out-title"].as<std::string>();
    params.extra_text = vm["extra-text"].as<std::string>();
    params.plot_names = vm["plot-names"].as<std::vector<std::string>>();
    return params;
  } catch (const po::error &e) {
    std::cerr << e.what() << '\n';
    std::cerr << desc << '\n';
    std::exit(1);
  }
}
constexpr double min = 0.1, max = 100;

const std::vector<std::string> interactions_full{
    "nu_e_C12", "nu_e_bar_C12", "nu_mu_C12", "nu_mu_bar_C12",
    "nu_e_H1",  "nu_e_bar_H1",  "nu_mu_H1",  "nu_mu_bar_H1"};

auto inline get_spline_sum(TFile *file,
                           const std::vector<std::string> &channels,
                           std::string interaction) {
  double factor = interaction.contains("C12") ? 12. : 1.;
  return TF1{
      interaction.c_str(),
      [spline_vec =
           channels | std::views::transform([&](const std::string &channel) {
             auto path = interaction + "/" + channel;
             auto xsec_graph = dynamic_cast<TGraph *>(file->Get(path.c_str()));
             if (!xsec_graph) {
               std::cerr << "Error: Cannot find spline " << path
                         << " this might be expect on QEL channel" << std::endl;
               return std::optional<TSpline3>{std::nullopt};
             }
             return std::optional<TSpline3>{std::in_place, path.c_str(),
                                            xsec_graph};
           }) |
           std::views::filter(
               [](auto &&spline) -> bool { return spline.has_value(); }) |
           std::ranges::to<std::vector>(),
       factor](const double *x, const double *) {
        return std::ranges::fold_left(
            spline_vec | std::views::transform([&, factor](auto &&spline) {
              return spline.value().Eval(x[0]) / x[0] / factor;
            }),
            0.0, std::plus<double>{});
      },
      min, max, 0};
}

double get_maxmium(TFile *file, const std::vector<std::string> &channels) {
  return std::ranges::max(
      interactions_full |
      std::views::transform([&](const std::string &interaction) {
        return get_spline_sum(file, channels, interaction).GetMaximum();
      }));
}