#pragma once
#include <TAxis.h>
#include <TCanvas.h>
#include <TGaxis.h>
#include <TGraph.h>
#include <TH2D.h>
#include <THStack.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TPaletteAxis.h>
#include <TROOT.h>
#include <TStyle.h>
#include <iostream>
#include <string>

template <typename PtrType, typename Base>
concept IsBasePtr = std::is_base_of_v<
    std::__remove_cvref_t<Base>,
    std::__remove_cvref_t<decltype(*std::declval<PtrType>())>>;

template <typename PtrType, typename... Bases>
concept IsAnyBasePtr = (IsBasePtr<PtrType, Bases> || ...);

template <typename PtrType, typename... Bases>
concept IsAllBasePtr = (IsBasePtr<PtrType, Bases> && ...);

namespace style {
constexpr auto fgkTextSize = 0.05;
constexpr auto fgkTitleSize = 0.05;
constexpr auto fgkMarkerSize = 1;
constexpr auto fgkLineWidth = 2;
constexpr auto fgkTextFont = 42;
constexpr auto fgkLabelOffset = 0.01;
constexpr auto fgkXTitleOffset = 1.25; // 1.1;//1.25;
constexpr auto fgkYTitleOffset = 1.1;  // 1.2;
constexpr auto fgkTickLength = 0.02;

// constexpr auto fgkTextSize = 0.07;
// constexpr auto fgkTitleSize = 0.07;
// constexpr auto fgkMarkerSize = 1;
// constexpr auto fgkLineWidth = 2;
// constexpr auto fgkTextFont = 42;
// constexpr auto fgkLabelOffset = 0.01;
// constexpr auto fgkXTitleOffset = 1.1; // 1.1;//1.25;
// constexpr auto fgkYTitleOffset = 0.75;  // 1.2;
// constexpr auto fgkTickLength = 0.02;

class global_style {
public:
  global_style(bool lStat = false) {
    using namespace style;
    std::cout << "Setting Style" << std::endl;
    gStyle->SetFrameBorderMode(0);
    gStyle->SetFrameFillColor(0);
    gStyle->SetCanvasBorderMode(0);
    gStyle->SetPadBorderMode(0);
    gStyle->SetPadColor(10);
    gStyle->SetCanvasColor(10);
    gStyle->SetTitleFillColor(10);
    gStyle->SetTitleBorderSize(-1);
    gStyle->SetStatColor(10);
    gStyle->SetStatBorderSize(-1);
    // gStyle->SetLegendBorderSize(-1);
    //
    gStyle->SetDrawBorder(0);
    gStyle->SetTextFont(fgkTextFont);
    gStyle->SetStatFont(fgkTextFont);
    gStyle->SetStatFontSize(fgkTextSize);
    gStyle->SetStatX(0.97);
    gStyle->SetStatY(0.98);
    gStyle->SetStatH(0.03);
    gStyle->SetStatW(0.3);
    gStyle->SetTickLength(fgkTickLength, "xy");
    gStyle->SetEndErrorSize(3);
    gStyle->SetLabelSize(fgkTextSize, "xyz");
    gStyle->SetLabelFont(fgkTextFont, "xyz");
    gStyle->SetLabelOffset(fgkLabelOffset, "xyz");
    gStyle->SetTitleFont(fgkTextFont, "xyz");
    gStyle->SetTitleFont(fgkTextFont, "");
    gStyle->SetTitleFontSize(fgkTitleSize);
    gStyle->SetTitleOffset(fgkXTitleOffset, "x");
    gStyle->SetTitleOffset(fgkYTitleOffset, "y");
    gStyle->SetTitleOffset(1.0, "z");
    gStyle->SetTitleSize(fgkTitleSize, "xyz");
    gStyle->SetTitleSize(fgkTitleSize, "");
    gStyle->SetMarkerSize(fgkMarkerSize);
    gStyle->SetPalette(1, 0);
    TGaxis::SetMaxDigits(3);
    gStyle->SetTitleBorderSize(-1);
    if (lStat) {
      gStyle->SetOptTitle(1);
      gStyle->SetOptStat(1111);
      gStyle->SetOptFit(1111);
    } else {
      gStyle->SetOptTitle(0);
      gStyle->SetOptStat(0);
      gStyle->SetOptFit(0);
    }

    TGaxis::SetMaxDigits(3);
    gStyle->SetTitleBorderSize(-1);

    gROOT->ForceStyle();
  }
};
// automatically set the style, per program instance
inline global_style global_style_instance;
} // namespace style

