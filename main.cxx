#include <TArrow.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TGraphAsymmErrors.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <TLine.h>
#include <TMarker.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TText.h>
#include <algorithm>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <print>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tools.h>
#include <type_traits>

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

// ---------------------------------------------------------------------------
// JSON config structs
// ---------------------------------------------------------------------------

namespace nlohmann {
template <typename T> struct adl_serializer<std::optional<T>> {
  static void from_json(const json &j, std::optional<T> &opt) {
    if (j.is_null())
      opt = std::nullopt;
    else
      opt = j.get<T>();
  }
  static void to_json(json &j, const std::optional<T> &opt) {
    if (opt)
      j = *opt;
    else
      j = nullptr;
  }
};
} // namespace nlohmann

struct LegendPlace {
  double x1 = 0.7, y1 = 0.7, x2 = 0.9, y2 = 0.9;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LegendPlace, x1, y1, x2, y2)

// Per-histogram entry from the "hists" array
struct HistConfig {
  std::string legend;
  std::string file_path;
  std::string hist;
  double scale = 1.;
  std::size_t rebin = 1;
  std::optional<bool> overE;  // falls back to global PlotConfig::overE
  std::optional<bool> shape;  // falls back to global PlotConfig::shape
  bool cdf = false;
  int normalize = -1;         // -1 = no normalization (for 2D slices)
  int line_style = kSolid;
  int line_width = 2;
  int line_color = -1;        // -1 = auto-assign from palette
  int marker_style = -1;      // -1 = use ROOT object default
  double marker_size = -1.;   // -1 = use ROOT object default
  std::string append_opt;
  std::string leg_opt = "lpf";
  bool skip = false;
  bool add2bar = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HistConfig, legend, file_path,
    hist, scale, rebin, overE, shape, cdf, normalize, line_style, line_width,
    line_color, marker_style, marker_size, append_opt, leg_opt, skip, add2bar)

// Per-graph entry from the "graphs" array
struct GraphConfig {
  std::string legend;
  std::string file_path;
  std::string graph;
  double scale = 1.;
  std::optional<bool> overE;  // falls back to global PlotConfig::overE
  int line_style = kSolid;
  int line_width = 2;
  int line_color = -1;        // -1 = auto-assign from palette
  int marker_color = -1;      // -1 = use line_color
  int marker_style = -1;      // -1 = use ROOT object default
  double marker_size = -1.;   // -1 = use ROOT object default
  std::string append_opt;
  std::string leg_opt = "lpf";
  bool skip = false;
  bool add2bar = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GraphConfig, legend, file_path,
    graph, scale, overE, line_style, line_width, line_color, marker_color,
    marker_style, marker_size, append_opt, leg_opt, skip, add2bar)

// One entry from the "misc" array (overlaid lines, markers, text, etc.)
// Not all fields apply to every type; absent fields stay at defaults.
struct MiscConfig {
  std::string type;
  std::string legend;
  // Positional — which ones are required depends on `type`
  std::optional<double> x, y, x1, y1, x2, y2;
  // Common style
  int color = kBlack;
  int width = 1;
  int style = -1;             // -1 = use ROOT object default
  // Text / TLatex
  std::string text;
  double size = 0.05;
  int font = 42;
  int align = 22;
  bool ndc = true;
  // TMarker
  int marker = 20;
  // TArrow
  double arrowsize = 0.1;
  std::string arrowstyle = ">";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MiscConfig, type, legend,
    x, y, x1, y1, x2, y2, color, width, style, text, size, font, align, ndc,
    marker, arrowsize, arrowstyle)

// Top-level plot configuration
struct PlotConfig {
  std::string plot_title;
  double max = 0.;
  double max_y_value = 0.;
  std::optional<double> min_y_value;
  std::optional<double> max_x_value;
  std::optional<double> min_x_value;
  bool last_bin_as_overflow = false;
  bool add_median = false;
  bool overE = false;
  double left_margin_scale = 1.;
  double y_axis_title_offset_scale = 1.;
  bool scale_to_first = false;
  bool shape = false;
  std::string band_title = "Range";
  std::string draw_opt = "hist C";
  bool logy = false;
  bool logz = false;
  int canvas_x = 600;
  int canvas_y = 400;
  int legend_columns = 1;
  std::string legend_header;
  double legend_text_size = -1.;  // -1 = use ROOT default
  std::string legend_draw_opt;
  std::optional<std::array<double, 2>> rangex;  // for 2D plots
  std::optional<std::array<double, 2>> rangey;  // for 2D plots
  std::vector<std::string> output_names;
  std::optional<LegendPlace> legend_place;
  std::vector<HistConfig> hists;
  std::vector<GraphConfig> graphs;
  std::vector<MiscConfig> misc;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PlotConfig, plot_title, max,
    max_y_value, min_y_value, max_x_value, min_x_value, last_bin_as_overflow,
    add_median, overE, left_margin_scale, y_axis_title_offset_scale,
    scale_to_first, shape, band_title, draw_opt, logy, logz, canvas_x, canvas_y,
    legend_columns, legend_header, legend_text_size, legend_draw_opt,
    rangex, rangey, output_names, legend_place, hists, graphs, misc)

