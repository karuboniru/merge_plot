#include <TArrow.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TGraphAsymmErrors.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <TLegend.h>
#include <TMarker.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TText.h>
#include <algorithm>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <print>
#include <ranges>
#include <string>
#include <tools.h>
#include <type_traits>
#include <unordered_map>

double median(TH1D *hist) {
  double x{}, q{};
  q = 0.5;                 // 0.5 for "median"
  hist->ComputeIntegral(); // just a precaution
  hist->GetQuantiles(1, &x, &q);
  return x;
}

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename Type, typename... Types, typename T, typename F>
auto my_visit(T *ptr, F func)
  requires((std::is_base_of_v<T, Type>))
{
  if (auto *casted = dynamic_cast<Type *>(ptr)) {
    return func(casted);
  }
  if constexpr (sizeof...(Types) > 0) {
    return my_visit<Types...>(ptr, func);
  }
  __builtin_unreachable();
}

void overE(TH1 *hist) {
  for (int i = 1; i <= hist->GetNbinsX(); ++i) {
    double content = hist->GetBinContent(i);
    double bin_center = hist->GetBinCenter(i);
    hist->SetBinContent(i, content / bin_center);
  }
}

void overE(TGraph *graph) {
  if (auto graph_asym = dynamic_cast<TGraphAsymmErrors *>(graph)) {
    for (int i = 0; i < graph_asym->GetN(); ++i) {
      double x{}, y{}, exl{}, exh{}, eyl{}, eyh{};
      graph_asym->GetPoint(i, x, y);
      exl = graph_asym->GetErrorXlow(i);
      exh = graph_asym->GetErrorXhigh(i);
      eyl = graph_asym->GetErrorYlow(i);
      eyh = graph_asym->GetErrorYhigh(i);
      graph_asym->SetPoint(i, x, y / x);
      graph_asym->SetPointError(i, exl, exh, eyl / x, eyh / x);
    }
    return;
  }
  if (auto graph_err = dynamic_cast<TGraphErrors *>(graph)) {
    for (int i = 0; i < graph_err->GetN(); ++i) {
      double x{}, y{}, ex{}, ey{};
      graph_err->GetPoint(i, x, y);
      ex = graph_err->GetErrorX(i);
      ey = graph_err->GetErrorY(i);
      graph_err->SetPoint(i, x, y / x);
      graph_err->SetPointError(i, ex / x, ey / x);
    }
    return;
  }
  for (int i = 0; i < graph->GetN(); ++i) {
    double x{}, y{};
    graph->GetPoint(i, x, y);
    graph->SetPoint(i, x, y / x);
  }
}

