// raiden_main.cpp - 雷电将军（雷电影）主题 UI
//
// 视觉：稻妻和风 / 紫金白 / 金色新月弧 / 菱纹 / 宣纸面板
// 业务：复用 hana 的 hw_bridge（ReadProcessMemory 数据层，不注入不打补丁）
//
// 字体修复：上一版「寂」「鸣」等字显示成 ?，原因是它们不在
// GetGlyphRangesChineseSimplifiedCommon() 的 2500 常用字表里。
// 现在把所有 UI 文案集中到 Txt 命名空间，构建字形范围时整段扫描，
// 文案里出现的字一定进图集。

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <math.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <stdio.h>
#include <string.h>

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include "hw_bridge.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define IDI_APPICON 101

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// ============================================================ 色板
struct Rgba { float r, g, b, a; };

static inline ImU32 C(const Rgba& c, float k = 1.f)
{
    float a = c.a * k;
    if (a < 0.f) a = 0.f;
    if (a > 1.f) a = 1.f;
    int R = (int)(c.r * 255.f + .5f), G = (int)(c.g * 255.f + .5f), B = (int)(c.b * 255.f + .5f);
    return IM_COL32(R < 0 ? 0 : (R > 255 ? 255 : R), G < 0 ? 0 : (G > 255 ? 255 : G),
                    B < 0 ? 0 : (B > 255 ? 255 : B), (int)(a * 255.f + .5f));
}

struct RaidenSkin {
    Rgba ink0, ink1, ink2;
    Rgba violet, violetHi, violetGlow;
    Rgba gold, goldHi, goldDim;
    Rgba lilac, lilacDim;
    Rgba paper, paperEdge, paperInk;
    Rgba text, text2, text3;
    Rgba good, warn, danger;
};

// 色板自参考立绘采样（见 assets/README.md）
static const RaidenSkin R = {
    {0.063f, 0.047f, 0.094f, 1.f},
    {0.106f, 0.082f, 0.153f, 1.f},
    {0.165f, 0.129f, 0.239f, 1.f},
    {0.596f, 0.435f, 0.729f, 1.f},
    {0.760f, 0.616f, 0.878f, 1.f},
    {0.639f, 0.478f, 0.878f, 1.f},
    {0.941f, 0.847f, 0.659f, 1.f},
    {0.996f, 0.949f, 0.804f, 1.f},
    {0.702f, 0.596f, 0.400f, 1.f},
    {0.847f, 0.706f, 0.753f, 1.f},
    {0.678f, 0.545f, 0.643f, 1.f},
    {0.961f, 0.949f, 0.976f, 1.f},
    {0.878f, 0.855f, 0.906f, 1.f},
    {0.180f, 0.141f, 0.251f, 1.f},
    {0.965f, 0.953f, 0.988f, 1.f},
    {0.761f, 0.714f, 0.827f, 1.f},
    {0.510f, 0.463f, 0.596f, 1.f},
    {0.502f, 0.851f, 0.702f, 1.f},
    {0.973f, 0.788f, 0.435f, 1.f},
    {0.925f, 0.416f, 0.475f, 1.f},
};

// ============================================================ 全部文案
// 集中管理：改文案只动这里，字体构建也自动覆盖。
namespace Txt {
const char* kBrand     = "雷电将军";
const char* kBrandEn   = "RAIDEN SHOGUN  /  TRAINER";
const char* kNav[4]     = {"角色", "批量", "皮肤", "关于"};
const char* kNavEn[4]   = {"ROSTER", "BULK", "SKINS", "ABOUT"};
const char* kRoster    = "角色列表";
const char* kConnect   = "连接并扫描";
const char* kRescan    = "重新扫描";
const char* kRestore   = "恢复";
const char* kAttrs     = "属性";
const char* kWriteRow  = "写入本行";
const char* kRestoreOne= "恢复原值";
const char* kEmptyBig  = "静待雷鸣";
const char* kEmptySub  = "先启动游戏，再点左侧「连接并扫描」";
const char* kHint      = "选中角色后自动填入当前值，只改要改的项再写入。";
const char* kBulkTitle = "雷罚一式";
const char* kBulkSub   = "把全部角色的等级、生命与五维一次写成同一个值。";
const char* kBulkGo    = "全部拉满";
const char* kBulkYes   = "确认写入";
const char* kBulkNo    = "取消";
const char* kBulkWarn  = "会覆盖全部角色，原始快照仍可恢复";
const char* kFields    = "覆盖字段";
const char* kNotes     = "注记";
const char* kSkinTitle = "换装";
const char* kSkinApply = "应用换装";
const char* kSkinPick  = "先扫描并选中一位角色。";
const char* kAbout     = "关于";
const char* kAboutBig  = "雷电将军 · 修改器";
const char* kAboutPoem = "「一刀断绝，万世皆空。」";
const char* kIdleDot   = "寂无声";
const char* kLiveDot   = "雷光相应";
const char* kFooter    = "RAIDEN / INAZUMA";
const char* kFieldCn[8] = {"等级", "当前生命", "最大生命", "力量", "技巧", "智力", "体质", "敏捷"};
const char* kFieldEn[8] = {"LEVEL", "HP NOW", "HP MAX", "STR", "DEX", "INT", "CON", "AGI"};
const char* kNotesLines[10] = {
    "只改内存数值字段，", "不注入、不打补丁、", "不分配可执行内存，", "不会触发反作弊。", "",
    "写入后需重开游戏角色面板，", "才会刷出新显示值。", "",
    "首次扫描会自动存原始快照，", "随时可用「恢复」回滚。",
};
}

// ============================================================ 字体
static ImFont* fUi = nullptr;
static ImFont* fSm = nullptr;
static ImFont* fLg = nullptr;
static ImFont* fXl = nullptr;
static ImFont* fSerif = nullptr;
static ImFont* fCaps = nullptr;
static ImVector<ImWchar> g_cnRanges;

// 把 UI 全部文案拼成一个字符串，供字形构建扫描
static void CollectText(char* out, int cap)
{
    out[0] = 0;
    size_t used = 0;
    auto add = [&](const char* s) {
        if (!s) return;
        const size_t n = strlen(s);
        if (used + n + 1 < (size_t)cap) {
            memcpy(out + used, s, n);
            used += n;
            out[used] = 0;
        }
    };
    add(Txt::kBrand); add(Txt::kBrandEn);
    for (int i = 0; i < 4; i++) { add(Txt::kNav[i]); add(Txt::kNavEn[i]); }
    add(Txt::kRoster); add(Txt::kConnect); add(Txt::kRescan); add(Txt::kRestore);
    add(Txt::kAttrs); add(Txt::kWriteRow); add(Txt::kRestoreOne);
    add(Txt::kEmptyBig); add(Txt::kEmptySub); add(Txt::kHint);
    add(Txt::kBulkTitle); add(Txt::kBulkSub); add(Txt::kBulkGo); add(Txt::kBulkYes);
    add(Txt::kBulkNo); add(Txt::kBulkWarn); add(Txt::kFields); add(Txt::kNotes);
    add(Txt::kSkinTitle); add(Txt::kSkinApply); add(Txt::kSkinPick);
    add(Txt::kAbout); add(Txt::kAboutBig); add(Txt::kAboutPoem);
    add(Txt::kIdleDot); add(Txt::kLiveDot); add(Txt::kFooter);
    for (int i = 0; i < 8; i++) { add(Txt::kFieldCn[i]); add(Txt::kFieldEn[i]); }
    for (int i = 0; i < 10; i++) add(Txt::kNotesLines[i]);
    // 运行时拼出来的状态字
    add("扫描完成位角色已写入请重开游戏面板换装界面恢复该原始值全部失败没有快照"
        "尚未连接先选中一个未找到启动管理员权限越界皮肤正整数输入选择无名当前"
        "静候再点左侧连接");
}

static void BuildCnRanges()
{
    g_cnRanges.clear();
    char all[4096];
    CollectText(all, sizeof(all));

    ImFontGlyphRangesBuilder b;
    b.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesDefault());
    b.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
    b.AddText(all);   // 关键：文案里每个字都进图集
    b.BuildRanges(&g_cnRanges);
}

static ImFont* LoadFont(const char* path, float px, const ImWchar* ranges, int idx = 0)
{
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) return nullptr;
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = false;
    cfg.FontNo = idx;
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(path, px, &cfg, ranges);
}

static void LoadFonts(float base)
{
    ImGuiIO& io = ImGui::GetIO();
    BuildCnRanges();
    const ImWchar* cn = g_cnRanges.Data;

    const char* yahei = "C:\\Windows\\Fonts\\msyh.ttc";
    const char* serif = "C:\\Windows\\Fonts\\STZHONGS.TTF";
    const char* song  = "C:\\Windows\\Fonts\\simsun.ttc";

    fUi = LoadFont(yahei, base, cn);
    if (!fUi) fUi = io.Fonts->AddFontDefault();

    fSm = LoadFont(yahei, base - 2.5f, cn);
    if (!fSm) fSm = fUi;
    fLg = LoadFont(yahei, base + 6.f, cn);
    if (!fLg) fLg = fUi;

    fXl = LoadFont(serif, base + 22.f, cn);
    if (!fXl) fXl = LoadFont(song, base + 22.f, cn);
    if (!fXl) fXl = fLg;

    fSerif = LoadFont(serif, base + 1.f, cn);
    if (!fSerif) fSerif = LoadFont(song, base + 1.f, cn);
    if (!fSerif) fSerif = fLg;

    fCaps = LoadFont("C:\\Windows\\Fonts\\consola.ttf", base - 2.f,
                     io.Fonts->GetGlyphRangesDefault());
    if (!fCaps) fCaps = fSm;
}

