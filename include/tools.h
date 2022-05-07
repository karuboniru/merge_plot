#pragma once
#include <TAxis.h>
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

template <typename T>
concept HistLike = std::is_base_of_v<TH1, std::remove_cvref_t<T>> || std::is_base_of_v<TH2, std::remove_cvref_t<T>> ||
    std::is_base_of_v<THStack, std::remove_cvref_t<T>>;

template <typename T>
concept HistLikePtr = requires(T obj) {
    { *obj } -> HistLike;
};

template <typename T>
concept StackPtr = requires(T obj) {
    std::is_base_of_v<THStack, std::remove_cvref_t<decltype(*obj)>>;
};

template <typename T>
concept Hist2D = std::is_base_of_v<TH2, std::remove_cvref_t<T>>;

template <typename T>
concept Hist1D = std::is_base_of_v<TH1, std::remove_cvref_t<T>>;

template <typename T>
concept AxisLIke = std::is_base_of_v<TAxis, std::remove_cvref_t<T>> || std::is_base_of_v<TGaxis, std::remove_cvref_t<T>>;

template <typename T>
concept AxisLIkePtr = requires(T obj) {
    { *obj } -> AxisLIke;
};

template <typename T>
concept CanvasPtr = requires(T obj) {
    std::is_base_of_v<TCanvas, std::remove_cvref_t<decltype(*obj)>>;
};

namespace style {
    static constexpr Double_t fgkTextSize = 0.05;
    static constexpr Double_t fgkTitleSize = 0.05;
    static constexpr Double_t fgkMarkerSize = 1;
    static constexpr Double_t fgkLineWidth = 2;
    static constexpr Int_t fgkTextFont = 42;
    static constexpr Double_t fgkLabelOffset = 0.01;
    static constexpr Double_t fgkXTitleOffset = 1.25; // 1.1;//1.25;
    static constexpr Double_t fgkYTitleOffset = 1.1;  // 1.2;
    static constexpr Double_t fgkTickLength = 0.02;
} // namespace style

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
        gROOT->ForceStyle();
    }
};

inline global_style global_style_instance;

template <CanvasPtr T>
void PadSetup(T &&currentPad, const Double_t currentLeft = 0.12, const Double_t currentTop = 0.09,
              const Double_t currentRight = 0.13, const Double_t currentBottom = 0.14) {
    currentPad->SetTicks(1, 1);
    currentPad->SetLeftMargin(currentLeft);
    currentPad->SetTopMargin(currentTop);
    currentPad->SetRightMargin(currentRight);
    currentPad->SetBottomMargin(currentBottom);

    currentPad->SetFillColor(0); // this is the desired one!!!
}

template <AxisLIkePtr T> void AxisStyle(T &&ax, Bool_t kcen) {
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
    if (std::is_base_of_v<TGaxis, T>) {
        ax->SetTitleOffset(fgkXTitleOffset);
    }
}

template <HistLikePtr T> void ResetStyle(T &&obj, TVirtualPad *cpad, Bool_t kcen = true) {
    using namespace style;
    if (!obj) {
        printf("style::ResetStyle obj null!\n");
        exit(1);
    }

    AxisStyle(obj->GetXaxis(), kcen);
    AxisStyle(obj->GetYaxis(), kcen);

    obj->GetXaxis()->SetTitleOffset(fgkXTitleOffset);
    obj->GetYaxis()->SetTitleOffset(fgkYTitleOffset);
    if constexpr (!StackPtr<T>) {
        obj->SetMarkerSize(fgkMarkerSize);
        if (cpad) {
            TPaletteAxis *palette = (TPaletteAxis *)obj->GetListOfFunctions()->FindObject("palette");
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
}

std::unique_ptr<TCanvas> inline getCanvas(const char *name = "") {
    const double factor = 1;
    auto c = std::make_unique<TCanvas>(name, name, 800 * factor, 600 * factor);
    PadSetup(c);
    c->cd();
    return c;
}

template <typename T>
concept LegendPtr = std::is_base_of_v<TLegend, std::remove_cvref_t<decltype(*std::declval<T>())>>;

template <LegendPtr T> void ResetStyle(T &&obj, Double_t mar = -999, const Double_t ff = 0.8) {
    using namespace style;
    if (mar > 0) {
        obj->SetMargin(mar);
    }
    obj->SetFillStyle(-1);
    obj->SetBorderSize(-1);
    obj->SetTextFont(fgkTextFont);
    obj->SetTextSize(fgkTextSize * ff);
}