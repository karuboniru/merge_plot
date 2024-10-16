#include "spline_plot_params.h"
#include <GuiTypes.h>
#include <Rtypes.h>
#include <TArrow.h>
#include <TAttLine.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1.h>
#include <THStack.h>
#include <TLegend.h>
#include <TMarker.h>
#include <TPad.h>
#include <TSpline.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TText.h>
#include <functional>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <ranges>
#include <regex>
#include <string>
#include <tools.h>

std::string name_normalize(std::string str_in) {
  const std::vector<std::pair<std::string, std::string>> replace_list{
      {"nu_e_bar", "#bar{#nu}_{e}"},
      {"nu_mu_bar", "#bar{#nu}_{#mu}"},
      {"nu_e", "#nu_{e}"},
      {"nu_mu", "#nu_{#mu}"},
      {"_C12", " ^{12}C"}};
  for (auto &&[from, to] : replace_list) {
    std::regex reg{from};
    str_in = std::regex_replace(str_in, reg, to);
  }
  std::cout << "Normalized name: " << str_in << std::endl;
  return str_in;
}

auto get_spline_sum_filtered(TFile *file,
                             const std::vector<std::string> &channels,
                             std::string interaction) {
  auto splines_vec =
      channels | std::views::transform([&](const std::string &channel) {
        auto path = interaction + "/" + channel;
        auto xsec_graph = dynamic_cast<TGraph *>(file->Get(path.c_str()));
        if (!xsec_graph) {
          std::cerr << "Error: Cannot find spline " << path
                    << " this might be expect on QEL channel" << std::endl;
          return std::optional<TSpline3>{std::nullopt};
        }
        auto opt =
            std::optional<TSpline3>{std::in_place, path.c_str(), xsec_graph};
        opt.value().SetName(channel.c_str());
        return opt;
      }) |
      std::views::filter(
          [](auto &&spline) -> bool { return spline.has_value(); }) |
      std::views::transform([](auto &&spline) { return spline.value(); }) |
      std::views::filter([](auto &&spline) { return spline.Eval(100) > 0.1; }) |
      std::ranges::to<std::vector>();
  std::vector<TF1> splines;
  splines.resize(splines_vec.size());
  for (auto &&[i, spline] : splines_vec | std::views::enumerate) {
    splines[i] = TF1{spline.GetName(),
                     [=](const double *x, const double *) {
                       return std::ranges::fold_left(
                           splines_vec | std::views::take(i + 1) |
                               std::views::transform([&](auto &&spline) {
                                 return spline.Eval(x[0]) / x[0] / 12.;
                               }),
                           0.0, std::plus<double>{});
                     },
                     min, max, 0};
  }
  return splines;
}