// ============================================================ 背景纹理
static ID3D11ShaderResourceView* g_bgSRV = nullptr;
static ImTextureID g_bgTex = 0;
static int g_bgW = 0, g_bgH = 0;

static bool LoadBg(const char* path)
{
    int w = 0, h = 0, n = 0;
    unsigned char* px = stbi_load(path, &w, &h, &n, 4);
    if (!px) return false;

    extern ID3D11Device* g_dev;

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = (UINT)w;
    td.Height = (UINT)h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = px;
    sd.SysMemPitch = (UINT)w * 4;

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = g_dev->CreateTexture2D(&td, &sd, &tex);
    stbi_image_free(px);
    if (FAILED(hr) || !tex) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC sv = {};
    sv.Format = td.Format;
    sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sv.Texture2D.MipLevels = 1;
    hr = g_dev->CreateShaderResourceView(tex, &sv, &g_bgSRV);
    tex->Release();
    if (FAILED(hr)) return false;

    g_bgTex = (ImTextureID)(intptr_t)g_bgSRV;
    g_bgW = w;
    g_bgH = h;
    return true;
}

// ============================================================ 工具
static const float kPi = 3.14159265358979323846f;
static float g_t = 0.f, g_dt = 0.f;

static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
static float Approach(float cur, float target, float rate)
{
    return cur + (target - cur) * (1.f - expf(-rate * (g_dt > 0.f ? g_dt : 0.016f)));
}

static void DiamondPattern(ImDrawList* dl, ImVec2 a, ImVec2 b, float cell, ImU32 col, float thick)
{
    if (cell < 4.f) return;
    dl->PushClipRect(a, b, true);
    for (float x = a.x - (b.y - a.y); x < b.x + (b.y - a.y); x += cell) {
        dl->AddLine(ImVec2(x, a.y), ImVec2(x + (b.y - a.y), b.y), col, thick);
        dl->AddLine(ImVec2(x, a.y), ImVec2(x - (b.y - a.y), b.y), col, thick);
    }
    dl->PopClipRect();
}

static void GoldArc(ImDrawList* dl, ImVec2 center, float radius, float a0, float a1,
                    ImU32 col, float thick)
{
    const int seg = 64;
    dl->PathClear();
    for (int i = 0; i <= seg; i++) {
        const float t = (float)i / seg;
        const float ang = Lerp(a0, a1, t);
        dl->PathLineTo(ImVec2(center.x + cosf(ang) * radius, center.y + sinf(ang) * radius));
    }
    dl->PathStroke(col, 0, thick);
}

static void LightningMotes(ImDrawList* dl, ImVec2 origin, float spread, int count, float t)
{
    for (int i = 0; i < count; i++) {
        const float ph = (float)i * 2.399f;
        const float sp = 0.35f + (float)(i % 5) * 0.13f;
        const float drift = fmodf(t * sp + ph, 6.283f);
        const float r = spread * (0.15f + 0.85f * (drift / 6.283f));
        const float ang = ph + drift * 0.7f;
        const ImVec2 p(origin.x + cosf(ang) * r, origin.y + sinf(ang) * r * 0.6f);
        const float alpha = 0.10f + 0.42f * (1.f - drift / 6.283f);
        const float rad = 0.9f + (float)(i % 3) * 0.55f;
        dl->AddCircleFilled(p, rad, C(R.goldHi, alpha), 5);
        if (i % 7 == 0)
            dl->AddCircleFilled(p, rad * 3.2f, C(R.violetGlow, alpha * 0.22f), 8);
    }
}

// ============================================================ 控件
static bool Hit(const char* id, ImVec2 a, ImVec2 b, bool* hov = nullptr, bool* act = nullptr)
{
    ImGui::SetCursorScreenPos(a);
    const ImVec2 sz(b.x - a.x > 1 ? b.x - a.x : 1, b.y - a.y > 1 ? b.y - a.y : 1);
    ImGui::PushID(id);
    const bool cl = ImGui::InvisibleButton("##h", sz);
    if (hov) *hov = ImGui::IsItemHovered();
    if (act) *act = ImGui::IsItemActive();
    ImGui::PopID();
    return cl;
}

static bool GoldButton(const char* id, ImVec2 a, ImVec2 b, const char* label,
                       bool primary, bool enabled = true)
{
    bool hov = false, act = false;
    const bool cl = enabled ? Hit(id, a, b, &hov, &act) : false;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = b.y - a.y;
    const float r = 4.f;

    if (primary) {
        if (enabled && hov) {
            for (int i = 3; i >= 1; i--)
                dl->AddRect(ImVec2(a.x - i * 1.5f, a.y - i * 1.5f),
                            ImVec2(b.x + i * 1.5f, b.y + i * 1.5f),
                            C(R.gold, 0.07f * i), r + i * 1.5f, 0, 1.f);
        }
        dl->AddRectFilled(a, b, C(R.violet, enabled ? (act ? 0.72f : 0.92f) : 0.22f), r);
        dl->AddRectFilled(ImVec2(a.x, a.y + h * .5f), b,
                          C(R.violetGlow, enabled ? (act ? 0.42f : 0.60f) : 0.14f), r);
        dl->AddRect(a, b, C(R.gold, enabled ? (hov ? 0.95f : 0.70f) : 0.22f), r, 0, hov ? 1.6f : 1.2f);
        dl->AddRect(ImVec2(a.x + 3, a.y + 3), ImVec2(b.x - 3, b.y - 3),
                    C(R.goldHi, enabled ? 0.22f : 0.06f), r * .6f, 0, 1.f);
        ImGui::PushFont(fUi);
        const ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2((a.x + b.x - ts.x) * .5f, (a.y + b.y - ts.y) * .5f),
                    C({0.996f, 0.973f, 0.929f, enabled ? 1.f : 0.40f}), label);
        ImGui::PopFont();
    } else {
        dl->AddRectFilled(a, b, C({1, 1, 1, enabled ? (hov ? 0.075f : 0.040f) : 0.015f}), r);
        dl->AddRect(a, b, C(R.lilac, enabled ? (hov ? 0.55f : 0.28f) : 0.10f), r, 0, 1.f);
        ImGui::PushFont(fUi);
        const ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2((a.x + b.x - ts.x) * .5f, (a.y + b.y - ts.y) * .5f),
                    enabled ? (hov ? C(R.text) : C(R.text2)) : C(R.text3), label);
        ImGui::PopFont();
    }
    return cl;
}

static bool PaperButton(const char* id, ImVec2 a, ImVec2 b, const char* label,
                        bool primary, bool enabled = true)
{
    bool hov = false, act = false;
    const bool cl = enabled ? Hit(id, a, b, &hov, &act) : false;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float r = 4.f;

    if (primary) {
        dl->AddRectFilled(a, b, C(R.paperInk, enabled ? (act ? 0.82f : 0.94f) : 0.10f), r);
        dl->AddRect(a, b, C(R.gold, enabled ? (hov ? 0.90f : 0.55f) : 0.15f), r, 0, hov ? 1.6f : 1.2f);
        ImGui::PushFont(fUi);
        const ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2((a.x + b.x - ts.x) * .5f, (a.y + b.y - ts.y) * .5f),
                    C({0.996f, 0.976f, 0.945f, enabled ? 1.f : 0.35f}), label);
        ImGui::PopFont();
    } else {
        dl->AddRectFilled(a, b, C(R.paperInk, enabled ? (hov ? 0.10f : 0.055f) : 0.02f), r);
        dl->AddRect(a, b, C(R.paperInk, enabled ? (hov ? 0.34f : 0.18f) : 0.08f), r, 0, 1.f);
        ImGui::PushFont(fUi);
        const ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2((a.x + b.x - ts.x) * .5f, (a.y + b.y - ts.y) * .5f),
                    enabled ? (hov ? C(R.paperInk) : C(R.lilacDim)) : C(R.paperEdge), label);
        ImGui::PopFont();
    }
    return cl;
}

