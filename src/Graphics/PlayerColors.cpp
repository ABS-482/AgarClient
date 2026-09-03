#include "PlayerColors.h"

#include <array>

namespace
{
    constexpr std::array<RGB, 139> colors =
    { {
        {255, 255, 255}, // playercolors0
        {0, 51, 102}, // playercolors1
        {51, 102, 153}, // playercolors2
        {51, 102, 204}, // playercolors3
        {0, 51, 153}, // playercolors4
        {0, 0, 153}, // playercolors5
        {0, 0, 204}, // playercolors6
        {0, 0, 102}, // playercolors7
        {0, 102, 102}, // playercolors8
        {0, 102, 153}, // playercolors9
        {0, 153, 204}, // playercolors10
        {0, 102, 204}, // playercolors11
        {0, 51, 204}, // playercolors12
        {0, 0, 255}, // playercolors13
        {51, 51, 255}, // playercolors14
        {51, 51, 153}, // playercolors15
        {102, 153, 153}, // playercolors16
        {0, 153, 153}, // playercolors17
        {51, 204, 204}, // playercolors18
        {0, 204, 255}, // playercolors19
        {0, 153, 255}, // playercolors20
        {0, 102, 255}, // playercolors21
        {51, 102, 255}, // playercolors22
        {51, 51, 204}, // playercolors23
        {102, 102, 153}, // playercolors24
        {51, 153, 102}, // playercolors25
        {0, 204, 153}, // playercolors26
        {0, 255, 204}, // playercolors27
        {0, 255, 255}, // playercolors28
        {51, 204, 255}, // playercolors29
        {51, 153, 255}, // playercolors30
        {102, 153, 255}, // playercolors31
        {102, 102, 255}, // playercolors32
        {102, 0, 255}, // playercolors33
        {102, 0, 204}, // playercolors34
        {51, 153, 51}, // playercolors35
        {0, 204, 102}, // playercolors36
        {0, 255, 153}, // playercolors37
        {102, 255, 204}, // playercolors38
        {102, 255, 255}, // playercolors39
        {102, 204, 255}, // playercolors40
        {153, 204, 255}, // playercolors41
        {153, 153, 255}, // playercolors42
        {153, 102, 255}, // playercolors43
        {153, 51, 255}, // playercolors44
        {153, 0, 255}, // playercolors45
        {0, 102, 0}, // playercolors46
        {0, 204, 0}, // playercolors47
        {0, 255, 0}, // playercolors48
        {102, 255, 153}, // playercolors49
        {153, 255, 204}, // playercolors50
        {204, 255, 255}, // playercolors51
        {204, 204, 255}, // playercolors52
        {204, 153, 255}, // playercolors53
        {204, 102, 255}, // playercolors54
        {204, 51, 255}, // playercolors55
        {204, 0, 255}, // playercolors56
        {153, 0, 204}, // playercolors57
        {0, 51, 0}, // playercolors58
        {0, 153, 51}, // playercolors59
        {51, 204, 51}, // playercolors60
        {102, 255, 102}, // playercolors61
        {153, 255, 153}, // playercolors62
        {204, 255, 204}, // playercolors63
        {255, 204, 255}, // playercolors64
        {255, 153, 255}, // playercolors65
        {255, 102, 255}, // playercolors66
        {255, 0, 255}, // playercolors67
        {204, 0, 204}, // playercolors68
        {102, 0, 102}, // playercolors69
        {51, 102, 0}, // playercolors70
        {0, 153, 0}, // playercolors71
        {102, 255, 51}, // playercolors72
        {153, 255, 102}, // playercolors73
        {204, 255, 153}, // playercolors74
        {255, 255, 204}, // playercolors75
        {255, 204, 204}, // playercolors76
        {255, 153, 204}, // playercolors77
        {255, 102, 204}, // playercolors78
        {255, 51, 204}, // playercolors79
        {204, 0, 153}, // playercolors80
        {153, 51, 153}, // playercolors81
        {51, 51, 0}, // playercolors82
        {102, 153, 0}, // playercolors83
        {153, 255, 51}, // playercolors84
        {204, 255, 102}, // playercolors85
        {255, 255, 153}, // playercolors86
        {255, 204, 153}, // playercolors87
        {255, 153, 153}, // playercolors88
        {255, 102, 153}, // playercolors89
        {255, 51, 153}, // playercolors90
        {204, 51, 153}, // playercolors91
        {153, 0, 153}, // playercolors92
        {102, 102, 51}, // playercolors93
        {153, 204, 0}, // playercolors94
        {204, 255, 51}, // playercolors95
        {255, 255, 102}, // playercolors96
        {255, 204, 102}, // playercolors97
        {255, 153, 102}, // playercolors98
        {255, 102, 102}, // playercolors99
        {255, 0, 102}, // playercolors100
        {204, 102, 153}, // playercolors101
        {153, 51, 102}, // playercolors102
        {153, 153, 102}, // playercolors103
        {204, 204, 0}, // playercolors104
        {255, 255, 0}, // playercolors105
        {255, 204, 0}, // playercolors106
        {255, 153, 51}, // playercolors107
        {255, 102, 0}, // playercolors108
        {255, 80, 80}, // playercolors109
        {204, 0, 102}, // playercolors110
        {102, 0, 51}, // playercolors111
        {153, 102, 51}, // playercolors112
        {204, 153, 0}, // playercolors113
        {255, 153, 0}, // playercolors114
        {204, 102, 0}, // playercolors115
        {255, 51, 0}, // playercolors116
        {255, 0, 0}, // playercolors117
        {204, 0, 0}, // playercolors118
        {153, 0, 51}, // playercolors119
        {102, 51, 0}, // playercolors120
        {153, 102, 0}, // playercolors121
        {204, 51, 0}, // playercolors122
        {153, 51, 0}, // playercolors123
        {153, 0, 0}, // playercolors124
        {128, 0, 0}, // playercolors125
        {153, 51, 51}, // playercolors126
        {235, 75, 0}, // playercolors127
        {225, 125, 255}, // playercolors128
        {180, 7, 20}, // playercolors129
        {80, 170, 240}, // playercolors130
        {180, 90, 135}, // playercolors131
        {195, 240, 0}, // playercolors132
        {150, 18, 255}, // playercolors133
        {80, 245, 0}, // playercolors134
        {165, 25, 0}, // playercolors135
        {80, 145, 0}, // playercolors136
        {80, 170, 240}, // playercolors137
        {55, 92, 255}, // playercolors138
    } };
}

RGB getPlayerColor(uint8_t colorIndex)
{
    // playercolors255 в оригинале — особый случай, не входит
    // в основной непрерывный диапазон 0..138.
    if (colorIndex == 255)
        return { 0, 255, 0 };

    if (colorIndex >= colors.size())
        return { 255, 0, 0 }; // заметный "ошибочный" цвет — сигнал, что индекс не найден

    return colors[colorIndex];
}