template <typename T>
void PadSetup(T &&currentPad, const Double_t currentLeft = 0.12,
              const Double_t currentTop = 0.08,
              const Double_t currentRight = 0.037,
              const Double_t currentBottom = 0.17) {
  currentPad->SetTicks(1, 1);
  currentPad->SetLeftMargin(currentLeft);
  currentPad->SetTopMargin(currentTop);
  currentPad->SetRightMargin(currentRight);
  currentPad->SetBottomMargin(currentBottom);

  currentPad->SetFillColor(0); // this is the desired one!!!
}

template <IsAnyBasePtr<TAxis, TGaxis> T> void AxisStyle(T &&ax, Bool_t kcen) {
  if (!ax) {
    throw std::runtime_error("Axis is null, in AxisStyle<AxisLikePtr>");
  }
  using namespace style;
  ax->SetTickLength(fgkTickLength);

  ax->SetLabelFont(fgkTextFont);
  ax->SetLabelSize(fgkTextSize);
  ax->SetLabelOffset(fgkLabelOffset);

  ax->SetTitleFont(fgkTextFont);
  ax->SetTitleSize(fgkTitleSize);

  kcen = 1;
  ax->CenterTitle(kcen);

  ax->SetNdivisions(505);
  if constexpr (IsBasePtr<T, TGaxis>) {
    ax->SetTitleOffset(fgkXTitleOffset);
  }
}

template <IsAnyBasePtr<TH1, TGraph> T>
void ResetStyle(T &&obj, TVirtualPad *cpad = nullptr, Bool_t kcen = true) {
  using namespace style;
  if (!obj) {
    throw std::runtime_error("Object is null in ResetStyle<HistLikePtr>");
  }

  AxisStyle(obj->GetXaxis(), kcen);
  AxisStyle(obj->GetYaxis(), kcen);

  obj->GetXaxis()->SetTitleOffset(fgkXTitleOffset);
  obj->GetYaxis()->SetTitleOffset(fgkYTitleOffset);
  obj->SetMarkerSize(fgkMarkerSize);
  if (cpad) {
    TPaletteAxis *palette =
        (TPaletteAxis *)obj->GetListOfFunctions()->FindObject("palette");
    if (!palette) {
      printf("ResetStyle no palette!!\n");
      obj->GetListOfFunctions()->Print();
    } else {
      palette->SetX1NDC(1 - cpad->GetRightMargin() + 0.005);
      palette->SetX2NDC(1 - cpad->GetRightMargin() / 3 * 2);
      palette->SetY1NDC(cpad->GetBottomMargin());
      palette->SetY2NDC(1 - cpad->GetTopMargin());
      palette->SetLabelFont(fgkTextFont);
      palette->SetLabelSize(fgkTextSize);
      palette->SetLabelOffset(fgkLabelOffset);
    }
  }
}