static void Section(ImDrawList* dl, ImVec2 p, const char* cn, const char* en,
                    const Rgba& cnCol, const Rgba& enCol)
{
    const ImU32 lineCol = C(R.gold, 0.80f);
    dl->AddLine(ImVec2(p.x, p.y + 9.f), ImVec2(p.x + 16.f, p.y + 9.f), lineCol, 1.6f);
    dl->AddQuadFilled(ImVec2(p.x + 20.f, p.y + 9.f), ImVec2(p.x + 23.5f, p.y + 5.5f),
                      ImVec2(p.x + 27.f, p.y + 9.f), ImVec2(p.x + 23.5f, p.y + 12.5f), lineCol);

    // 必须先用衬线体量宽度，否则拿到的是默认字体的错误宽度，中英会重叠
    ImGui::PushFont(fSerif);
    const ImVec2 ts = ImGui::CalcTextSize(cn);
    dl->AddText(ImVec2(p.x + 33.f, p.y - 4.f), C(cnCol), cn);
    ImGui::PopFont();

    if (en && en[0]) {
        ImGui::PushFont(fCaps);
        dl->AddText(ImVec2(p.x + 33.f + ts.x + 10.f, p.y + 2.f), C(enCol), en);
        ImGui::PopFont();
    }
}

static bool NumInput(const char* id, ImVec2 a, ImVec2 b, int* v, bool onPaper, int step = 1)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(a, b, onPaper ? C(R.paperInk, 0.035f) : C({1, 1, 1, 0.045f}), 3.f);
    dl->AddRect(a, b, onPaper ? C(R.paperInk, 0.16f) : C(R.lilac, 0.22f), 3.f, 0, 1.f);
    dl->AddRectFilled(ImVec2(a.x + 1.f, a.y + 6.f), ImVec2(a.x + 3.f, b.y - 6.f),
                      C(R.gold, onPaper ? 0.75f : 0.55f), 1.f);

    const float btnW = 20.f, pad = 6.f;
    const float rightCol = b.x - pad - btnW - 3.f - btnW;

    ImGui::SetCursorScreenPos(ImVec2(a.x + 10.f, a.y + (b.y - a.y - 18.f) * .5f));
    ImGui::PushID(id);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    const Rgba& ink = onPaper ? R.paperInk : R.text;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ink.r, ink.g, ink.b, 1.f));
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(R.violet.r, R.violet.g, R.violet.b, 0.35f));
    ImGui::PushStyleColor(ImGuiCol_NavHighlight, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushFont(fUi);
    ImGui::SetNextItemWidth(rightCol - a.x - 10.f);
    const bool ch = ImGui::InputInt("##v", v, 0, 0, ImGuiInputTextFlags_CharsDecimal);
    ImGui::PopFont();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(6);
    ImGui::PopID();

    const float cy = a.y + (b.y - a.y) * .5f;
    const float r = btnW * .5f;
    for (int k = 0; k < 2; k++) {
        const ImVec2 c(rightCol + k * (btnW + 3.f) + r, cy);
        char bid[8];
        _snprintf_s(bid, sizeof(bid), _TRUNCATE, "s%d", k);
        ImGui::SetCursorScreenPos(ImVec2(c.x - r, c.y - r));
        ImGui::PushID(bid);
        const bool cl = ImGui::InvisibleButton("##sb", ImVec2(btnW, btnW));
        const bool hover = ImGui::IsItemHovered();
        const bool actv = ImGui::IsItemActive();
        ImGui::PopID();
        if (cl) *v += (k == 0 ? -step : step);

        dl->AddCircleFilled(c, r, hover ? C(R.violet, actv ? 0.34f : 0.22f)
                                        : (onPaper ? C(R.paperInk, 0.06f) : C({1, 1, 1, 0.06f})));
        dl->AddCircle(c, r, hover ? C(R.gold, 0.85f)
                                  : (onPaper ? C(R.paperInk, 0.18f) : C(R.lilac, 0.20f)), 18, 1.f);
        const ImU32 lc = hover ? C(R.goldHi) : (onPaper ? C(R.paperInk, 0.72f) : C(R.text2));
        dl->AddLine(ImVec2(c.x - 4.f, c.y), ImVec2(c.x + 4.f, c.y), lc, 1.3f);
        if (k == 1) dl->AddLine(ImVec2(c.x, c.y - 4.f), ImVec2(c.x, c.y + 4.f), lc, 1.3f);
    }
    return ch;
}

// ============================================================ 应用状态
struct App {
    int page = 0;
    int sel = 0;
    float navK[4] = {0, 0, 0, 0};
    float rowK[64] = {0};
    float scroll = 0.f;
    int edit[8] = {0};
    int editSkin = 0;
    int bulkVal = 999;
    bool confirmBulk = false;
    bool scanned = false;
    char status[256] = "静候。先启动游戏，再点「连接并扫描」";
    float introK = 0.f;
};
static App g;

static void SyncEdit()
{
    if (!HW_Ready()) return;
    const HeroRow* h = HW_HeroAt(g.sel);
    if (!h) return;
    g.edit[0] = (int)h->level;
    g.edit[1] = h->curHp;
    g.edit[2] = h->maxHp;
    g.edit[3] = h->str_;
    g.edit[4] = h->dex;
    g.edit[5] = h->int_;
    g.edit[6] = h->con;
    g.edit[7] = h->agi;
    g.editSkin = h->skin;
}

// ============================================================ 背景
static void DrawArtwork(ImVec2 lo, ImVec2 hi)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float W = hi.x - lo.x, H = hi.y - lo.y;

    dl->AddRectFilledMultiColor(lo, hi, C(R.ink1), C(R.ink1), C(R.ink0), C(R.ink0));

    if (g_bgTex) {
        const float imgAR = (float)g_bgW / (float)g_bgH;
        const float winAR = W / H;
        float uw, uh;
        if (winAR > imgAR) { uw = W; uh = W / imgAR; } else { uh = H; uw = H * imgAR; }
        const float zoom = 1.f + 0.012f * sinf(g_t * 0.22f);
        uw *= zoom; uh *= zoom;
        const float ox = lo.x + (W - uw) * 0.5f + sinf(g_t * 0.13f) * W * 0.004f;
        const float oy = lo.y + (H - uh) * 0.5f + cosf(g_t * 0.17f) * H * 0.005f;
        dl->AddImage(g_bgTex, ImVec2(ox, oy), ImVec2(ox + uw, oy + uh));
    }

    // 只在最左侧压暗，人物区间保持原亮度
    dl->AddRectFilledMultiColor(lo, ImVec2(lo.x + W * 0.16f, hi.y),
                                C(R.ink0, 0.90f), C(R.ink0, 0.30f), C(R.ink0, 0.30f), C(R.ink0, 0.90f));
    dl->AddRectFilledMultiColor(lo, ImVec2(hi.x, lo.y + H * 0.28f),
                                C(R.ink0, 0.72f), C(R.ink0, 0.72f), C(R.ink0, 0.f), C(R.ink0, 0.f));
    dl->AddRectFilledMultiColor(ImVec2(lo.x, hi.y - H * 0.20f), hi,
                                C(R.ink0, 0.f), C(R.ink0, 0.f), C(R.ink0, 0.88f), C(R.ink0, 0.88f));

    const ImVec2 glow(hi.x - W * 0.12f, lo.y + H * 0.10f);
    for (int i = 12; i >= 1; i--) {
        const float k = (float)i / 12.f;
        dl->AddCircleFilled(glow, 60.f + 320.f * k, C(R.violetGlow, 0.010f), 48);
    }

    // 金色新月弧：圆心在画面外，只落左侧暗部，不横穿人物
    const float breathe = 0.55f + 0.45f * sinf(g_t * 0.55f);
    {
        const ImVec2 c(lo.x - W * 0.02f, lo.y + H * 0.56f);
        const float rad = W * 0.235f;
        for (int i = 3; i >= 1; i--)
            GoldArc(dl, c, rad + i * 4.f, -0.85f, 0.85f, C(R.gold, 0.040f * i * breathe + 0.015f), 2.0f);
        GoldArc(dl, c, rad, -0.85f, 0.85f, C(R.gold, 0.34f + 0.16f * breathe), 1.6f);
    }
    {
        const ImVec2 c(lo.x + W * 1.02f, lo.y + H * 0.42f);
        const float rad = W * 0.20f;
        for (int i = 3; i >= 1; i--)
            GoldArc(dl, c, rad + i * 4.f, kPi - 0.80f, kPi + 0.80f, C(R.gold, 0.030f * i + 0.012f), 1.8f);
        GoldArc(dl, c, rad, kPi - 0.80f, kPi + 0.80f, C(R.gold, 0.26f + 0.10f * breathe), 1.3f);
    }

    LightningMotes(dl, glow, W * 0.30f, 46, g_t);

    // 高光抑制：立绘袖口纯白紧邻深紫底，落差过大会刺眼。
    // 竖向分带加权：暗部几乎不动，亮部拉低到 ~200，保留高光层次。
    {
        static const float bandW[14] = {
            0.05f, 0.14f, 0.10f, 0.07f, 0.06f, 0.09f, 0.13f,
            0.20f, 0.26f, 0.30f, 0.24f, 0.16f, 0.11f, 0.07f
        };
        const float bw = W / 14.f;
        for (int i = 0; i < 14; i++)
            dl->AddRectFilled(ImVec2(lo.x + i * bw, lo.y),
                              ImVec2(lo.x + (i + 1) * bw + 1.f, hi.y), C(R.ink0, bandW[i]));
    }

    // 噪点
    static bool built = false;
    static ImVec2 pts[300];
    static float al[300];
    if (!built) {
        unsigned seed = 20241016u;
        for (int i = 0; i < 300; i++) {
            seed = seed * 1103515245u + 12345u; pts[i].x = (float)((seed >> 8) & 0xFFFF) / 65535.f;
            seed = seed * 1103515245u + 12345u; pts[i].y = (float)((seed >> 8) & 0xFFFF) / 65535.f;
            seed = seed * 1103515245u + 12345u; al[i] = 0.015f + ((float)((seed >> 8) & 0xFF) / 255.f) * 0.055f;
        }
        built = true;
    }
    for (int i = 0; i < 300; i++) {
        const float ph = g_t * (0.06f + (float)(i % 9) * 0.017f) + (float)i * 0.37f;
        float y = lo.y + pts[i].y * H - fmodf(ph * 7.f, H);
        if (y < lo.y) y += H;
        dl->AddCircleFilled(ImVec2(lo.x + pts[i].x * W, y), 1.0f, C({1, 1, 1, al[i]}), 4);
    }

    if (g.introK < 1.f) {
        const float k = g.introK;
        const float sweep = lo.x + W * k;
        dl->AddRectFilledMultiColor(ImVec2(sweep - 120.f, lo.y), ImVec2(sweep, hi.y),
                                    C(R.ink0, 0.f), C(R.goldHi, 0.5f * (1.f - k)),
                                    C(R.goldHi, 0.5f * (1.f - k)), C(R.ink0, 0.f));
        dl->AddLine(ImVec2(sweep, lo.y), ImVec2(sweep, hi.y), C(R.goldHi, 0.85f * (1.f - k)), 2.f);
    }
}