int main(int argc, char **argv) {
  constexpr double titleoffset = 0.5;
  constexpr double textfont = 42;
  constexpr double textsize = 0.07;
  constexpr double fair_share = 0.6;

  const std::vector<std::string> interactions{"nu_e_C12", "nu_e_bar_C12",
                                              "nu_mu_C12", "nu_mu_bar_C12"};
  auto params = read_options(argc, argv);
  auto &&[x1, y1, x2, y2, factor, leg_off, root_input, out_title, extra_text,
          plot_names] = params;
  auto file = std::make_unique<TFile>(root_input.c_str());
  if (file->IsZombie()) {
    std::cerr << "Error: Cannot open file " << root_input << std::endl;
    return 1;
  }
  auto splines =
      interactions |
      std::views::transform([capture0 = file.get(),
                             capture1 = std::cref(plot_names)](auto &&PH1) {
        return get_spline_sum_filtered(capture0, capture1,
                                       std::forward<decltype(PH1)>(PH1));
      }) |
      std::ranges::to<std::vector>();
  // auto splines = get_spline_sum_filtered(file.get(), plot_names,
  // interactions[0]);
  auto plot = [&](std::vector<TF1> &splines, const std::string &out_title) {
    auto canvas = std::make_unique<TCanvas>("canvas", "canvas", 1000, 600);
    canvas->cd();
    // canvas->Divide()
    auto pad1 = std::make_unique<TPad>("plot", "plot", 0.0, 1 - fair_share, 1.0,
                                       1.0, 0);
    auto pad2 = std::make_unique<TPad>("ratio", "ratio", 0.0, 0.0, 1.0,
                                       1 - fair_share, 0);
    PadSetup(canvas);
    PadSetup(pad1);
    PadSetup(pad2);
    auto currentm = pad1->GetBottomMargin();
    auto currentr = canvas->GetRightMargin();
    auto currentl = canvas->GetLeftMargin();
    auto currentt = canvas->GetTopMargin();
    // canvas->SetRightMargin(0.5);
    pad1->SetRightMargin(currentr / 2);
    pad2->SetRightMargin(currentr / 2);

    pad1->SetLeftMargin(currentl / 1.5);
    pad2->SetLeftMargin(currentl / 1.5);

    // pad1->SetTopMargin(currentt / 4);

    pad1->SetBottomMargin(0);
    pad2->SetTopMargin(0);
    pad2->SetBottomMargin(currentm * 2.0);
    pad1->cd();
    pad1->SetLogx();
    pad1->SetGrid(1, 1);
    // pad1->SetLogy();
    auto legend = std::make_unique<TLegend>(x1, y1, x2, y2);

    // auto legend = std::make_unique<TLegend>();
    ResetStyle(legend.get());
    legend->SetNColumns(1);
    legend->SetHeader((extra_text).c_str());
    legend->SetTextSize(legend->GetTextSize() * 1.6);

    // const int colors[] = {kRed, kBlue, kBlue, kRed, kMagenta};
    const int colors[] = {kRed, kBlue, kOrange, kBlack, kGreen};
    const int style[] = {kSolid, kSolid, kSolid, kSolid, kDashed};
    const int width[] = {2, 4, 4, 2, 4};
    auto max_val1 =
        std::ranges::max(splines | std::views::transform([](const TF1 &func) {
                           return func.GetMaximum();
                         }));
    double max_val = std::max(max_val1, 0.);
    for (auto &&[id, spline] : splines | std::views::enumerate) {
      // spline.SetMinimum(0);
      ResetStyle(&spline);
      spline.SetTitle(out_title.c_str());
      spline.SetMinimum(-5e-3);
      spline.SetMaximum(max_val * factor);
      // spline.GetXaxis()->SetTitle("E_{#nu} (GeV)");
      spline.GetYaxis()->SetTitle(
          "#sigma / #it{E}_{#nu} (10^{#minus 38} cm^{2}/GeV/nucleon)");
      spline.SetLineStyle(style[id]);
      spline.SetLineColor(colors[id]);
      spline.SetFillColor(colors[id]);
      spline.SetLineWidth(width[id]);
      spline.GetYaxis()->SetTitleOffset(titleoffset);
      spline.GetXaxis()->SetLabelSize(textsize);
      spline.GetXaxis()->SetTitleSize(textsize);
      spline.GetYaxis()->SetLabelSize(textsize);
      spline.GetYaxis()->SetTitleSize(textsize);
      spline.Draw(id ? "same" : "");
    }
    for (auto &&spline : splines | std::views::reverse) {
      legend->AddEntry(
          &spline, name_normalize(std::string{spline.GetName()}).c_str(), "l");
    }
    legend->Draw();

    canvas->cd();
    pad1->Draw();

    pad2->cd();
    pad2->SetGrid(1, 1);
    pad2->SetLogx();
    auto &&base_func = splines[0];
    auto &&max_func = splines[splines.size() - 1];
    auto spline2ratio = std::views::transform([&](const TF1 &func_in) {
                          return TF1{"",
                                     [&](const double *x, const double *) {
                                       auto a = func_in.Eval(x[0]);
                                       //  a = a < 0 ? 0 : a;
                                       auto b = max_func.Eval(x[0]);
                                       //  b = b < 0 ? 0 : b;
                                       if (a == b) {
                                         return 1.;
                                       }
                                       if (std::abs(b) < 1e-5) {
                                         return 1.;
                                       }
                                       return a / b;
                                     },
                                     min, max, 0};
                        }) |
                        std::ranges::to<std::vector>();
    auto spline_ratio = splines | spline2ratio;

    for (auto &&[id, spline] : std::views::enumerate(spline_ratio)) {
      spline.SetMaximum(1.2);
      spline.SetMinimum(-0.1);
      ResetStyle(&spline);
      spline.SetTitle(out_title.c_str());
      spline.GetXaxis()->SetTitle("#it{E}_{#nu} (GeV)");
      spline.GetYaxis()->SetTitle("Stacked Ratio");
      spline.SetLineStyle(style[id]);
      spline.SetLineColor(colors[id]);
      spline.SetLineWidth(style[id] == kDashed ? 3 : 2);
      spline.GetXaxis()->SetLabelSize(textsize * fair_share / (1 - fair_share));
      spline.GetXaxis()->SetTitleSize(textsize * fair_share / (1 - fair_share));

      spline.GetYaxis()->SetLabelSize(textsize * fair_share / (1 - fair_share));
      spline.GetYaxis()->SetTitleSize(textsize * fair_share / (1 - fair_share));
      spline.GetYaxis()->SetTitleOffset(titleoffset /
                                        (fair_share / (1 - fair_share)));
      spline.Draw(id ? "same" : "");
    }

    canvas->cd();
    pad2->Draw();

    canvas->SaveAs((out_title + ".pdf").c_str());
    canvas->SaveAs((out_title + ".eps").c_str());
    canvas->SaveAs((out_title + ".svg").c_str());
  };
  for (auto &&[interaction, splines] : std::views::zip(interactions, splines)) {
    plot(splines, out_title + interaction);
  }
  return 0;
}