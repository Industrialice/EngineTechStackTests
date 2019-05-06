#include "BasicHeader.hpp"
#include "VirtualKeys.hpp"

namespace EngineCore
{
    auto GetPlatformMapping() -> const array<KeyCode, 256> &
    {
        static const array<KeyCode, 256> mapping =
        {
            /*    0 none  */  KeyCode::Undefined,
            /*    1 VK_LBUTTON  */  KeyCode::MButton0,
            /*    2 VK_RBUTTON  */  KeyCode::MButton1,
            /*    3 none  */  KeyCode::Undefined,
            /*    4 VK_MBUTTON  */  KeyCode::MButton2,
            /*    5 none  */  KeyCode::Undefined,
            /*    6 none  */  KeyCode::Undefined,
            /*    7 none  */  KeyCode::Undefined,
            /*    8 VK_BACK  */  KeyCode::Backspace,
            /*    9 VK_TAB  */  KeyCode::Tab,
            /*   10 none  */  KeyCode::Undefined,
            /*   11 none  */  KeyCode::Undefined,
            /*   12 none  */  KeyCode::Undefined,
            /*   13 VK_RETURN  */  KeyCode::LEnter,
            /*   14 none  */  KeyCode::Undefined,
            /*   15 none  */  KeyCode::Undefined,
            /*   16 VK_SHIFT  */  KeyCode::LShift,
            /*   17 VK_CONTROL  */  KeyCode::LControl,
            /*   18 VK_MENU  */  KeyCode::LAlt,
            /*   19 VK_PAUSE  */  KeyCode::Pause,
            /*   20 VK_CAPITAL  */  KeyCode::CapsLock,
            /*   21 none  */  KeyCode::Undefined,
            /*   22 none  */  KeyCode::Undefined,
            /*   23 none  */  KeyCode::Undefined,
            /*   24 none  */  KeyCode::Undefined,
            /*   25 none  */  KeyCode::Undefined,
            /*   26 none  */  KeyCode::Undefined,
            /*   27 VK_ESCAPE  */  KeyCode::Escape,
            /*   28 none  */  KeyCode::Undefined,
            /*   29 none  */  KeyCode::Undefined,
            /*   30 none  */  KeyCode::Undefined,
            /*   31 none  */  KeyCode::Undefined,
            /*   32 VK_SPACE  */  KeyCode::Space,
            /*   33 VK_PRIOR  */  KeyCode::PageUp,
            /*   34 VK_NEXT  */  KeyCode::PageDown,
            /*   35 VK_END  */  KeyCode::End,
            /*   36 VK_HOME  */  KeyCode::Home,
            /*   37 VK_LEFT  */  KeyCode::LeftArrow,
            /*   38 VK_UP  */  KeyCode::UpArrow,
            /*   39 VK_RIGHT  */  KeyCode::RightArrow,
            /*   40 VK_DOWN  */  KeyCode::DownArrow,
            /*   41 none  */  KeyCode::Undefined,
            /*   42 VK_PRINT  */  KeyCode::PrintScreen,
            /*   43 none  */  KeyCode::Undefined,
            /*   44 none  */  KeyCode::Undefined,
            /*   45 VK_INSERT  */  KeyCode::Insert,
            /*   46 VK_DELETE  */  KeyCode::LDelete,
            /*   47 none  */  KeyCode::Undefined,
            /*   48    0  */  KeyCode::Digit0,
            /*   49    1  */  KeyCode::Digit1,
            /*   50    2  */  KeyCode::Digit2,
            /*   51    3  */  KeyCode::Digit3,
            /*   52    4  */  KeyCode::Digit4,
            /*   53    5  */  KeyCode::Digit5,
            /*   54    6  */  KeyCode::Digit6,
            /*   55    7  */  KeyCode::Digit7,
            /*   56    8  */  KeyCode::Digit8,
            /*   57    9  */  KeyCode::Digit9,
            /*   58 none  */  KeyCode::Undefined,
            /*   59 none  */  KeyCode::Undefined,
            /*   60 none  */  KeyCode::Undefined,
            /*   61 none  */  KeyCode::Undefined,
            /*   62 none  */  KeyCode::Undefined,
            /*   63 none  */  KeyCode::Undefined,
            /*   64 none  */  KeyCode::Undefined,
            /*   65    A  */  KeyCode::A,
            /*   66    B  */  KeyCode::B,
            /*   67    C  */  KeyCode::C,
            /*   68    D  */  KeyCode::D,
            /*   69    E  */  KeyCode::E,
            /*   70    F  */  KeyCode::F,
            /*   71    G  */  KeyCode::G,
            /*   72    H  */  KeyCode::H,
            /*   73    I  */  KeyCode::I,
            /*   74    J  */  KeyCode::J,
            /*   75    K  */  KeyCode::K,
            /*   76    L  */  KeyCode::L,
            /*   77    M  */  KeyCode::M,
            /*   78    N  */  KeyCode::N,
            /*   79    O  */  KeyCode::O,
            /*   80    P  */  KeyCode::P,
            /*   81    Q  */  KeyCode::Q,
            /*   82    R  */  KeyCode::R,
            /*   83    S  */  KeyCode::S,
            /*   84    T  */  KeyCode::T,
            /*   85    U  */  KeyCode::U,
            /*   86    V  */  KeyCode::V,
            /*   87    W  */  KeyCode::W,
            /*   88    X  */  KeyCode::X,
            /*   89    Y  */  KeyCode::Y,
            /*   90    Z  */  KeyCode::Z,
            /*   91 VK_LWIN  */  KeyCode::LSystem,
            /*   92 VK_RWIN  */  KeyCode::RSystem,
            /*   93 none  */  KeyCode::Undefined,
            /*   94 none  */  KeyCode::Undefined,
            /*   95 none  */  KeyCode::Undefined,
            /*   96 VK_NUMPAD0  */  KeyCode::NPad0,
            /*   97 VK_NUMPAD1  */  KeyCode::NPad1,
            /*   98 VK_NUMPAD2  */  KeyCode::NPad2,
            /*   99 VK_NUMPAD3  */  KeyCode::NPad3,
            /*  100 VK_NUMPAD4  */  KeyCode::NPad4,
            /*  101 VK_NUMPAD5  */  KeyCode::NPad5,
            /*  102 VK_NUMPAD6  */  KeyCode::NPad6,
            /*  103 VK_NUMPAD7  */  KeyCode::NPad7,
            /*  104 VK_NUMPAD8  */  KeyCode::NPad8,
            /*  105 VK_NUMPAD9  */  KeyCode::NPad9,
            /*  106 VK_MULTIPLY  */  KeyCode::NPadMul,
            /*  107 VK_ADD  */  KeyCode::NPadPlus,
            /*  108 none  */  KeyCode::Undefined,
            /*  109 VK_SUBTRACT  */  KeyCode::NPadMinus,
            /*  110 right delete  */  KeyCode::RDelete,
            /*  111 VK_DIVIDE  */  KeyCode::NPadDiv,
            /*  112 VK_F1  */  KeyCode::F1,
            /*  113 VK_F2  */  KeyCode::F2,
            /*  114 VK_F3  */  KeyCode::F3,
            /*  115 VK_F4  */  KeyCode::F4,
            /*  116 VK_F5  */  KeyCode::F5,
            /*  117 VK_F6  */  KeyCode::F6,
            /*  118 VK_F7  */  KeyCode::F7,
            /*  119 VK_F8  */  KeyCode::F8,
            /*  120 VK_F9  */  KeyCode::F9,
            /*  121 VK_F10  */  KeyCode::F10,
            /*  122 VK_F11  */  KeyCode::F11,
            /*  123 VK_F12  */  KeyCode::F12,
            /*  124 VK_F13  */  KeyCode::Undefined,
            /*  125 VK_F14  */  KeyCode::Undefined,
            /*  126 VK_F15  */  KeyCode::Undefined,
            /*  127 VK_F16  */  KeyCode::Undefined,
            /*  128 VK_F17  */  KeyCode::Undefined,
            /*  129 VK_F18  */  KeyCode::Undefined,
            /*  130 VK_F19  */  KeyCode::Undefined,
            /*  131 VK_F20  */  KeyCode::Undefined,
            /*  132 VK_F21  */  KeyCode::Undefined,
            /*  133 VK_F22  */  KeyCode::Undefined,
            /*  134 VK_F23  */  KeyCode::Undefined,
            /*  135 VK_F24  */  KeyCode::Undefined,
            /*  136 none  */  KeyCode::Undefined,
            /*  137 none  */  KeyCode::Undefined,
            /*  138 none  */  KeyCode::Undefined,
            /*  139 none  */  KeyCode::Undefined,
            /*  140 none  */  KeyCode::Undefined,
            /*  141 none  */  KeyCode::Undefined,
            /*  142 none  */  KeyCode::Undefined,
            /*  143 none  */  KeyCode::Undefined,
            /*  144 VK_NUMLOCK  */  KeyCode::NumLock,
            /*  145 VK_SCROLL  */  KeyCode::ScrollLock,
            /*  146 none  */  KeyCode::Undefined,
            /*  147 none  */  KeyCode::Undefined,
            /*  148 none  */  KeyCode::Undefined,
            /*  149 none  */  KeyCode::Undefined,
            /*  150 none  */  KeyCode::Undefined,
            /*  151 none  */  KeyCode::Undefined,
            /*  152 none  */  KeyCode::Undefined,
            /*  153 none  */  KeyCode::Undefined,
            /*  154 none  */  KeyCode::Undefined,
            /*  155 none  */  KeyCode::Undefined,
            /*  156 none  */  KeyCode::Undefined,
            /*  157 none  */  KeyCode::Undefined,
            /*  158 none  */  KeyCode::Undefined,
            /*  159 none  */  KeyCode::Undefined,
            /*  160 VK_LSHIFT  */  KeyCode::LShift,
            /*  161 VK_RSHIFT  */  KeyCode::RShift,
            /*  162 VK_LCONTROL  */  KeyCode::LControl,
            /*  163 VK_RCONTROL  */  KeyCode::RControl,
            /*  164 VK_LMENU  */  KeyCode::LAlt,
            /*  165 VK_RMENU  */  KeyCode::RAlt,
            /*  166 none  */  KeyCode::Undefined,
            /*  167 none  */  KeyCode::Undefined,
            /*  168 none  */  KeyCode::Undefined,
            /*  169 none  */  KeyCode::Undefined,
            /*  170 none  */  KeyCode::Undefined,
            /*  171 none  */  KeyCode::Undefined,
            /*  172 none  */  KeyCode::Undefined,
            /*  173 none  */  KeyCode::Undefined,
            /*  174 none  */  KeyCode::Undefined,
            /*  175 none  */  KeyCode::Undefined,
            /*  176 none  */  KeyCode::Undefined,
            /*  177 none  */  KeyCode::Undefined,
            /*  178 none  */  KeyCode::Undefined,
            /*  179 none  */  KeyCode::Undefined,
            /*  180 none  */  KeyCode::Undefined,
            /*  181 none  */  KeyCode::Undefined,
            /*  182 none  */  KeyCode::Undefined,
            /*  183 none  */  KeyCode::Undefined,
            /*  184 none  */  KeyCode::Undefined,
            /*  185 none  */  KeyCode::Undefined,
            /*  186 VK_OEM_1  */  KeyCode::OEM1,
            /*  187 VK_OEM_PLUS  */  KeyCode::Plus,
            /*  188 VK_OEM_COMMA  */  KeyCode::Comma,
            /*  189 VK_OEM_MINUS  */  KeyCode::Minus,
            /*  190 VK_OEM_PERIOD  */  KeyCode::Dot,
            /*  191 VK_OEM_2  */  KeyCode::OEM2,
            /*  192 VK_OEM_3  */  KeyCode::OEM3,
            /*  193 none  */  KeyCode::Undefined,
            /*  194 none  */  KeyCode::Undefined,
            /*  195 none  */  KeyCode::Undefined,
            /*  196 none  */  KeyCode::Undefined,
            /*  197 none  */  KeyCode::Undefined,
            /*  198 none  */  KeyCode::Undefined,
            /*  199 none  */  KeyCode::Undefined,
            /*  200 none  */  KeyCode::Undefined,
            /*  201 none  */  KeyCode::Undefined,
            /*  202 none  */  KeyCode::Undefined,
            /*  203 none  */  KeyCode::Undefined,
            /*  204 none  */  KeyCode::Undefined,
            /*  205 none  */  KeyCode::Undefined,
            /*  206 none  */  KeyCode::Undefined,
            /*  207 none  */  KeyCode::Undefined,
            /*  208 none  */  KeyCode::Undefined,
            /*  209 none  */  KeyCode::Undefined,
            /*  210 none  */  KeyCode::Undefined,
            /*  211 none  */  KeyCode::Undefined,
            /*  212 none  */  KeyCode::Undefined,
            /*  213 none  */  KeyCode::Undefined,
            /*  214 none  */  KeyCode::Undefined,
            /*  215 none  */  KeyCode::Undefined,
            /*  216 none  */  KeyCode::Undefined,
            /*  217 none  */  KeyCode::Undefined,
            /*  218 none  */  KeyCode::Undefined,
            /*  219 VK_OEM_4  */  KeyCode::OEM4,
            /*  220 VK_OEM_5  */  KeyCode::OEM5,
            /*  221 VK_OEM_6  */  KeyCode::OEM6,
            /*  222 VK_OEM_7  */  KeyCode::OEM7,
            /*  223 none  */  KeyCode::Undefined,
            /*  224 none  */  KeyCode::Undefined,
            /*  225 none  */  KeyCode::Undefined,
            /*  226 none  */  KeyCode::Undefined,
            /*  227 none  */  KeyCode::Undefined,
            /*  228 none  */  KeyCode::Undefined,
            /*  229 none  */  KeyCode::Undefined,
            /*  230 none  */  KeyCode::Undefined,
            /*  231 none  */  KeyCode::Undefined,
            /*  232 none  */  KeyCode::Undefined,
            /*  233 none  */  KeyCode::Undefined,
            /*  234 none  */  KeyCode::Undefined,
            /*  235 none  */  KeyCode::Undefined,
            /*  236 none  */  KeyCode::Undefined,
            /*  237 none  */  KeyCode::Undefined,
            /*  238 none  */  KeyCode::Undefined,
            /*  239 none  */  KeyCode::Undefined,
            /*  240 none  */  KeyCode::Undefined,
            /*  241 none  */  KeyCode::Undefined,
            /*  242 none  */  KeyCode::Undefined,
            /*  243 none  */  KeyCode::Undefined,
            /*  244 none  */  KeyCode::Undefined,
            /*  245 none  */  KeyCode::Undefined,
            /*  246 none  */  KeyCode::Undefined,
            /*  247 none  */  KeyCode::Undefined,
            /*  248 none  */  KeyCode::Undefined,
            /*  249 none  */  KeyCode::Undefined,
            /*  250 none  */  KeyCode::Undefined,
            /*  251 none  */  KeyCode::Undefined,
            /*  252 none  */  KeyCode::Undefined,
            /*  253 none  */  KeyCode::Undefined,
            /*  254 none  */  KeyCode::Undefined,
            /*  255 none  */  KeyCode::Undefined
        };

        return mapping;
    }
}