// ============================================================ 标题
static void DrawHeader(ImVec2 lo, ImVec2 hi, HWND hwnd)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const ImVec2 bp(lo.x + 26.f, lo.y + 20.f);
    for (int i = 3; i >= 1; i--)
        dl->AddCircleFilled(ImVec2(bp.x + 9.f, bp.y + 9.f), 9.f + i * 2.6f,
                            C(R.violetGlow, 0.05f * i), 24);
    dl->AddCircle(ImVec2(bp.x + 9.f, bp.y + 9.f), 9.f, C(R.gold, 0.85f), 24, 1.4f);
    dl->AddCircleFilled(ImVec2(bp.x + 9.f, bp.y + 9.f), 3.2f, C(R.goldHi));
    for (int k = 0; k < 3; k++) {
        const float a = g_t * 0.35f + k * (2.f * kPi / 3.f);
        const ImVec2 p1(bp.x + 9.f + cosf(a) * 4.4f, bp.y + 9.f + sinf(a) * 4.4f);
        const ImVec2 p2(bp.x + 9.f + cosf(a + 0.5f) * 7.6f, bp.y + 9.f + sinf(a + 0.5f) * 7.6f);
        dl->AddLine(p1, p2, C(R.gold, 0.75f), 1.2f);
    }

    ImGui::PushFont(fSerif);
    dl->AddText(ImVec2(bp.x + 28.f, bp.y - 3.f), C(R.text), Txt::kBrand);
    ImGui::PopFont();
    ImGui::PushFont(fCaps);
    dl->AddText(ImVec2(bp.x + 28.f, bp.y + 17.f), C(R.text3), Txt::kBrandEn);
    ImGui::PopFont();

    const float midY = hi.y - 6.f;
    dl->AddLine(ImVec2(lo.x + 26.f, midY), ImVec2(hi.x - 130.f, midY), C(R.gold, 0.28f), 1.f);
    dl->AddQuadFilled(ImVec2(hi.x - 124.f, midY), ImVec2(hi.x - 120.f, midY - 4.f),
                      ImVec2(hi.x - 116.f, midY), ImVec2(hi.x - 120.f, midY + 4.f), C(R.gold, 0.55f));

    const float bs = 26.f;
    const ImVec2 cb(hi.x - 22.f - bs, lo.y + 16.f);
    const ImVec2 mb(hi.x - 22.f - bs * 2 - 12.f, lo.y + 16.f);
    auto wbtn = [&](const char* id, ImVec2 a, bool danger) -> bool {
        bool hov = false;
        const bool cl = Hit(id, a, ImVec2(a.x + bs, a.y + bs), &hov);
        const ImVec2 c(a.x + bs * .5f, a.y + bs * .5f);
        if (hov) dl->AddCircleFilled(c, bs * .5f, C(danger ? R.danger : R.gold, 0.16f), 20);
        const ImU32 lc = hov ? C(danger ? R.danger : R.goldHi) : C(R.text3);
        if (danger) {
            dl->AddLine(ImVec2(c.x - 4.5f, c.y - 4.5f), ImVec2(c.x + 4.5f, c.y + 4.5f), lc, 1.4f);
            dl->AddLine(ImVec2(c.x + 4.5f, c.y - 4.5f), ImVec2(c.x - 4.5f, c.y + 4.5f), lc, 1.4f);
        } else {
            dl->AddLine(ImVec2(c.x - 5.5f, c.y + 2.f), ImVec2(c.x + 5.5f, c.y + 2.f), lc, 1.4f);
        }
        return cl;
    };
    if (wbtn("min", mb, false)) ShowWindow(hwnd, SW_MINIMIZE);
    if (wbtn("cls", cb, true)) PostMessage(hwnd, WM_CLOSE, 0, 0);

    if (!ImGui::IsAnyItemHovered() &&
        ImGui::IsMouseHoveringRect(lo, ImVec2(hi.x - 100.f, hi.y)) && ImGui::IsMouseClicked(0)) {
        ReleaseCapture();
        SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }
}

// ============================================================ 侧栏
static void DrawNav(ImVec2 lo, ImVec2 hi)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float itemH = 54.f;
    const float top = lo.y + 84.f;

    for (int i = 0; i < 4; i++) {
        const ImVec2 a(lo.x, top + i * (itemH + 6.f));
        const ImVec2 b(hi.x, a.y + itemH);
        bool hov = false;
        char navId[16];
        _snprintf_s(navId, sizeof(navId), _TRUNCATE, "nav%d", i);
        if (Hit(navId, a, b, &hov)) {
            g.page = i;
            if (i == 0) SyncEdit();
        }
        const bool sel = (g.page == i);
        g.navK[i] = Approach(g.navK[i], sel ? 1.f : 0.f, 15.f);
        const float k = g.navK[i];

        if (k > 0.02f) {
            dl->AddRectFilledMultiColor(a, b, C(R.violet, 0.24f * k), C(R.violet, 0.05f * k),
                                        C(R.violet, 0.05f * k), C(R.violet, 0.24f * k));
            dl->AddRectFilled(ImVec2(a.x + 10.f, a.y + 8.f), ImVec2(a.x + 12.5f, b.y - 8.f),
                              C(R.gold, k), 1.f);
            for (int j = 3; j >= 1; j--)
                dl->AddRectFilled(ImVec2(a.x + 10.f - j, a.y + 8.f),
                                  ImVec2(a.x + 12.5f + j, b.y - 8.f), C(R.gold, 0.05f * j * k), 2.f);
        } else if (hov) {
            dl->AddRectFilled(a, b, C({1, 1, 1, 0.03f}));
        }

        // 英文在上、中文在下（168px 栏宽放不下并排）
        ImGui::PushFont(fCaps);
        dl->AddText(ImVec2(a.x + 26.f, a.y + 8.f),
                    sel ? C(R.gold, 0.62f) : C(R.text3, 0.45f), Txt::kNavEn[i]);
        ImGui::PopFont();

        ImGui::PushFont(fSerif);
        dl->AddText(ImVec2(a.x + 26.f, a.y + 21.f),
                    sel ? C(R.goldHi) : (hov ? C(R.text2) : C(R.text3)), Txt::kNav[i]);
        ImGui::PopFont();
    }

    const ImVec2 sp(lo.x + 24.f, hi.y - 52.f);
    const bool ok = HW_Ready();
    if (ok)
        for (int i = 3; i >= 1; i--)
            dl->AddCircleFilled(ImVec2(sp.x + 4.f, sp.y + 4.f), 4.f + i * 2.8f, C(R.good, 0.07f * i), 12);
    dl->AddCircleFilled(ImVec2(sp.x + 4.f, sp.y + 4.f), 3.6f, ok ? C(R.good) : C(R.goldDim), 12);

    ImGui::PushFont(fSm);
    dl->AddText(ImVec2(sp.x + 16.f, sp.y - 3.f), C(R.text2), ok ? Txt::kLiveDot : Txt::kIdleDot);
    ImGui::PopFont();
    ImGui::PushFont(fCaps);
    dl->AddText(ImVec2(sp.x + 16.f, sp.y + 15.f), C(R.text3), HW_StatusLine());
    ImGui::PopFont();
}

