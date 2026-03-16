#pragma once

#include <imgui.h>

#include <array>
#include <string>

namespace MaraGl
{
    struct ImGuiFontSet
    {
        ImFont *defaultFont = nullptr;
        ImFont *monoFont = nullptr;
        ImFont *mergedIconFont = nullptr;
        std::string mergedIconFontPath;
    };

    inline ImGuiFontSet ConfigureDefaultImGuiFonts(ImGuiIO &io,
                                                   float defaultFontSize = 20.0f,
                                                   float monoFontSize = 20.0f)
    {
        ImGuiFontSet fonts;
        io.Fonts->Clear();

        fonts.defaultFont = io.Fonts->AddFontFromFileTTF(
            "resources/fonts/Roboto/Roboto-VariableFont_wdth,wght.ttf",
            defaultFontSize);

        if (!fonts.defaultFont)
            fonts.defaultFont = io.Fonts->AddFontDefault();

        io.FontDefault = fonts.defaultFont;

        static constexpr ImWchar iconGlyphRanges[] = {0xf000, 0xf8ff, 0};
        const std::array<const char *, 4> iconPathOptions = {
            "resources/fonts/FontAwesome/Font Awesome 6 Free-Solid-900.otf",
            "resources/fonts/FontAwesome6-Solid.otf",
            "resources/fonts/FontAwesome/FontAwesome6-Solid.otf",
            "resources/fonts/icons/FontAwesome6-Solid.otf"};

        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.DstFont = fonts.defaultFont;
        iconConfig.PixelSnapH = true;
        iconConfig.GlyphOffset = ImVec2(0.0f, 1.0f);

        for (const char *path : iconPathOptions)
        {
            ImFont *merged = io.Fonts->AddFontFromFileTTF(
                path,
                defaultFontSize,
                &iconConfig,
                iconGlyphRanges);
            if (!merged)
                continue;

            fonts.mergedIconFont = fonts.defaultFont;
            fonts.mergedIconFontPath = path;
            break;
        }

        fonts.monoFont = io.Fonts->AddFontFromFileTTF(
            "resources/fonts/Roboto_Mono/RobotoMono-VariableFont_wght.ttf",
            monoFontSize);

        return fonts;
    }
}