template <IsBasePtr<TF1> T>
void ResetStyle(T &&obj, TVirtualPad *cpad = nullptr, Bool_t kcen = true) {
  using namespace style;
  if (!obj) {
    throw std::runtime_error("Object is null in ResetStyle<HistLikePtr>");
  }

  AxisStyle(obj->GetXaxis(), kcen);
  AxisStyle(obj->GetYaxis(), kcen);

  obj->GetXaxis()->SetTitleOffset(fgkXTitleOffset);
  obj->GetYaxis()->SetTitleOffset(fgkYTitleOffset);
  // obj->SetMarkerSize(fgkMarkerSize);
  // if (cpad) {
  //   TPaletteAxis *palette =
  //       (TPaletteAxis *)obj->GetListOfFunctions()->FindObject("palette");
  //   if (!palette) {
  //     printf("ResetStyle no palette!!\n");
  //     obj->GetListOfFunctions()->Print();
  //   } else {
  //     palette->SetX1NDC(1 - cpad->GetRightMargin() + 0.005);
  //     palette->SetX2NDC(1 - cpad->GetRightMargin() / 3 * 2);
  //     palette->SetY1NDC(cpad->GetBottomMargin());
  //     palette->SetY2NDC(1 - cpad->GetTopMargin());
  //     palette->SetLabelFont(fgkTextFont);
  //     palette->SetLabelSize(fgkTextSize);
  //     palette->SetLabelOffset(fgkLabelOffset);
  //   }
  // }
}

std::unique_ptr<TCanvas> inline getCanvas(const char *name = "", int ww = 600,
                                          int wh = 400) {
  constexpr size_t factor = 1;
  auto c = std::make_unique<TCanvas>(name, name, ww * factor, wh * factor);
  PadSetup(c);
  c->cd();
  return c;
}

template <IsBasePtr<TLegend> T>
void ResetStyle(T &&obj, Double_t mar = -999, const Double_t ff = 0.8) {
  using namespace style;
  if (mar > 0) {
    obj->SetMargin(mar);
  }
  obj->SetFillStyle(-1);
  obj->SetBorderSize(-1);
  obj->SetTextFont(fgkTextFont);
  obj->SetTextSize(fgkTextSize * ff);
}

template <IsAnyBasePtr<TAxis, TGaxis> T> void BinLogX(T &&axis) {
  // void XGLUtils::BinLogX(TAxis *axis)
  //
  //  Method for the correct logarithmic binning of histograms
  //  copied and modified from AliTPCcalibBase

  const auto bins = axis->GetNbins();

  const auto from = axis->GetXmin();
  const auto to = axis->GetXmax();
  if (from < 1e-12)
    return;
  auto new_bins = std::make_unique<double[]>(bins + 1);

  new_bins[0] = from;
  const auto factor = std::pow(to / from, 1. / bins);

  for (int i = 1; i <= bins; i++) {
    new_bins[i] = factor * new_bins[i - 1];
  }
  axis->Set(bins, new_bins.get());
}

template <IsBasePtr<TH1> T> void UpdateLogX(T &&obj) {
  TAxis *axis = obj->GetXaxis();
  double xmin = axis->GetXmin();
  double xmax = axis->GetXmax();
  axis->SetLimits(std::pow(10, xmin), std::pow(10, xmax));
  BinLogX(axis);
}

template <IsBasePtr<TH2> T>
auto normalize_slice(T &&hist, bool on_axis_x = true) {
  using HistType = std::__remove_cvref_t<decltype(*std::declval<T>())>;
  using std::string_literals::operator""s;
  auto hist_name_new = hist->GetName() + (on_axis_x ? "_norm_x"s : "_norm_y"s);
  auto hist_norm =
      reinterpret_cast<HistType *>(hist->Clone(hist_name_new.c_str()));
  auto axis = on_axis_x ? hist->GetXaxis() : hist->GetYaxis();
  auto o_axis = on_axis_x ? hist->GetYaxis() : hist->GetXaxis();
  for (int i = 1; i < axis->GetNbins() + 1; i++) {
    double max = 0;
    for (int j = 1; j < o_axis->GetNbins() + 1; j++) {
      auto [x, y] = on_axis_x ? std::make_pair(i, j) : std::make_pair(j, i);
      max = std::max(max, hist->GetBinContent(x, y));
    }
    for (int j = 1; j < o_axis->GetNbins() + 1; j++) {
      auto [x, y] = on_axis_x ? std::make_pair(i, j) : std::make_pair(j, i);
      if (max != 0)
        hist_norm->SetBinContent(x, y, hist->GetBinContent(x, y) / max);
    }
  }
  return std::unique_ptr<HistType>(hist_norm);
}