// ============================================================ 角色页
static void PageRoster(ImVec2 lo, ImVec2 hi)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float pad = 18.f;
    const float listW = 264.f;

    const ImVec2 la(lo.x, lo.y), lb(lo.x + listW, hi.y);
    dl->AddRectFilled(ImVec2(la.x, la.y), ImVec2(lb.x - 22.f, lb.y), C(R.ink0, 0.90f));
    dl->AddRectFilledMultiColor(ImVec2(lb.x - 22.f, la.y), ImVec2(lb.x + 18.f, lb.y),
                                C(R.ink0, 0.90f), C(R.ink0, 0.00f), C(R.ink0, 0.00f), C(R.ink0, 0.90f));
    dl->AddLine(ImVec2(lb.x, la.y), ImVec2(lb.x, lb.y), C(R.gold, 0.16f), 1.f);

    Section(dl, ImVec2(la.x + 22.f, la.y + 14.f), Txt::kRoster, "ROSTER", R.gold, R.text3);

    const ImVec2 b1(la.x + 18.f, la.y + 54.f);
    if (GoldButton("scan", b1, ImVec2(b1.x + 150.f, b1.y + 34.f),
                   g.scanned ? Txt::kRescan : Txt::kConnect, true)) {
        if (HW_AttachAndScan()) {
            g.scanned = true;
            g.sel = 0;
            SyncEdit();
            _snprintf_s(g.status, sizeof(g.status), _TRUNCATE, "扫描完成：%d 位角色", HW_Count());
        } else {
            _snprintf_s(g.status, sizeof(g.status), _TRUNCATE, "%s", HW_LastError());
        }
    }
    const ImVec2 b2(b1.x + 158.f, b1.y);
    if (GoldButton("restoreAll", b2, ImVec2(b2.x + 100.f, b2.y + 34.f), Txt::kRestore,
                   false, HW_Ready())) {
        _snprintf_s(g.status, sizeof(g.status), _TRUNCATE, "%s",
                    HW_RestoreAll() ? "已恢复全部原值" : "没有原始快照");
    }

    const float rowH = 50.f;
    const ImVec2 ra(la.x + 12.f, la.y + 102.f), rb(lb.x - 12.f, lb.y - 12.f);
    dl->PushClipRect(ra, rb, true);
    const int n = HW_Count();
    for (int i = 0; i < n; i++) {
        const HeroRow* h = HW_HeroAt(i);
        if (!h) continue;
        const ImVec2 a(ra.x, ra.y + (float)i * rowH - g.scroll);
        const ImVec2 b(rb.x, a.y + rowH - 6.f);
        if (b.y < ra.y - 50.f || a.y > rb.y) continue;

        bool hov = false;
        char rk[16];
        _snprintf_s(rk, sizeof(rk), _TRUNCATE, "row%d", i);
        if (Hit(rk, a, b, &hov)) { g.sel = i; SyncEdit(); }

        const bool sel = (g.sel == i);
        g.rowK[i] = Approach(g.rowK[i], sel ? 1.f : 0.f, 16.f);
        const float k = g.rowK[i];

        dl->AddRectFilled(a, b, sel ? C(R.violet, 0.25f)
                                    : (hov ? C({1, 1, 1, 0.045f}) : C({1, 1, 1, 0.018f})), 4.f);
        if (k > 0.02f) {
            dl->AddRectFilled(ImVec2(a.x, a.y + 8.f), ImVec2(a.x + 2.5f, b.y - 8.f), C(R.gold, k), 1.f);
            dl->AddRect(a, b, C(R.gold, 0.30f * k), 4.f, 0, 1.f);
        }

        char nm[80];
        WideToUtf8(h->name, nm, sizeof(nm));
        ImGui::PushFont(fSerif);
        dl->AddText(ImVec2(a.x + 14.f, a.y + 6.f), C(sel ? R.goldHi : R.text), nm);
        ImGui::PopFont();

        char sub[96];
        ImGui::PushFont(fCaps);
        _snprintf_s(sub, sizeof(sub), _TRUNCATE, "Lv.%u", h->level);
        dl->AddText(ImVec2(a.x + 14.f, a.y + 27.f), C(sel ? R.gold : R.text3), sub);
        _snprintf_s(sub, sizeof(sub), _TRUNCATE, "%d / %d", h->curHp, h->maxHp);
        dl->AddText(ImVec2(b.x - 8.f - ImGui::CalcTextSize(sub).x, a.y + 27.f),
                    C(R.text3, sel ? 0.95f : 0.7f), sub);
        ImGui::PopFont();

        if (h->maxHp > 0) {
            float p = (float)h->curHp / (float)h->maxHp;
            if (p < 0.f) p = 0.f;
            if (p > 1.f) p = 1.f;
            const float bw = (b.x - a.x) - 28.f;
            const ImVec2 ba(a.x + 14.f, b.y - 5.f);
            dl->AddRectFilled(ba, ImVec2(ba.x + bw, ba.y + 1.8f), C(R.text3, 0.20f), 1.f);
            dl->AddRectFilled(ba, ImVec2(ba.x + bw * p, ba.y + 1.8f),
                              C(sel ? R.gold : R.goldDim, 0.85f), 1.f);
        }
    }
    ImGui::PopClipRect();

    {
        const ImVec2 mp = ImGui::GetIO().MousePos;
        if (mp.x > ra.x && mp.x < rb.x && mp.y > ra.y && mp.y < rb.y) {
            const float total = (float)n * rowH;
            const float vis = rb.y - ra.y;
            const float maxS = total - vis > 0 ? total - vis : 0.f;
            g.scroll -= ImGui::GetIO().MouseWheel * 50.f;
            if (g.scroll < 0.f) g.scroll = 0.f;
            if (g.scroll > maxS) g.scroll = maxS;
        }
    }

    // 右栏宣纸
    const float paperW = 402.f;
    const ImVec2 pa(hi.x - paperW - pad, lo.y + pad), pb(hi.x - pad, hi.y - pad);
    dl->AddRectFilled(pa, pb, C(R.paper, 0.955f), 6.f);
    dl->AddRect(pa, pb, C(R.gold, 0.55f), 6.f, 0, 1.2f);
    dl->AddRect(ImVec2(pa.x + 4.f, pa.y + 4.f), ImVec2(pb.x - 4.f, pb.y - 4.f),
                C(R.paperInk, 0.07f), 4.f, 0, 1.f);
    DiamondPattern(dl, ImVec2(pa.x + 5.f, pa.y + 5.f), ImVec2(pb.x - 5.f, pb.y - 5.f),
                   46.f, C(R.paperInk, 0.022f), 1.f);

    if (!HW_Ready() || HW_Count() == 0) {
        ImGui::PushFont(fXl);
        const ImVec2 s1 = ImGui::CalcTextSize(Txt::kEmptyBig);
        dl->AddText(ImVec2((pa.x + pb.x) * .5f - s1.x * .5f, (pa.y + pb.y) * .5f - 46.f),
                    C(R.paperInk, 0.82f), Txt::kEmptyBig);
        ImGui::PopFont();
        ImGui::PushFont(fSm);
        const ImVec2 s2 = ImGui::CalcTextSize(Txt::kEmptySub);
        dl->AddText(ImVec2((pa.x + pb.x) * .5f - s2.x * .5f, (pa.y + pb.y) * .5f + 4.f),
                    C(R.lilacDim), Txt::kEmptySub);
        ImGui::PopFont();
        return;
    }

    const HeroRow* h = HW_HeroAt(g.sel);
    if (!h) return;
    char nm[80];
    WideToUtf8(h->name, nm, sizeof(nm));

    ImGui::PushFont(fXl);
    dl->AddText(ImVec2(pa.x + 30.f, pa.y + 24.f), C(R.paperInk), nm);
    ImGui::PopFont();

    char addrbuf[40];
    _snprintf_s(addrbuf, sizeof(addrbuf), _TRUNCATE, "ADDR 0x%llX", h->addr);
    ImGui::PushFont(fCaps);
    dl->AddText(ImVec2(pa.x + 32.f, pa.y + 68.f), C(R.lilacDim), addrbuf);
    ImGui::PopFont();

    {
        const ImVec2 cc(pb.x - 78.f, pa.y + 62.f);
        const float rad = 30.f;
        float p = h->maxHp > 0 ? (float)h->curHp / (float)h->maxHp : 0.f;
        if (p < 0.f) p = 0.f;
        if (p > 1.f) p = 1.f;
        const float a0 = -kPi * .5f;
        const float a1 = a0 + kPi * 2.f * p;
        dl->AddCircle(cc, rad, C(R.paperInk, 0.10f), 64, rad * .17f);
        dl->PathClear();
        dl->PathArcTo(cc, rad, a0, a1, 48);
        dl->PathStroke(C(R.gold, 0.98f), 0, rad * .17f);
        dl->PathClear();
        dl->PathArcTo(cc, rad, a0, a1, 48);
        dl->PathStroke(C(R.goldHi, 0.35f), 0, rad * .30f);

        char vb[32];
        _snprintf_s(vb, sizeof(vb), _TRUNCATE, "%d", h->curHp);
        ImGui::PushFont(fLg);
        const ImVec2 vs = ImGui::CalcTextSize(vb);
        dl->AddText(ImVec2(cc.x - vs.x * .5f, cc.y - vs.y * .58f), C(R.paperInk), vb);
        ImGui::PopFont();
        ImGui::PushFont(fCaps);
        const ImVec2 cs = ImGui::CalcTextSize("HP");
        dl->AddText(ImVec2(cc.x - cs.x * .5f, cc.y + vs.y * .18f), C(R.lilacDim), "HP");
        ImGui::PopFont();
    }

    dl->AddLine(ImVec2(pa.x + 30.f, pa.y + 96.f), ImVec2(pb.x - 30.f, pa.y + 96.f),
                C(R.gold, 0.35f), 1.f);

    Section(dl, ImVec2(pa.x + 30.f, pa.y + 112.f), Txt::kAttrs, "ATTRIBUTES",
            R.paperInk, R.lilacDim);

    const float fw = (pb.x - pa.x - 84.f) * .5f;
    for (int i = 0; i < 8; i++) {
        const int col = i % 2, row = i / 2;
        const ImVec2 fa(pa.x + 30.f + col * (fw + 24.f), pa.y + 152.f + row * 64.f);
        const ImVec2 fb(fa.x + fw, fa.y + 38.f);

        ImGui::PushFont(fCaps);
        dl->AddText(ImVec2(fa.x + 1.f, fa.y - 14.f), C(R.lilacDim), Txt::kFieldEn[i]);
        ImGui::PopFont();
        ImGui::PushFont(fSm);
        const float ew = ImGui::CalcTextSize(Txt::kFieldEn[i]).x;
        dl->AddText(ImVec2(fa.x + 1.f + ew + 7.f, fa.y - 14.f),
                    C(R.paperInk, 0.55f), Txt::kFieldCn[i]);
        ImGui::PopFont();

        char fid[16];
        _snprintf_s(fid, sizeof(fid), _TRUNCATE, "f%d", i);
        NumInput(fid, fa, fb, &g.edit[i], true);
    }

    const ImVec2 wb(pa.x + 30.f, pb.y - 82.f);
    if (PaperButton("write", wb, ImVec2(wb.x + 140.f, wb.y + 36.f), Txt::kWriteRow, true)) {
        _snprintf_s(g.status, sizeof(g.status), _TRUNCATE, "%s",
                    HW_WriteRow(g.sel, g.edit) ? "已写入，请重开游戏面板" : "写入失败");
    }
    if (PaperButton("restoreOne", ImVec2(wb.x + 152.f, wb.y),
                    ImVec2(wb.x + 272.f, wb.y + 36.f), Txt::kRestoreOne, false)) {
        _snprintf_s(g.status, sizeof(g.status), _TRUNCATE, "%s",
                    HW_RestoreOne(g.sel) ? "已恢复该角色原值" : "没有原始快照");
    }

    ImGui::PushFont(fSm);
    dl->AddText(ImVec2(pa.x + 30.f, pb.y - 34.f), C(R.paperInk, 0.42f), Txt::kHint);
    ImGui::PopFont();
}

