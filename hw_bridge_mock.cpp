// hw_bridge_mock.cpp - 假数据实现, 只为了让界面能独立跑起来看效果
//
// 编译时加 /DHW_MOCK 并链接这个文件代替 hw_bridge.cpp。
// 真接游戏时: 把 HWEditor3.cpp 里的内存函数搬进 hw_bridge.cpp 实现同一套接口。

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "hw_bridge.h"

#define MAX_HEROES 64

static HeroRow g_heroes[MAX_HEROES];
static int     g_count = 0;
static int     g_ready = 0;
static char    g_err[256] = "（mock 模式：未连接真实游戏）";
static char    g_line[256] = "MOCK / 无进程";

const char* WideToUtf8(const wchar_t* w, char* out, int cap)
{
    if (!w || !out || cap <= 0) return out;
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, out, cap, NULL, NULL);
    if (n <= 0) out[0] = 0;
    return out;
}

static const wchar_t* kNames[] = {
    L"\u971c\u534e",     // 霜华
    L"\u96ea\u89c1",     // 雪见
    L"\u51b0\u5f26",     // 冰弦
    L"\u5bd2\u6f6d",     // 寒潭
    L"\u51dd\u9732",     // 凝露
    L"\u971c\u534e\u00b7\u5f71", // 霜华·影
    L"\u96ea\u539f",     // 雪原
    L"\u5bd2\u661f",     // 寒星
    L"\u51b0\u5c18",     // 冰尘
    L"\u6e05\u51b7",     // 清冷
};

static const wchar_t* kSkins[] = {
    L"\u9ed8\u8ba4\u5916\u89c2",       // 默认外观
    L"\u51ac\u5f52",                   // 冬归
    L"\u9732\u534e\u77f3",             // 露华石
    L"\u5bd2\u5929\u4e4b\u7fbd",       // 寒天之羽
    L"\u771f\u541b\u4e4b\u6b7f",       // 真君之殁
    L"\u6e05\u5fc3\u4e0b\u5c18",       // 清心下尘
};

int HW_AttachAndScan(void)
{
    g_count = 0;
    for (int i = 0; i < 10; i++) {
        HeroRow* h = &g_heroes[g_count++];
        memset(h, 0, sizeof(*h));
        h->addr = 0x1E4A0000ull + (unsigned long long)i * 0x1A0ull;
        h->heroId = 1000 + i;
        wcscpy_s(h->name, 64, kNames[i]);
        h->level = 60 + i * 5;
        h->maxHp = 18000 + i * 4200;
        h->curHp = h->maxHp - i * 900;
        h->str_ = 120 + i * 11;
        h->dex = 95 + i * 9;
        h->int_ = 88 + i * 13;
        h->con = 110 + i * 7;
        h->spi = 70 + i * 5;
        h->agi = 102 + i * 6;
        h->skin = (unsigned short)(i % 4);
        h->defSkin = 0;
        h->skinIdCount = 6;
        for (int s = 0; s < 6; s++) h->skinIds[s] = s;
    }
    g_ready = 1;
    _snprintf_s(g_err, sizeof(g_err), _TRUNCATE, "ok");
    _snprintf_s(g_line, sizeof(g_line), _TRUNCATE, "MOCK / pid 0 / GameHero 0x0");
    return 1;
}

int HW_Ready(void) { return g_ready; }
const char* HW_LastError(void) { return g_err; }
const char* HW_StatusLine(void) { return g_line; }

int HW_Count(void) { return g_count; }

const HeroRow* HW_HeroAt(int i)
{
    if (i < 0 || i >= g_count) return NULL;
    return &g_heroes[i];
}

int HW_SkinCount(int row)
{
    if (row < 0 || row >= g_count) return 0;
    return g_heroes[row].skinIdCount;
}

int HW_SkinIdAt(int row, int i)
{
    if (row < 0 || row >= g_count) return 0;
    if (i < 0 || i >= g_heroes[row].skinIdCount) return 0;
    return g_heroes[row].skinIds[i];
}

const wchar_t* HW_SkinName(int skinId)
{
    if (skinId < 0 || skinId > 5) return L"";
    return kSkins[skinId];
}

int HW_WriteRow(int row, const int f[8])
{
    if (row < 0 || row >= g_count) return 0;
    HeroRow* h = &g_heroes[row];
    h->level = (unsigned)f[0];
    h->curHp = f[1];
    h->maxHp = f[2];
    h->str_ = f[3];
    h->dex = f[4];
    h->int_ = f[5];
    h->con = f[6];
    h->agi = f[7];
    return 1;
}

int HW_ApplyAll(int v)
{
    for (int i = 0; i < g_count; i++) {
        g_heroes[i].level = (unsigned)v;
        g_heroes[i].curHp = v;
        g_heroes[i].maxHp = v;
        g_heroes[i].str_ = v;
        g_heroes[i].dex = v;
        g_heroes[i].int_ = v;
        g_heroes[i].con = v;
        g_heroes[i].agi = v;
    }
    return 1;
}

int HW_WriteSkin(int row, int id)
{
    if (row < 0 || row >= g_count) return 0;
    g_heroes[row].skin = (unsigned short)id;
    return 1;
}

int HW_RestoreOne(int row) { return (row >= 0 && row < g_count); }
int HW_RestoreAll(void) { return g_count > 0; }
void HW_SetStatusSink(HW_StatusFn fn) { (void)fn; }