int main(int argc, char const *argv[]) {
  {
    gStyle->SetOptStat(0);
    gSystem->ResetSignal(kSigBus);
    gSystem->ResetSignal(kSigSegmentationViolation);
    gSystem->ResetSignal(kSigIllegalInstruction);
    TH1::AddDirectory(kFALSE);
    if (argc != 2) {
      std::cout << "Usage: merge_plot <config_file>" << '\n';
      return 1;
    }
  }
  using entry_t = std::tuple<std::string, std::unique_ptr<TNamed>, std::string,
                             std::string, bool>;

  std::deque<entry_t> entries{};
  std::vector<std::pair<std::unique_ptr<TObject>, std::string>> objects{};
  // std::vector<std::string> append_opt{}, leg_opt{};
  nlohmann::json config;
  constexpr std::array<int, 10> col{kRed,   kBlue, kViolet, kYellow, kOrange,
                                    kGreen, kGray, kTeal,   kPink};

  {
    std::ifstream config_file{argv[1]};
    if (!config_file.is_open()) {
      std::cout << "Error: Could not open config file" << '\n';
      return 1;
    }
    config_file >> config;
  }
  auto max_conf = config.value<double>("max", 0.);
  double max{};
  bool ymin_set = config.contains("min_y_value");
  double min_y_value = ymin_set ? config["min_y_value"].get<double>() : 0;
  max = config.value<double>("max_y_value", 0.);
  bool last_bin_as_overflow = config.value("last_bin_as_overflow", false);
  bool add_median = config.value("add_median", false);
  bool overE_flag = config.value("overE", false);
  double left_margin_scale = config.value("left_margin_scale", 1.0);
  double y_axis_title_offset_scale =
      config.value("y_axis_title_offset_scale", 1.);
  bool scale_to_first = config.value("scale_to_first", false);
  double first_integral = 1.0;
  auto shape_global = config.value("shape", false);
  std::vector<TNamed *> form_bar{};
  double uplimit{-INFINITY};
  {
    auto plot_title = config["plot_title"].get<std::string>();

    size_t i{};
    for (const auto &[id, entry] : config["hists"] | std::views::enumerate) {
      // if (entry.value("skip", false)) {
      //   continue;
      // }
      std::string legend = entry["legend"];
      std::string file_path = entry["file_path"];
      std::string plot_name = entry["hist"];
      double scale = entry.value("scale", 1.);
      auto rebin_factor = entry.value<size_t>("rebin", 1);
      TFile file{file_path.c_str(), "READ"};
      if (!file.IsOpen()) {
        std::cout << "Error: Could not open file " << file_path << '\n';
        return 1;
      }
      std::unique_ptr<TH1> hist{
          dynamic_cast<TH1 *>(file.Get(plot_name.c_str()))};
      if (!hist) {
        std::cout << "Error: Could not get histogram " << plot_name
                  << " from file " << file_path << '\n';
        return 1;
      }
      if (entry.value("overE", overE_flag)) {
        overE(hist.get());
      }
      if (scale_to_first) {
        if (id == 0) {
          first_integral = hist->Integral();
        }
        // scale /= first_integral;
        hist->Scale(1 / first_integral, "WIDTH");
      }
      // if (scale_to_first )
      hist->Scale(scale / rebin_factor);
      if (rebin_factor != 1)
        hist->Rebin(rebin_factor);
      if (entry.value("shape", shape_global)) {
        hist->Scale(1. / hist->Integral());
      }
      if (entry.value("cdf", false)) {
        hist.reset(hist->GetCumulative());
      }
      if (config.contains("max_x_value"))
        uplimit = std::max(uplimit, config["max_x_value"].get<double>());
      else
        uplimit = std::max(uplimit, hist->GetXaxis()->GetXmax());
      auto hist_2dptr = dynamic_cast<TH2 *>(hist.get());
      if (hist_2dptr) { // if a 2d plot
        hist_2dptr->SetAxisRange(config["rangex"][0], config["rangex"][1], "X");
        hist_2dptr->SetAxisRange(config["rangey"][0], config["rangey"][1], "Y");
        if (entry.value("normalize", -1) != -1) {
          hist =
              normalize_slice(hist_2dptr, entry["normalize"].get<int>() != 0);
        }
      } else {
        if (last_bin_as_overflow) {
          std::cout << "last bin as overflow" << '\n'
                    << "before \t" << hist->GetBinContent(hist->GetNbinsX())
                    << '\n';
          hist->SetBinContent(hist->GetNbinsX(),
                              hist->GetBinContent(hist->GetNbinsX()) +
                                  hist->GetBinContent(hist->GetNbinsX() + 1));
          std::cout << "after \t" << hist->GetBinContent(hist->GetNbinsX());
          hist->SetBinContent(hist->GetNbinsX() + 1, 0);
        } else {
          hist->GetXaxis()->SetRangeUser(hist->GetBinLowEdge(1), uplimit);
        }
        if (config.contains("min_x_value") && entries.empty()) {
          hist->GetXaxis()->SetRangeUser(config["min_x_value"].get<double>(),
                                         uplimit);
        }
      }
      std::cout << "plotting " << plot_name << " from file " << file_path
                << " scale " << scale << " rebin with " << rebin_factor << '\n';
      if (!config.contains("max_y_value"))
        max = std::max(max, hist->GetMaximum());
      ResetStyle(hist);
      hist->SetLineStyle(entry.value("line_style", kSolid));
      hist->SetLineWidth(entry.value("line_width", 2));
      hist->SetMarkerStyle(entry.value("marker_style", hist->GetMarkerStyle()));
      hist->SetMarkerSize(entry.value("marker_size", hist->GetMarkerSize()));
      auto color = entry.value("line_color", -1);
      if (color != -1)
        hist->SetLineColor(color);
      else
        hist->SetLineColor(col[i++]);
      hist->SetMarkerColor(hist->GetLineColor());
      hist->SetTitle(plot_title.c_str());
      if (entry.value("add2bar", false)) {
        form_bar.emplace_back(hist.get());
      }
      entries.emplace_back(
          legend, std::move(hist), entry.value("append_opt", ""),
          entry.value("leg_opt", "lpf"), entry.value("skip", false));
    }

    for (const auto &entry : config["graphs"]) {
      // if (entry.value("skip", false)) {
      //   continue;
      // }
      std::string legend = entry["legend"];
      std::string file_path = entry["file_path"];
      std::string plot_name = entry["graph"];
      double scale = entry.value("scale", 1.);
      TFile file{file_path.c_str(), "READ"};
      if (!file.IsOpen()) {
        std::cout << "Error: Could not open file " << file_path << '\n';
        return 1;
      }
      std::unique_ptr<TGraph> graph{
          dynamic_cast<TGraph *>(file.Get(plot_name.c_str()))};
      if (!graph) {
        std::cout << "Error: Could not get graph " << plot_name << " from file "
                  << file_path << '\n';
        return 1;
      }
      if (entry.value("overE", overE_flag)) {
        overE(graph.get());
      }
      if (config.contains("max_x_value"))
        uplimit = std::max(uplimit, config["max_x_value"].get<double>());
      else
        uplimit = std::max(uplimit, graph->GetXaxis()->GetXmax());
      if (!config.contains("max_y_value"))
        max = std::max(max, graph->GetHistogram()->GetMaximum());
      graph->GetXaxis()->SetRangeUser(graph->GetXaxis()->GetXmin(), uplimit);
      graph->GetYaxis()->SetRangeUser(min_y_value, max);
      if (config.contains("min_x_value") && entries.empty()) {
        graph->GetXaxis()->SetRangeUser(config["min_x_value"].get<double>(),
                                        uplimit);
      }
      graph->Scale(scale);
      graph->SetLineStyle(entry.value("line_style", kSolid));
      graph->SetLineWidth(entry.value("line_width", 2));
      auto color = entry.value("line_color", -1);
      if (color != -1)
        graph->SetLineColor(color);
      else
        graph->SetLineColor(col[i++]);
      // graph->SetMarkerColor(graph->GetLineColor());
      graph->SetMarkerColor(entry.value("marker_color", graph->GetLineColor()));
      graph->SetMarkerStyle(
          entry.value("marker_style", graph->GetMarkerStyle()));
      graph->SetMarkerSize(entry.value("marker_size", graph->GetMarkerSize()));
      graph->SetTitle(plot_title.c_str());
      if (entry.value("add2bar", false)) {
        form_bar.emplace_back(graph.get());
      }
      entries.emplace_back(
          legend, std::move(graph), entry.value("append_opt", ""),
          entry.value("leg_opt", "lpf"), entry.value("skip", false));
      std::cout << "plotting " << plot_name << " from file " << file_path
                << " scale " << scale << '\n';
    }

    if (max_conf != 0) {
      // double oldmax = max;
      double scale_conf = log10(max_conf);
      double scale = log10(max);
      int scale_diff = scale - scale_conf;
      max = max_conf * pow(10, scale_diff);
    } else {
      max *= 1.1;
    }
    if (config.contains("misc")) {
      const std::unordered_map<
          std::string,
          std::function<std::unique_ptr<TObject>(const nlohmann::json &)>>
          misc_objects_maker{
              {"TLine",
               [](const nlohmann::json &config) -> std::unique_ptr<TObject> {
                 auto line = std::make_unique<TLine>(
                     config["x1"], config["y1"], config["x2"], config["y2"]);
                 line->SetLineColor(config.value("color", kBlack));
                 line->SetLineWidth(config.value("width", 1));
                 line->SetLineStyle(
                     config.value("style", line->GetLineStyle()));
                 return line;
               }},
              {"VLine",
               [&max](
                   const nlohmann::json &config) -> std::unique_ptr<TObject> {
                 auto line =
                     std::make_unique<TLine>(config["x"], 0, config["x"], max);
                 line->SetLineColor(config.value("color", kBlack));
                 line->SetLineWidth(config.value("width", 1));
                 line->SetLineStyle(
                     config.value("style", line->GetLineStyle()));
                 return line;
               }},
              {"HLine",
               [&uplimit](
                   const nlohmann::json &config) -> std::unique_ptr<TObject> {
                 auto line = std::make_unique<TLine>(0, config["y"], uplimit,
                                                     config["y"]);
                 line->SetLineColor(config.value("color", kBlack));
                 line->SetLineWidth(config.value("width", 1));
                 line->SetLineStyle(
                     config.value("style", line->GetLineStyle()));
                 return line;
               }},
              {"TMarker",
               [](const nlohmann::json &config) -> std::unique_ptr<TObject> {
                 auto marker = std::make_unique<TMarker>(
                     config["x"], config["y"], config.value("marker", 20));
                 marker->SetMarkerColor(config.value("color", kBlack));
                 marker->SetMarkerSize(config.value("size", 1));
                 marker->SetNDC(config.value("ndc", true));
                 return marker;
               }},
              {"TText",
               [](const nlohmann::json &config) -> std::unique_ptr<TObject> {
                 auto text = std::make_unique<TText>(
                     config["x"], config["y"],
                     config["text"].get<std::string>().c_str());
                 text->SetTextSize(config.value("size", 0.05));
                 text->SetTextColor(config.value("color", kBlack));
                 text->SetTextFont(config.value("font", 42));
                 text->SetTextAlign(config.value("align", 22));
                 text->SetNDC(config.value("ndc", true));
                 return text;
               }},
              {"TLatex",
               [](const nlohmann::json &config) -> std::unique_ptr<TObject> {
                 auto latex = std::make_unique<TLatex>(
                     config["x"], config["y"],
                     config["text"].get<std::string>().c_str());
                 latex->SetTextSize(config.value("size", 0.05));
                 latex->SetTextColor(config.value("color", kBlack));
                 latex->SetTextFont(config.value("font", 42));
                 latex->SetTextAlign(config.value("align", 22));
                 latex->SetNDC(config.value("ndc", true));
                 return latex;
               }},
              {"TArrow",
               [](const nlohmann::json &config) -> std::unique_ptr<TObject> {
                 auto arrow = std::make_unique<TArrow>(
                     config["x1"], config["y1"], config["x2"], config["y2"],
                     config.value("arrowsize", 0.1),
                     config.value("arrowstyle", ">").c_str());
                 arrow->SetLineColor(config.value("color", kBlack));
                 arrow->SetLineWidth(config.value("width", 1));
                 arrow->SetLineStyle(
                     config.value("style", arrow->GetLineStyle()));
                 arrow->SetNDC(config.value("ndc", true));
                 return arrow;
               }}};

      for (const auto &entry : config["misc"]) {
        auto title = entry.value("legend", "");
        objects.emplace_back(
            misc_objects_maker.at(entry["type"].get<std::string>())(entry),
            title);
      }
    }
  }

  {
    if (form_bar.size() >= 2) {
      auto npoints = my_visit<TGraph, TH1>(
          form_bar[0], overloaded{[](TGraph *g) { return g->GetN(); },
                                  [](TH1 *h) { return h->GetNbinsX(); }});
      auto xmax_user = my_visit<TGraph, TH1>(
          form_bar[0], [](auto *g) { return g->GetXaxis()->GetXmax(); });
      if (config.contains("max_x_value"))
        xmax_user = config["max_x_value"].get<double>();
      auto xmin_user = my_visit<TGraph, TH1>(
          form_bar[0], [](auto *g) { return g->GetXaxis()->GetXmin(); });
      if (config.contains("min_x_value"))
        xmin_user = config["min_x_value"].get<double>();
      auto x_axis_title = my_visit<TGraph, TH1>(
          form_bar[0], [](auto *g) { return g->GetXaxis()->GetTitle(); });
      auto y_axis_title = my_visit<TGraph, TH1>(
          form_bar[0], [](auto *g) { return g->GetYaxis()->GetTitle(); });
      auto grerr = std::make_unique<TGraphErrors>(npoints);
      for (size_t i = 0; i < npoints; ++i) {
        auto x_value = my_visit<TGraph, TH1>(
            form_bar[0],
            overloaded{[i](TGraph *g) {
                         double x{}, y{};
                         g->GetPoint(i, x, y);
                         return x;
                       },
                       [i](TH1 *h) { return h->GetBinCenter(i + 1); }});
        auto vals =
            form_bar | std::views::transform([&](auto ptr) {
              return my_visit<TGraph, TH1>(
                  ptr,
                  overloaded{[&](TH1 *h) { return h->Interpolate(x_value); },
                             [&](TGraph *h) { return h->Eval(x_value); }});
            }) |
            std::ranges::to<std::vector>();
        auto max = std::ranges::max(vals);
        auto min = std::ranges::min(vals);
        auto mean = (max + min) / 2;
        auto err = (max - min) / 2;
        grerr->SetPoint(i, x_value, mean);
        grerr->SetPointError(i, 0, err);
      }
      ResetStyle(grerr.get());
      grerr->GetXaxis()->SetTitle(x_axis_title);
      grerr->GetYaxis()->SetTitle(y_axis_title);
      std::println("range user {} {}", xmin_user, xmax_user);
      grerr->GetXaxis()->SetRangeUser(xmin_user, xmax_user);
      grerr->SetFillColor(kGray);
      grerr->SetMarkerColor(kGray);
      grerr->SetLineColor(kGray);
      grerr->SetTitle("");
      std::println("Finished drawing error band with {} inputs",
                   form_bar.size());
      entries.emplace_front(config.value("band_title", "Range"),
                            std::move(grerr), "AP E3", "f", false);
    }
  }

  {
    auto leg =
        config.contains("legend_place")
            ? std::make_unique<TLegend>(
                  config["legend_place"]["x1"], config["legend_place"]["y1"],
                  config["legend_place"]["x2"], config["legend_place"]["y2"],
                  config.value("legend_header", "").c_str())
            : std::make_unique<TLegend>(.7, .7, .9, .9);
    ResetStyle(leg);
    leg->SetNColumns(config.value("legend_columns", 1));
    auto cx = config.value("canvas_x", 600);
    auto cy = config.value("canvas_y", 400);
    auto canvas = getCanvas("", cx, cy);
    canvas->SetLeftMargin(left_margin_scale * canvas->GetLeftMargin());
    if (config.value("logy", false))
      canvas->SetLogy();
    if (config.value("logz", false))
      canvas->SetLogz();
    const auto draw_opt = config.value("draw_opt", "hist C");

    for (auto &&[i, entry] : entries | std::views::enumerate) {
      auto &[legend_title, hist, append_opt, leg_opt, skip] = entry;
      if (skip)
        continue;
      if (auto hist_casted = dynamic_cast<TH1D *>(hist.get());
          add_median && hist_casted) {
        auto median_value = median(hist_casted);
        auto median_line =
            std::make_unique<TLine>(median_value, 0, median_value, max);
        median_line->SetLineColor(hist_casted->GetLineColor());
        median_line->SetLineStyle(2);
        objects.emplace_back(std::move(median_line), "");
        std::stringstream ss{};
        ss << "("
           << "m = " << std::fixed << std::setprecision(2) << median_value
           << ")";
        legend_title += ss.str();
      }
      const auto opt = draw_opt + " same";
      std::string allopt = opt + append_opt;
      my_visit<TGraph, TH1>(hist.get(), [&](auto h) {
        h->SetMaximum(max);
        if (ymin_set) {
          h->SetMinimum(min_y_value);
        }
        h->GetYaxis()->SetTitleOffset(y_axis_title_offset_scale *
                                      h->GetYaxis()->GetTitleOffset());
      });
      if (i == 0) {
        hist->Draw(draw_opt.c_str());
        for (const auto &[plottable, name] : objects) {
          // object.first->Draw("same");
          plottable->Draw("same");
          if (!name.empty()) {
            leg->AddEntry(plottable.get(), name.c_str(), "l");
          }
        }
      }
      if (!legend_title.empty())
        leg->AddEntry(hist.get(), legend_title.c_str(), leg_opt.c_str());

      hist->Draw(opt.c_str());
    }

    leg->SetTextSize(config.value("legend_text_size", leg->GetTextSize()));
    leg->Draw(config.value("legend_draw_opt", "").c_str());

    for (const std::string output_name : config["output_names"]) {
      if (!std::filesystem::path(output_name).parent_path().empty())
        std::filesystem::create_directories(
            std::filesystem::path(output_name).parent_path());
      canvas->SaveAs(output_name.c_str());
    }
  }

  return 0;
}