// ============================================================ 批量页
static void PageBulk(ImVec2 lo, ImVec2 hi)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 pa(lo.x + 18.f, lo.y + 18.f), pb(hi.x - 18.f, hi.y - 18.f);
    dl->AddRectFilled(pa, pb, C(R.ink0, 0.72f), 6.f);
    dl->AddRect(pa, pb, C(R.gold, 0.30f), 6.f, 0, 1.f);
    DiamondPattern(dl, ImVec2(pa.x + 4.f, pa.y + 4.f), ImVec2(pb.x - 4.f, pb.y - 4.f),
                   58.f, C(R.gold, 0.030f), 1.f);

    Section(dl, ImVec2(pa.x + 26.f, pa.y + 18.f), Txt::kBulkTitle, "BULK WRITE", R.gold, R.text3);

    ImGui::PushFont(fSm);
    dl->AddText(ImVec2(pa.x + 26.f, pa.y + 52.f), C(R.text2), Txt::kBulkSub);
    ImGui::PopFont();

    const ImVec2 fa(pa.x + 26.f, pa.y + 90.f), fb(fa.x + 210.f, fa.y + 40.f);
    ImGui::PushFont(fCaps);
    dl->AddText(ImVec2(fa.x + 1.f, fa.y - 14.f), C(R.text3), "VALUE");
    ImGui::PopFont();
    NumInput("bulk", fa, fb, &g.bulkVal, false, 10);

    const int quick[4] = {99, 999, 9999, 99999};
    for (int i = 0; i < 4; i++) {
        const ImVec2 a(fb.x + 26.f + i * 78.f, fa.y), b(a.x + 70.f, a.y + 40.f);
        char lb[16];
        _snprintf_s(lb, sizeof(lb), _TRUNCATE, "%d", quick[i]);
        const bool sel = (g.bulkVal == quick[i]);
        bool hov = false;
        char qk[16];
        _snprintf_s(qk, sizeof(qk), _TRUNCATE, "q%d", i);
        if (Hit(qk, a, b, &hov)) g.bulkVal = quick[i];
        dl->AddRectFilled(a, b, sel ? C(R.violet, 0.30f) : C({1, 1, 1, hov ? 0.07f : 0.035f}), 3.f);
        dl->AddRect(a, b, sel ? C(R.gold, 0.85f) : C(R.lilac, hov ? 0.40f : 0.18f), 3.f, 0, 1.f);
        ImGui::PushFont(fCaps);
        const ImVec2 ts = ImGui::CalcTextSize(lb);
        dl->AddText(ImVec2((a.x + b.x - ts.x) * .5f, (a.y + b.y - ts.y) * .5f),
                    sel ? C(R.goldHi) : C(R.text2), lb);
        ImGui::PopFont();
    }

    Section(dl, ImVec2(pa.x + 26.f, pa.y + 172.f), Txt::kFields, "FIELDS", R.gold, R.text3);
    ImGui::PushFont(fSm);
    for (int i = 0; i < 8; i++) {
        const ImVec2 p(pa.x + 26.f + (i % 4) * 118.f, pa.y + 206.f + (i / 4) * 26.f);
        dl->AddQuadFilled(ImVec2(p.x + 4.f, p.y + 1.f), ImVec2(p.x + 7.5f, p.y + 4.5f),
                          ImVec2(p.x + 4.f, p.y + 8.f), ImVec2(p.x + 0.5f, p.y + 4.5f),
                          C(R.gold, 0.8f));
        dl->AddText(ImVec2(p.x + 14.f, p.y - 2.f), C(R.text2), Txt::kFieldCn[i]);
    }
    ImGui::PopFont();

    const ImVec2 ba(pa.x + 26.f, pb.y - 96.f);
    if (!g.confirmBulk) {
        if (GoldButton("bulkGo", ba, ImVec2(ba.x + 170.f, ba.y + 40.f), Txt::kBulkGo,
                       true, HW_Ready())) g.confirmBulk = true;
    } else {
        if (GoldButton("bulkYes", ba, ImVec2(ba.x + 170.f, ba.y + 40.f), Txt::kBulkYes,
                       true, HW_Ready())) {
            _snprintf_s(g.status, sizeof(g.status), _TRUNCATE, "%s",
                        HW_ApplyAll(g.bulkVal) ? "已批量写入全部角色" : "批量写入失败");
            g.confirmBulk = false;
        }
        if (GoldButton("bulkNo", ImVec2(ba.x + 182.f, ba.y),
                       ImVec2(ba.x + 272.f, ba.y + 40.f), Txt::kBulkNo, false))
            g.confirmBulk = false;
    }
    if (g.confirmBulk) {
        ImGui::PushFont(fSm);
        dl->AddText(ImVec2(ba.x + 182.f, ba.y - 22.f), C(R.warn), Txt::kBulkWarn);
        ImGui::PopFont();
    }

    const ImVec2 ia(pb.x - 300.f, pa.y + 52.f);
    dl->AddLine(ImVec2(ia.x - 18.f, ia.y), ImVec2(ia.x - 18.f, pb.y - 40.f), C(R.gold, 0.18f), 1.f);
    Section(dl, ImVec2(ia.x, ia.y - 30.f), Txt::kNotes, "NOTES", R.gold, R.text3);
    ImGui::PushFont(fSm);
    for (int i = 0; i < 10; i++)
        dl->AddText(ImVec2(ia.x, ia.y + 6.f + i * 22.f), C(R.text3), Txt::kNotesLines[i]);
    ImGui::PopFont();
}

