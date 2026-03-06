#pragma once

#include <string>

namespace MaraGl
{
    namespace Icons
    {
        // FontAwesome 6 Free (Solid) icon definitions
        // Works with both TTF and OTF font files
        // Use these constants when you have loaded an icon font
        // Example: ImGui::Text("%s Save", Icons::FileSave);

        // File operations
        constexpr const char *FileNew = "\xEF\x83\x9B";    // fa-file
        constexpr const char *FileOpen = "\xEF\x81\x81";   // fa-folder-open
        constexpr const char *FileSave = "\xEF\x83\x87";   // fa-floppy-disk
        constexpr const char *FileSaveAs = "\xEF\x83\x88"; // fa-floppy-disk (variation)
        constexpr const char *FileImport = "\xEF\x81\x90"; // fa-upload
        constexpr const char *FileExport = "\xEF\x81\x8A"; // fa-download

        // Edit operations
        constexpr const char *Edit = "\xEF\x81\x84";   // fa-pen
        constexpr const char *Undo = "\xEF\x8C\xBB";   // fa-arrow-rotate-left
        constexpr const char *Redo = "\xEF\x8C\xBC";   // fa-arrow-rotate-right
        constexpr const char *Copy = "\xEF\x83\xB5";   // fa-copy
        constexpr const char *Paste = "\xEF\x83\xB6";  // fa-paste
        constexpr const char *Cut = "\xEF\x83\xB7";    // fa-cut
        constexpr const char *Delete = "\xEF\x81\xB8"; // fa-trash

        // View operations
        constexpr const char *Eye = "\xEF\x81\xAE";      // fa-eye
        constexpr const char *EyeSlash = "\xEF\x81\xB0"; // fa-eye-slash
        constexpr const char *Expand = "\xEF\x81\xB5";   // fa-expand
        constexpr const char *Compress = "\xEF\x81\xB6"; // fa-compress
        constexpr const char *Maximize = "\xEF\x86\x84"; // fa-maximize

        // Object manipulation
        constexpr const char *Move = "\xEF\x85\x85";     // fa-arrows
        constexpr const char *MoveUp = "\xEF\x85\x86";   // fa-arrow-up
        constexpr const char *MoveDown = "\xEF\x85\x87"; // fa-arrow-down
        constexpr const char *Rotate = "\xEF\x85\xB8";   // fa-rotate
        constexpr const char *Scale = "\xEF\x81\xB6";    // fa-expand
        constexpr const char *Lock = "\xEF\x84\x90";     // fa-lock
        constexpr const char *Unlock = "\xEF\x84\x91";   // fa-unlock

        // Playback controls
        constexpr const char *Play = "\xEF\x81\x8B";        // fa-play
        constexpr const char *Pause = "\xEF\x81\x8C";       // fa-pause
        constexpr const char *Stop = "\xEF\x81\x8D";        // fa-stop
        constexpr const char *StepBack = "\xEF\x81\x8E";    // fa-step-backward
        constexpr const char *StepForward = "\xEF\x81\x8F"; // fa-step-forward

        // UI elements
        constexpr const char *Plus = "\xEF\x81\xA7";     // fa-plus
        constexpr const char *Minus = "\xEF\x81\xA8";    // fa-minus
        constexpr const char *Check = "\xEF\x81\x98";    // fa-check
        constexpr const char *Times = "\xEF\x81\x99";    // fa-times (X)
        constexpr const char *Search = "\xEF\x80\x82";   // fa-search
        constexpr const char *Settings = "\xEF\x80\x93"; // fa-cog
        constexpr const char *Refresh = "\xEF\x80\x98";  // fa-refresh

        // Light/Visibility
        constexpr const char *Sun = "\xEF\x86\x85";       // fa-sun
        constexpr const char *Moon = "\xEF\x86\x86";      // fa-moon
        constexpr const char *Lightbulb = "\xEF\x83\xAB"; // fa-lightbulb

        // Navigation
        constexpr const char *ChevronRight = "\xEF\x81\xB4"; // fa-chevron-right
        constexpr const char *ChevronLeft = "\xEF\x81\xB3";  // fa-chevron-left
        constexpr const char *Home = "\xEF\x80\x95";         // fa-home

        // Info/Help
        constexpr const char *Info = "\xEF\x81\xA1";     // fa-info-circle
        constexpr const char *Warning = "\xEF\x81\xB1";  // fa-exclamation-triangle
        constexpr const char *Error = "\xEF\x81\xB2";    // fa-times-circle
        constexpr const char *Question = "\xEF\x82\x9D"; // fa-question-circle

        // Hierarchy/Tree
        constexpr const char *Folder = "\xEF\x81\xBB";     // fa-folder
        constexpr const char *FolderOpen = "\xEF\x81\xBC"; // fa-folder-open
        constexpr const char *Cube = "\xEF\x87\x8B";       // fa-cube
        constexpr const char *Layer = "\xEF\x87\x82";      // fa-layer-group

        // Animation
        constexpr const char *Film = "\xEF\x80\x88";  // fa-film
        constexpr const char *Clock = "\xEF\x80\x97"; // fa-clock

        // Helpers
        inline std::string Icon(const char *iconChar, const char *label = "")
        {
            return std::string(iconChar) + (label && label[0] ? std::string(" ") + label : "");
        }

        inline std::string IconOnly(const char *iconChar)
        {
            return std::string(iconChar);
        }
    }
}