// ---------------------------------------------------------------------------
// Misc object factory
// ---------------------------------------------------------------------------

std::unique_ptr<TObject> make_misc_object(const MiscConfig &m, double ymax,
                                          double xmax) {
  if (m.type == "TLine") {
    auto line = std::make_unique<TLine>(*m.x1, *m.y1, *m.x2, *m.y2);
    line->SetLineColor(m.color);
    line->SetLineWidth(m.width);
    if (m.style != -1)
      line->SetLineStyle(m.style);
    return line;
  }
  if (m.type == "VLine") {
    auto line = std::make_unique<TLine>(*m.x, 0, *m.x, ymax);
    line->SetLineColor(m.color);
    line->SetLineWidth(m.width);
    if (m.style != -1)
      line->SetLineStyle(m.style);
    return line;
  }
  if (m.type == "HLine") {
    auto line = std::make_unique<TLine>(0, *m.y, xmax, *m.y);
    line->SetLineColor(m.color);
    line->SetLineWidth(m.width);
    if (m.style != -1)
      line->SetLineStyle(m.style);
    return line;
  }
  if (m.type == "TMarker") {
    auto marker = std::make_unique<TMarker>(*m.x, *m.y, m.marker);
    marker->SetMarkerColor(m.color);
    marker->SetMarkerSize(m.size);
    marker->SetNDC(m.ndc);
    return marker;
  }
  if (m.type == "TText") {
    auto text = std::make_unique<TText>(*m.x, *m.y, m.text.c_str());
    text->SetTextSize(m.size);
    text->SetTextColor(m.color);
    text->SetTextFont(m.font);
    text->SetTextAlign(m.align);
    text->SetNDC(m.ndc);
    return text;
  }
  if (m.type == "TLatex") {
    auto latex = std::make_unique<TLatex>(*m.x, *m.y, m.text.c_str());
    latex->SetTextSize(m.size);
    latex->SetTextColor(m.color);
    latex->SetTextFont(m.font);
    latex->SetTextAlign(m.align);
    latex->SetNDC(m.ndc);
    return latex;
  }
  if (m.type == "TArrow") {
    auto arrow = std::make_unique<TArrow>(*m.x1, *m.y1, *m.x2, *m.y2,
                                          m.arrowsize, m.arrowstyle.c_str());
    arrow->SetLineColor(m.color);
    arrow->SetLineWidth(m.width);
    if (m.style != -1)
      arrow->SetLineStyle(m.style);
    arrow->SetNDC(m.ndc);
    return arrow;
  }
  throw std::runtime_error("Unknown misc type: " + m.type);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

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

  constexpr std::array<int, 10> col{kRed,   kBlue, kViolet, kYellow, kOrange,
                                    kGreen, kGray, kTeal,   kPink};

  PlotConfig cfg;
  {
    std::ifstream config_file{argv[1]};
    if (!config_file.is_open()) {
      std::cout << "Error: Could not open config file" << '\n';
      return 1;
    }
    nlohmann::json json_doc;
    config_file >> json_doc;
    cfg = json_doc.get<PlotConfig>();
  }

  double max = cfg.max_y_value;
  double uplimit{-INFINITY};
  double first_integral = 1.0;
  std::vector<TNamed *> form_bar{};

  size_t i{};  // shared color-palette index across hists and graphs

  for (const auto &[id, hcfg] : cfg.hists | std::views::enumerate) {
    TFile file{hcfg.file_path.c_str(), "READ"};
    if (!file.IsOpen()) {
      std::cout << "Error: Could not open file " << hcfg.file_path << '\n';
      return 1;
    }
    std::unique_ptr<TH1> hist{
        dynamic_cast<TH1 *>(file.Get(hcfg.hist.c_str()))};
    if (!hist) {
      std::cout << "Error: Could not get histogram " << hcfg.hist
                << " from file " << hcfg.file_path << '\n';
      return 1;
    }

    if (hcfg.overE.value_or(cfg.overE))
      overE(hist.get());

    if (cfg.scale_to_first) {
      if (id == 0)
        first_integral = hist->Integral();
      hist->Scale(1 / first_integral, "WIDTH");
    }
    hist->Scale(hcfg.scale / hcfg.rebin);
    if (hcfg.rebin != 1)
      hist->Rebin(hcfg.rebin);
    if (hcfg.shape.value_or(cfg.shape))
      hist->Scale(1. / hist->Integral());
    if (hcfg.cdf)
      hist.reset(hist->GetCumulative());

    if (cfg.max_x_value)
      uplimit = std::max(uplimit, *cfg.max_x_value);
    else
      uplimit = std::max(uplimit, hist->GetXaxis()->GetXmax());

    auto hist_2dptr = dynamic_cast<TH2 *>(hist.get());
    if (hist_2dptr) {
      if (cfg.rangex)
        hist_2dptr->SetAxisRange((*cfg.rangex)[0], (*cfg.rangex)[1], "X");
      if (cfg.rangey)
        hist_2dptr->SetAxisRange((*cfg.rangey)[0], (*cfg.rangey)[1], "Y");
      if (hcfg.normalize != -1)
        hist = normalize_slice(hist_2dptr, hcfg.normalize != 0);
    } else {
      if (cfg.last_bin_as_overflow) {
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
      if (cfg.min_x_value && entries.empty())
        hist->GetXaxis()->SetRangeUser(*cfg.min_x_value, uplimit);
    }

    std::cout << "plotting " << hcfg.hist << " from file " << hcfg.file_path
              << " scale " << hcfg.scale << " rebin with " << hcfg.rebin
              << '\n';

    if (!cfg.max_y_value)
      max = std::max(max, hist->GetMaximum());

    ResetStyle(hist);
    hist->SetLineStyle(hcfg.line_style);
    hist->SetLineWidth(hcfg.line_width);
    if (hcfg.marker_style != -1)
      hist->SetMarkerStyle(hcfg.marker_style);
    if (hcfg.marker_size >= 0.)
      hist->SetMarkerSize(hcfg.marker_size);
    hist->SetLineColor(hcfg.line_color != -1 ? hcfg.line_color : col[i++]);
    hist->SetMarkerColor(hist->GetLineColor());
    hist->SetTitle(cfg.plot_title.c_str());

    if (hcfg.add2bar)
      form_bar.emplace_back(hist.get());
    entries.emplace_back(hcfg.legend, std::move(hist), hcfg.append_opt,
                         hcfg.leg_opt, hcfg.skip);
  }

  for (const auto &gcfg : cfg.graphs) {
    TFile file{gcfg.file_path.c_str(), "READ"};
    if (!file.IsOpen()) {
      std::cout << "Error: Could not open file " << gcfg.file_path << '\n';
      return 1;
    }
    std::unique_ptr<TGraph> graph{
        dynamic_cast<TGraph *>(file.Get(gcfg.graph.c_str()))};
    if (!graph) {
      std::cout << "Error: Could not get graph " << gcfg.graph << " from file "
                << gcfg.file_path << '\n';
      return 1;
    }

    if (gcfg.overE.value_or(cfg.overE))
      overE(graph.get());

    if (cfg.max_x_value)
      uplimit = std::max(uplimit, *cfg.max_x_value);
    else
      uplimit = std::max(uplimit, graph->GetXaxis()->GetXmax());

    if (!cfg.max_y_value)
      max = std::max(max, graph->GetHistogram()->GetMaximum());

    graph->GetXaxis()->SetRangeUser(graph->GetXaxis()->GetXmin(), uplimit);
    graph->GetYaxis()->SetRangeUser(cfg.min_y_value.value_or(0.), max);

    if (cfg.min_x_value && entries.empty())
      graph->GetXaxis()->SetRangeUser(*cfg.min_x_value, uplimit);

    graph->Scale(gcfg.scale);
    graph->SetLineStyle(gcfg.line_style);
    graph->SetLineWidth(gcfg.line_width);
    graph->SetLineColor(gcfg.line_color != -1 ? gcfg.line_color : col[i++]);
    graph->SetMarkerColor(gcfg.marker_color != -1 ? gcfg.marker_color
                                                   : graph->GetLineColor());
    if (gcfg.marker_style != -1)
      graph->SetMarkerStyle(gcfg.marker_style);
    if (gcfg.marker_size >= 0.)
      graph->SetMarkerSize(gcfg.marker_size);
    graph->SetTitle(cfg.plot_title.c_str());

    if (gcfg.add2bar)
      form_bar.emplace_back(graph.get());
    entries.emplace_back(gcfg.legend, std::move(graph), gcfg.append_opt,
                         gcfg.leg_opt, gcfg.skip);
    std::cout << "plotting " << gcfg.graph << " from file " << gcfg.file_path
              << " scale " << gcfg.scale << '\n';
  }

  if (cfg.max != 0.) {
    double scale_conf = log10(cfg.max);
    double scale = log10(max);
    int scale_diff = scale - scale_conf;
    max = cfg.max * pow(10, scale_diff);
  } else {
    max *= 1.1;
  }

  for (const auto &mcfg : cfg.misc)
    objects.emplace_back(make_misc_object(mcfg, max, uplimit), mcfg.legend);

  {
    if (form_bar.size() >= 2) {
      auto npoints = my_visit<TGraph, TH1>(
          form_bar[0], overloaded{[](TGraph *g) { return g->GetN(); },
                                  [](TH1 *h) { return h->GetNbinsX(); }});
      auto xmax_user = my_visit<TGraph, TH1>(
          form_bar[0], [](auto *g) { return g->GetXaxis()->GetXmax(); });
      if (cfg.max_x_value)
        xmax_user = *cfg.max_x_value;
      auto xmin_user = my_visit<TGraph, TH1>(
          form_bar[0], [](auto *g) { return g->GetXaxis()->GetXmin(); });
      if (cfg.min_x_value)
        xmin_user = *cfg.min_x_value;
      auto x_axis_title = my_visit<TGraph, TH1>(
          form_bar[0], [](auto *g) { return g->GetXaxis()->GetTitle(); });
      auto y_axis_title = my_visit<TGraph, TH1>(
          form_bar[0], [](auto *g) { return g->GetYaxis()->GetTitle(); });
      auto grerr = std::make_unique<TGraphErrors>(npoints);
      for (size_t k = 0; k < npoints; ++k) {
        auto x_value = my_visit<TGraph, TH1>(
            form_bar[0],
            overloaded{[k](TGraph *g) {
                         double x{}, y{};
                         g->GetPoint(k, x, y);
                         return x;
                       },
                       [k](TH1 *h) { return h->GetBinCenter(k + 1); }});
        auto vals =
            form_bar | std::views::transform([&](auto ptr) {
              return my_visit<TGraph, TH1>(
                  ptr,
                  overloaded{[&](TH1 *h) { return h->Interpolate(x_value); },
                             [&](TGraph *h) { return h->Eval(x_value); }});
            }) |
            std::ranges::to<std::vector>();
        auto vmax = std::ranges::max(vals);
        auto vmin = std::ranges::min(vals);
        auto mean = (vmax + vmin) / 2;
        auto err = (vmax - vmin) / 2;
        grerr->SetPoint(k, x_value, mean);
        grerr->SetPointError(k, 0, err);
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
      entries.emplace_front(cfg.band_title, std::move(grerr), "AP E3", "f",
                            false);
    }
  }

  {
    auto leg = cfg.legend_place
                   ? std::make_unique<TLegend>(
                         cfg.legend_place->x1, cfg.legend_place->y1,
                         cfg.legend_place->x2, cfg.legend_place->y2,
                         cfg.legend_header.c_str())
                   : std::make_unique<TLegend>(.7, .7, .9, .9);
    ResetStyle(leg);
    leg->SetNColumns(cfg.legend_columns);
    auto canvas = getCanvas("", cfg.canvas_x, cfg.canvas_y);
    canvas->SetLeftMargin(cfg.left_margin_scale * canvas->GetLeftMargin());
    if (cfg.logy)
      canvas->SetLogy();
    if (cfg.logz)
      canvas->SetLogz();

    for (auto &&[idx, entry] : entries | std::views::enumerate) {
      auto &[legend_title, hist, append_opt, leg_opt, skip] = entry;
      if (skip)
        continue;
      if (auto hist_casted = dynamic_cast<TH1D *>(hist.get());
          cfg.add_median && hist_casted) {
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
      const auto opt = cfg.draw_opt + " same";
      std::string allopt = opt + append_opt;
      my_visit<TGraph, TH1>(hist.get(), [&](auto h) {
        h->SetMaximum(max);
        if (cfg.min_y_value)
          h->SetMinimum(*cfg.min_y_value);
        h->GetYaxis()->SetTitleOffset(cfg.y_axis_title_offset_scale *
                                      h->GetYaxis()->GetTitleOffset());
      });
      if (idx == 0) {
        hist->Draw(allopt.c_str());
        for (const auto &[plottable, name] : objects) {
          plottable->Draw("same");
          if (!name.empty())
            leg->AddEntry(plottable.get(), name.c_str(), "l");
        }
      }
      if (!legend_title.empty())
        leg->AddEntry(hist.get(), legend_title.c_str(), leg_opt.c_str());
      hist->Draw(allopt.c_str());
    }

    if (cfg.legend_text_size >= 0.)
      leg->SetTextSize(cfg.legend_text_size);
    leg->Draw(cfg.legend_draw_opt.c_str());

    for (const std::string &output_name : cfg.output_names) {
      if (!std::filesystem::path(output_name).parent_path().empty())
        std::filesystem::create_directories(
            std::filesystem::path(output_name).parent_path());
      canvas->SaveAs(output_name.c_str());
    }
  }

  return 0;
}