// ============================================================ 皮肤页
static void PageSkins(ImVec2 lo, ImVec2 hi)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 pa(lo.x + 18.f, lo.y + 18.f), pb(hi.x - 18.f, hi.y - 18.f);
    dl->AddRectFilled(pa, pb, C(R.ink0, 0.72f), 6.f);
    dl->AddRect(pa, pb, C(R.gold, 0.30f), 6.f, 0, 1.f);

    Section(dl, ImVec2(pa.x + 26.f, pa.y + 18.f), Txt::kSkinTitle, "SKINS", R.gold, R.text3);

    if (!HW_Ready() || HW_Count() == 0) {
        ImGui::PushFont(fSm);
        dl->AddText(ImVec2(pa.x + 26.f, pa.y + 56.f), C(R.text3), Txt::kSkinPick);
        ImGui::PopFont();
        return;
    }
    const HeroRow* h = HW_HeroAt(g.sel);
    if (!h) return;
    char nm[80];
    WideToUtf8(h->name, nm, sizeof(nm));

    ImGui::PushFont(fXl);
    dl->AddText(ImVec2(pa.x + 26.f, pa.y + 48.f), C(R.text), nm);
    ImGui::PopFont();

    char cur[64];
    _snprintf_s(cur, sizeof(cur), _TRUNCATE, "CURRENT  ID %u", (unsigned)h->skin);
    ImGui::PushFont(fCaps);
    dl->AddText(ImVec2(pa.x + 28.f, pa.y + 86.f), C(R.gold, 0.85f), cur);
    ImGui::PopFont();

    const int cnt = HW_SkinCount(g.sel);
    const float cw = 158.f, chh = 88.f;
    for (int i = 0; i < cnt && i < 60; i++) {
        const ImVec2 a(pa.x + 26.f + (i % 6) * (cw + 12.f), pa.y + 118.f + (i / 6) * (chh + 12.f));
        const ImVec2 b(a.x + cw, a.y + chh);
        if (b.y > pb.y - 100.f) break;
        const int id = HW_SkinIdAt(g.sel, i);
        const bool sel = (id == g.editSkin);

        bool hov = false;
        char sk[16];
        _snprintf_s(sk, sizeof(sk), _TRUNCATE, "sk%d", i);
        if (Hit(sk, a, b, &hov)) g.editSkin = id;

        dl->AddRectFilled(a, b, sel ? C(R.violet, 0.30f) : C({1, 1, 1, hov ? 0.07f : 0.030f}), 4.f);
        dl->AddRect(a, b, sel ? C(R.gold, 0.90f) : C(R.lilac, hov ? 0.40f : 0.16f),
                    4.f, 0, sel ? 1.4f : 1.f);
        dl->AddQuad(ImVec2(a.x + 8.f, a.y + 8.f), ImVec2(a.x + 13.f, a.y + 13.f),
                    ImVec2(a.x + 8.f, a.y + 18.f), ImVec2(a.x + 3.f, a.y + 13.f),
                    C(sel ? R.gold : R.lilac, sel ? 0.85f : 0.30f), 1.f);

        char idb[24];
        _snprintf_s(idb, sizeof(idb), _TRUNCATE, "ID %d", id);
        ImGui::PushFont(fCaps);
        dl->AddText(ImVec2(a.x + 20.f, a.y + 9.f), C(sel ? R.goldHi : R.text3), idb);
        ImGui::PopFont();

        char sn[80];
        WideToUtf8(HW_SkinName(id), sn, sizeof(sn));
        ImGui::PushFont(fSm);
        dl->AddText(ImVec2(a.x + 9.f, a.y + 42.f), C(sel ? R.text : R.text2),
                    sn[0] ? sn : "无名");
        ImGui::PopFont();

        if (id == (int)h->skin) {
            dl->AddCircleFilled(ImVec2(b.x - 11.f, a.y + 12.f), 3.2f, C(R.good), 10);
            dl->AddCircle(ImVec2(b.x - 11.f, a.y + 12.f), 5.5f, C(R.good, 0.30f), 10, 1.f);
        }
    }

    const ImVec2 ba(pa.x + 26.f, pb.y - 66.f);
    if (GoldButton("applySkin", ba, ImVec2(ba.x + 160.f, ba.y + 40.f), Txt::kSkinApply, true)) {
        _snprintf_s(g.status, sizeof(g.status), _TRUNCATE, "%s",
                    HW_WriteSkin(g.sel, g.editSkin) ? "已写入换装，请重开换装界面" : "写入失败");
    }
    ImGui::PushFont(fCaps);
    char tip[64];
    _snprintf_s(tip, sizeof(tip), _TRUNCATE, "SELECTED ID %d", g.editSkin);
    dl->AddText(ImVec2(ba.x + 176.f, ba.y + 14.f), C(R.text3), tip);
    ImGui::PopFont();
}

// ============================================================ 关于页
static void PageAbout(ImVec2 lo, ImVec2 hi)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 pa(lo.x + 18.f, lo.y + 18.f), pb(hi.x - 18.f, hi.y - 18.f);
    dl->AddRectFilled(pa, pb, C(R.paper, 0.94f), 6.f);
    dl->AddRect(pa, pb, C(R.gold, 0.50f), 6.f, 0, 1.2f);
    DiamondPattern(dl, ImVec2(pa.x + 5.f, pa.y + 5.f), ImVec2(pb.x - 5.f, pb.y - 5.f),
                   52.f, C(R.paperInk, 0.020f), 1.f);

    Section(dl, ImVec2(pa.x + 30.f, pa.y + 22.f), Txt::kAbout, "ABOUT", R.paperInk, R.lilacDim);

    ImGui::PushFont(fXl);
    dl->AddText(ImVec2(pa.x + 30.f, pa.y + 58.f), C(R.paperInk), Txt::kAboutBig);
    ImGui::PopFont();

    ImGui::PushFont(fSerif);
    dl->AddText(ImVec2(pa.x + 32.f, pa.y + 104.f), C(R.lilacDim), Txt::kAboutPoem);
    ImGui::PopFont();

    dl->AddLine(ImVec2(pa.x + 30.f, pa.y + 134.f), ImVec2(pb.x - 30.f, pa.y + 134.f),
                C(R.gold, 0.32f), 1.f);

    struct KV { const char* k; const char* v; };
    const KV rows[] = {
        {"STYLE",  "Raiden Shogun / Inazuma - 紫金白、新月弧、菱纹"},
        {"SOURCE", "github.com/xiaoyeuzixi/HanaBi-ui-Genshin（版式学习）"},
        {"DATA",   "ReadProcessMemory / WriteProcessMemory，不注入、不打补丁"},
        {"BUILD",  "MSVC v143 + Windows SDK 10.0，Release | x64"},
    };
    for (int i = 0; i < 4; i++) {
        ImGui::PushFont(fCaps);
        dl->AddText(ImVec2(pa.x + 30.f, pa.y + 158.f + i * 48.f), C(R.lilacDim), rows[i].k);
        ImGui::PopFont();
        ImGui::PushFont(fSm);
        dl->AddText(ImVec2(pa.x + 118.f, pa.y + 158.f + i * 48.f), C(R.paperInk, 0.80f), rows[i].v);
        ImGui::PopFont();
    }
}

// ============================================================ D3D11
ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static IDXGISwapChain* g_sw = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static HWND g_hwnd = nullptr;
static bool g_rtvDirty = true;
static UINT g_pendingW = 0, g_pendingH = 0;

static void CreateRTV()
{
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    ID3D11Texture2D* back = nullptr;
    if (SUCCEEDED(g_sw->GetBuffer(0, IID_PPV_ARGS(&back)))) {
        g_dev->CreateRenderTargetView(back, nullptr, &g_rtv);
        back->Release();
    }
}

static bool InitD3D(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL want[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL got;
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, want, 2,
                                      D3D11_SDK_VERSION, &sd, &g_sw, &g_dev, &got, &g_ctx) != S_OK)
        return false;
    CreateRTV();
    return true;
}

static void Resize(UINT w, UINT h)
{
    if (!g_sw || w == 0 || h == 0) return;
    if (g_rtv) { g_ctx->OMSetRenderTargets(0, nullptr, nullptr); g_rtv->Release(); g_rtv = nullptr; }
    g_sw->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
    CreateRTV();
    D3D11_VIEWPORT vp = {};
    vp.Width = (float)w; vp.Height = (float)h; vp.MaxDepth = 1.f;
    g_ctx->RSSetViewports(1, &vp);
}

