#include <TArrow.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TLegend.h>
#include <TMarker.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TText.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <math.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <tools.h>
#include <unordered_map>

double median(TH1D *hist) {
  double x, q;
  q = 0.5;                 // 0.5 for "median"
  hist->ComputeIntegral(); // just a precaution
  hist->GetQuantiles(1, &x, &q);
  return x;
}

int main(int argc, char const *argv[]) {
  {
    gStyle->SetOptStat(0);
    gSystem->ResetSignal(kSigBus);
    gSystem->ResetSignal(kSigSegmentationViolation);
    gSystem->ResetSignal(kSigIllegalInstruction);
    TH1::AddDirectory(kFALSE);
    if (argc != 2) {
      std::cout << "Usage: merge_plot <config_file>" << std::endl;
      return 1;
    }
  }
  using entry_t = std::pair<std::string, std::unique_ptr<TH1>>;

  std::vector<entry_t> entries{};
  std::vector<std::unique_ptr<TObject>> objects{};
  std::vector<std::string> append_opt{}, leg_opt{};
  nlohmann::json config;
  constexpr std::array<int, 10> col{kRed,   kBlue, kViolet, kYellow, kOrange,
                                    kGreen, kGray, kTeal,   kPink};

  {
    std::ifstream config_file{argv[1]};
    if (!config_file.is_open()) {
      std::cout << "Error: Could not open config file" << std::endl;
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
  // double first_to_d
  // std::cout << "max: " << max << std::endl;
  {
    auto plot_title = config["plot_title"].get<std::string>();
    double uplimit{-INFINITY};

    size_t i{};
    for (const auto &entry : config["hists"]) {
      if (entry.value("skip", false)) {
        continue;
      }
      std::string legend = entry["legend"];
      std::string file_path = entry["file_path"];
      std::string plot_name = entry["hist"];
      double scale = entry.value("scale", 1.);
      size_t rebin_factor = entry.value<size_t>("rebin", 1);
      TFile file{file_path.c_str(), "READ"};
      if (!file.IsOpen()) {
        std::cout << "Error: Could not open file " << file_path << std::endl;
        return 1;
      }
      std::unique_ptr<TH1> hist{
          dynamic_cast<TH1 *>(file.Get(plot_name.c_str()))};
      if (!hist) {
        std::cout << "Error: Could not get histogram " << plot_name
                  << " from file " << file_path << std::endl;
        return 1;
      }
      hist->Scale(scale / rebin_factor);
      if (rebin_factor != 1)
        hist->Rebin(rebin_factor);
      if (entry.value("shape", false)) {
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
          hist = normalize_slice(hist_2dptr, entry["normalize"].get<int>());
        }
      } else {
        if (last_bin_as_overflow) {
          std::cout << "last bin as overflow" << std::endl
                    << "before \t" << hist->GetBinContent(hist->GetNbinsX())
                    << std::endl;
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
                << " scale " << scale << " rebin with " << rebin_factor
                << std::endl;
      if (!config.contains("max_y_value"))
        max = std::max(max, hist->GetMaximum());
      ResetStyle(hist);
      hist->SetLineStyle(entry.value("line_style", kSolid));
      hist->SetLineWidth(entry.value("line_width", 2));
      hist->SetMarkerStyle(entry.value("marker_style", hist->GetMarkerStyle()));
      auto color = entry.value("line_color", -1);
      if (color != -1)
        hist->SetLineColor(color);
      else
        hist->SetLineColor(col[i++]);
      hist->SetMarkerColor(hist->GetLineColor());
      hist->SetTitle(plot_title.c_str());
      entries.emplace_back(legend, std::move(hist));
      append_opt.push_back(entry.value("append_opt", ""));
      leg_opt.push_back(entry.value("leg_opt", "lpf"));
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
        objects.emplace_back(misc_objects_maker.at(entry["type"])(entry));
      }
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
    if (config.value("logy", false))
      canvas->SetLogy();
    if (config.value("logz", false))
      canvas->SetLogz();
    const auto draw_opt = config.value("draw_opt", "hist C");
    for (std::size_t i = 0; i < entries.size(); ++i) {
      auto &[legend_title, hist] = entries[i];
      if (add_median) {
        auto median_value = median(dynamic_cast<TH1D *>(hist.get()));
        auto median_line =
            std::make_unique<TLine>(median_value, 0, median_value, max);
        median_line->SetLineColor(hist->GetLineColor());
        median_line->SetLineStyle(2);
        objects.emplace_back(std::move(median_line));
        // legend_title += " median: " + std::to_string(median_value);
        std::stringstream ss{};
        ss << "("
           << "m = " << std::fixed << std::setprecision(2) << median_value
           << ")";
        legend_title += ss.str();
      }
      // hist->SetLineColor(col[i]);
      // hist->SetLineWidth(2);
      if (!dynamic_cast<TH2 *>(hist.get()))
        leg->AddEntry(hist.get(), legend_title.c_str(), leg_opt[i].c_str());
      const auto opt = i == 0 ? draw_opt : draw_opt + " same";
      std::string allopt = opt + append_opt[i];
      hist->SetMaximum(max);
      if (ymin_set) {
        hist->SetMinimum(min_y_value);
      }
      // if (!config.value("logy", false))
      //   hist->SetMinimum(0);
      // hist->SetMa
      hist->Draw(allopt.c_str());
    }
    for (const auto &object : objects)
      object->Draw("same");
    leg->Draw(config.value("legend_draw_opt", "").c_str());
    for (const std::string &output_name : config["output_names"]) {
      if (!std::filesystem::path(output_name).parent_path().empty())
        std::filesystem::create_directories(
            std::filesystem::path(output_name).parent_path());
      canvas->SaveAs(output_name.c_str());
    }
  }

  return 0;
}
