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
#include <boost/program_options.hpp>
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
      {"_H1", " H"}};
  for (auto &&[from, to] : replace_list) {
    std::regex reg{from};
    str_in = std::regex_replace(str_in, reg, to);
  }
  std::cout << "Normalized name: " << str_in << std::endl;
  return str_in;
}

int main(int argc, char **argv) {
  constexpr double titleoffset = 0.5;
  constexpr double textfont = 42;
  constexpr double textsize = 0.07;
  constexpr double fair_share = 0.6;
  constexpr double min_head_off = 0.05;

  const std::vector<std::string> interactions{"nu_e_H1", "nu_e_bar_H1",
                                              "nu_mu_H1", "nu_mu_bar_H1"};
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
        return get_spline_sum(capture0, capture1,
                              std::forward<decltype(PH1)>(PH1));
      }) |
      std::ranges::to<std::vector>();
  auto canvas = std::make_unique<TCanvas>("canvas", "canvas", 1000, 600);
  canvas->cd();
  // canvas->Divide()
  auto pad1 =
      std::make_unique<TPad>("plot", "plot", 0.0, 1 - fair_share, 1.0, 1.0, 0);
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

  pad1->SetTopMargin(currentt / 4);

  pad1->SetBottomMargin(0);
  pad2->SetTopMargin(0);
  pad2->SetBottomMargin(currentm * 2.0);
  pad1->cd();
  pad1->SetLogx();
  pad1->SetGrid(1, 1);
  // pad1->SetLogy();
  auto legend = std::make_unique<TLegend>(x1, y1, x2, y2);
  auto legend_NuWro =
      leg_off ? std::make_unique<TLegend>(x1, y1 - (y2 - y1), x2, y1)
              : std::make_unique<TLegend>(x1 - (x2 - x1), y1, x1, y2);

  // auto legend = std::make_unique<TLegend>();
  ResetStyle(legend.get());
  ResetStyle(legend_NuWro.get());
  legend->SetNColumns(2);
  legend->SetHeader((extra_text).c_str());
  legend->SetTextSize(legend->GetTextSize() * 1.6);

  // const int colors[] = {kRed, kBlue, kBlue, kRed, kMagenta};
  const int colors[] = {kRed, kRed, kOrange, kOrange};
  const int style[] = {kSolid, kDashed, kSolid, kDashed};
  const int width[] = {3, 3, 2, 2};
  // auto max_val1 =
  //     std::ranges::max(splines | std::views::transform([](const TF1 &func) {
  //                        return func.GetMaximum();
  //                      }));
  auto max_val1 = get_maxmium(file.get(), plot_names);

  // double max_val = std::max(max_val1, max_val2);
  for (auto &&[id, spline] : std::views::enumerate(splines)) {
    // spline.SetMinimum(0);
    ResetStyle(&spline);
    spline.SetTitle(out_title.c_str());
    spline.SetMinimum(-min_head_off);
    spline.SetMaximum(max_val1 * factor);
    // spline.GetXaxis()->SetTitle("E_{#nu} (GeV)");
    spline.GetYaxis()->SetTitle(
        "#sigma / #it{E}_{#nu} (10^{#minus 38} cm^{2}/GeV/nucleon)");
    spline.SetLineStyle(style[id]);
    spline.SetLineColor(colors[id]);
    spline.SetLineWidth(width[id]);
    spline.GetYaxis()->SetTitleOffset(titleoffset);
    spline.GetXaxis()->SetLabelSize(textsize);
    spline.GetXaxis()->SetTitleSize(textsize);
    spline.GetYaxis()->SetLabelSize(textsize);
    spline.GetYaxis()->SetTitleSize(textsize);
    spline.Draw(id ? "same" : "");
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
  auto spline2ratio = std::views::transform([&](const TF1 &func_in) {
                        return TF1{"",
                                   [&](const double *x, const double *) {
                                     auto a = func_in.Eval(x[0]);
                                     //  a = a < 0 ? 0 : a;
                                     auto b = base_func.Eval(x[0]);
                                     //  b = b < 0 ? 0 : b;
                                     if (a == b) {
                                       return 1.;
                                     }
                                     if (std::abs(b) < 1e-5) {
                                       return 0.;
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
    spline.GetYaxis()->SetTitle(
        ("Ratio to  " + name_normalize(std::string{splines[0].GetName()}))
            .c_str());
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

  canvas->SaveAs((out_title + "_H.pdf").c_str());
  canvas->SaveAs((out_title + "_H.eps").c_str());
  canvas->SaveAs((out_title + "_H.svg").c_str());
  return 0;
}