// ============================================================ 窗口过程
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, w, l)) return true;
    switch (msg) {
    case WM_SIZE:
        if (w != SIZE_MINIMIZED) {
            g_pendingW = (UINT)LOWORD(l);
            g_pendingH = (UINT)HIWORD(l);
            g_rtvDirty = true;
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((w & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_NCHITTEST: {
        POINT p = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
        RECT r;
        GetWindowRect(hwnd, &r);
        const int b = 7;
        const bool L2 = p.x >= r.left && p.x < r.left + b;
        const bool R2 = p.x < r.right && p.x >= r.right - b;
        const bool T2 = p.y >= r.top && p.y < r.top + b;
        const bool B2 = p.y < r.bottom && p.y >= r.bottom - b;
        if (T2 && L2) return HTTOPLEFT;
        if (T2 && R2) return HTTOPRIGHT;
        if (B2 && L2) return HTBOTTOMLEFT;
        if (B2 && R2) return HTBOTTOMRIGHT;
        if (L2) return HTLEFT;
        if (R2) return HTRIGHT;
        if (T2) return HTTOP;
        if (B2) return HTBOTTOM;
        return HTCLIENT;
    }
    case WM_GETMINMAXINFO: {
        MINMAXINFO* mm = (MINMAXINFO*)l;
        mm->ptMinTrackSize.x = 1020;
        mm->ptMinTrackSize.y = 620;
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

// ============================================================ 入口
int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, LPWSTR cmd, int show)
{
    (void)prev; (void)show;

    {
        typedef BOOL(WINAPI * PFN_CTX)(void*);
        HMODULE u = GetModuleHandleW(L"user32.dll");
        PFN_CTX setCtx = u ? (PFN_CTX)GetProcAddress(u, "SetProcessDpiAwarenessContext") : nullptr;
        BOOL ok = setCtx ? setCtx((void*)(INT_PTR)-4) : FALSE;
        if (!ok) SetProcessDPIAware();
    }

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"RaidenShogunTrainerWnd";
    wc.hIcon = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    wc.hIconSm = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (!wc.hIcon) wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    if (!wc.hIconSm) wc.hIconSm = wc.hIcon;
    RegisterClassExW(&wc);

    const int cw = 1340, ch = 780;
    const int wx = (GetSystemMetrics(SM_CXSCREEN) - cw) / 2;
    const int wy = (GetSystemMetrics(SM_CYSCREEN) - ch) / 3;
    // 不加 WS_THICKFRAME：它会画系统边框，顶部会出现白带；缩放由 WM_NCHITTEST 实现
    g_hwnd = CreateWindowExW(0, wc.lpszClassName, L"雷电将军 · 修改器",
                             WS_POPUP | WS_MINIMIZEBOX,
                             wx, wy, cw, ch, nullptr, nullptr, inst, nullptr);
    if (!g_hwnd) return 1;

    // WS_POPUP 不继承窗口类图标，任务栏会空白，需显式设置
    {
        const int icx = GetSystemMetrics(SM_CXICON);
        const int icy = GetSystemMetrics(SM_CYICON);
        const int scx = GetSystemMetrics(SM_CXSMICON);
        const int scy = GetSystemMetrics(SM_CYSMICON);
        HICON hBig = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, icx, icy, LR_DEFAULTCOLOR);
        HICON hSml = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, scx, scy, LR_DEFAULTCOLOR);
        if (hBig) SendMessageW(g_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hBig);
        if (hSml) SendMessageW(g_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hSml);
    }

    if (!InitD3D(g_hwnd)) {
        MessageBoxW(nullptr, L"D3D11 初始化失败", L"错误", MB_ICONERROR);
        return 1;
    }

    // 白边处置：
    // 不用 DwmExtendFrameIntoClientArea（DWM 会在客户区外描系统边框色，四角露白）
    // 不用 SetWindowRgn（圆角外透桌面，换壁纸又像白边）
    // 只开 DWM 原生圆角 + 把边框色置 NONE
    {
        DWM_WINDOW_CORNER_PREFERENCE cp = DWMWCP_ROUND;
        DwmSetWindowAttribute(g_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cp, sizeof(cp));

        typedef HRESULT(WINAPI * PFN_SWA)(HWND, DWORD, LPCVOID, DWORD);
        HMODULE dm = LoadLibraryW(L"dwmapi.dll");
        PFN_SWA setAttr = dm ? (PFN_SWA)GetProcAddress(dm, "DwmSetWindowAttribute") : nullptr;
        if (setAttr) {
            DWORD none = 0xFFFFFFFE;
            setAttr(g_hwnd, 34, &none, sizeof(none));
            setAttr(g_hwnd, 35, &none, sizeof(none));
        }
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    LoadFonts(16.5f);

    ImGui::StyleColorsDark();
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 0.f;
    st.FrameRounding = 3.f;
    st.ItemSpacing = ImVec2(8, 8);
    st.WindowPadding = ImVec2(0, 0);
    st.FramePadding = ImVec2(6, 5);

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    // 背景：exe 同目录优先
    {
        wchar_t exeDir[MAX_PATH] = {0};
        GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
        wchar_t* slash = wcsrchr(exeDir, L'\\');
        if (slash) *(slash + 1) = 0;

        char dirA[MAX_PATH * 2] = {0};
        WideCharToMultiByte(CP_UTF8, 0, exeDir, -1, dirA, sizeof(dirA), nullptr, nullptr);

        char f1[MAX_PATH * 3], f2[MAX_PATH * 3];
        _snprintf_s(f1, sizeof(f1), _TRUNCATE, "%sraiden_bg.jpg", dirA);
        _snprintf_s(f2, sizeof(f2), _TRUNCATE, "%sraiden_bg_small.jpg", dirA);

        if (!LoadBg(f1))
            LoadBg(f2);
    }

    HW_SetStatusSink(nullptr);

    if (wcsstr(cmd, L"--autoscan")) {
        if (HW_AttachAndScan()) {
            g.scanned = true;
            g.sel = 0;
            SyncEdit();
            _snprintf_s(g.status, sizeof(g.status), _TRUNCATE, "扫描完成：%d 位角色", HW_Count());
        } else {
            _snprintf_s(g.status, sizeof(g.status), _TRUNCATE, "%s", HW_LastError());
        }
    }

    // 不用命令行 show：从某些入口启动时可能是 SW_HIDE，窗口会跑到屏幕外
    ShowWindow(g_hwnd, SW_SHOWNORMAL);
    SetWindowPos(g_hwnd, HWND_TOP, wx, wy, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    UpdateWindow(g_hwnd);

    bool running = true;
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);

    while (running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;

        QueryPerformanceCounter(&t1);
        const double el = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;
        if (el < 1.0 / 144.0) { Sleep(1); continue; }
        t0 = t1;
        g_dt = (float)el;
        g_t += g_dt;

        if (g.introK < 1.f) {
            g.introK += g_dt / 1.35f;
            if (g.introK > 1.f) g.introK = 1.f;
        }

        if (g_rtvDirty && g_pendingW && g_pendingH) {
            Resize(g_pendingW, g_pendingH);
            g_rtvDirty = false;
        }
        if (!g_rtv) continue;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        const ImVec2 vp = io.DisplaySize;
        ImDrawList* bg = ImGui::GetBackgroundDrawList();
        if (bg) bg->AddRectFilled(ImVec2(0, 0), vp, IM_COL32(0, 0, 0, 0));

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(vp);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("##root", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);

        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 lo(0, 0), hi(vp.x, vp.y);

            DrawArtwork(lo, hi);

            const float headH = 62.f;
            DrawHeader(ImVec2(lo.x + 14.f, lo.y + 8.f), ImVec2(hi.x - 14.f, lo.y + headH), g_hwnd);

            const float navW = 168.f;
            const ImVec2 navA(lo.x + 14.f, lo.y + headH + 6.f);
            const ImVec2 navB(navA.x + navW, hi.y - 52.f);
            DrawNav(navA, navB);

            const ImVec2 bodyA(navB.x + 12.f, navA.y), bodyB(hi.x - 14.f, hi.y - 52.f);
            ImGui::PushClipRect(bodyA, bodyB, true);
            switch (g.page) {
            case 0: PageRoster(bodyA, bodyB); break;
            case 1: PageBulk(bodyA, bodyB); break;
            case 2: PageSkins(bodyA, bodyB); break;
            case 3: PageAbout(bodyA, bodyB); break;
            }
            ImGui::PopClipRect();

            const ImVec2 fbA(lo.x + 14.f, hi.y - 44.f), fbB(hi.x - 14.f, hi.y - 10.f);
            dl->AddRectFilled(fbA, fbB, C(R.ink0, 0.55f), 4.f);
            dl->AddLine(ImVec2(fbA.x + 16.f, fbA.y), ImVec2(fbB.x - 16.f, fbA.y), C(R.gold, 0.20f), 1.f);
            dl->AddQuadFilled(ImVec2(fbA.x + 18.f, fbA.y + 17.f), ImVec2(fbA.x + 22.f, fbA.y + 13.f),
                              ImVec2(fbA.x + 26.f, fbA.y + 17.f), ImVec2(fbA.x + 22.f, fbA.y + 21.f),
                              C(HW_Ready() ? R.good : R.goldDim));
            ImGui::PushFont(fSm);
            dl->AddText(ImVec2(fbA.x + 34.f, fbA.y + 8.f), C(R.text2), g.status);
            ImGui::PopFont();

            ImGui::PushFont(fCaps);
            dl->AddText(ImVec2(fbB.x - 152.f, fbA.y + 9.f), C(R.text3, 0.75f), Txt::kFooter);
            ImGui::PopFont();
        }

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Render();
        const float clear[4] = {0.f, 0.f, 0.f, 0.f};
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_sw->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (g_bgSRV) g_bgSRV->Release();
    if (g_rtv) g_rtv->Release();
    if (g_sw) g_sw->Release();
    if (g_ctx) g_ctx->Release();
    if (g_dev) g_dev->Release();
    DestroyWindow(g_hwnd);
    UnregisterClassW(wc.lpszClassName, inst);
    return 0;
}
