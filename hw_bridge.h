// hw_bridge.h - UI 与业务层的唯一接口
//
// 你的 HWEditor3.cpp 里那套东西分两块:
//   (A) 内存操作: rd / wr / rq / ri / find_class / find_objects / read_hero / write_hero
//   (B) 界面:      proc / fill_list / load_fields / on_write_row / on_apply_all ...
//
// 只把 (A) 搬进 hw_bridge.cpp, 把 (B) 整个删掉换成 main.cpp 的 ImGui 绘制。
// 下面每个函数就是原来某个按钮回调的等价物, 参数从 HWND 控件读改成直接传值。

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// 一个角色的快照。字段和 HWEditor3.cpp 里的 Hero 一致, 名字保持不变,
// 这样你直接拿原来的 write_hero() 往里套。
typedef struct {
    unsigned long long addr;   // GameHero 实例地址
    int      heroId;
    wchar_t  name[64];
    unsigned int level;
    int      curHp, maxHp;
    int      str_, dex, int_, con, spi, agi;
    unsigned short skin;       // 当前装备的皮肤 id
    int      defSkin;
    int      skinIdCount;
    int      skinIds[64];      // 该角色拥有的皮肤 id 表
} HeroRow;

// ---- 生命周期 -------------------------------------------------------------
// 对应原来的 do_refresh() + scan_thread(): 找进程、OpenProcess、
// 找 GameHero 类、枚举实例、建皮肤映射、存原始快照。成功返回 1。
int  HW_AttachAndScan(void);

// 是否已连上并扫描过
int  HW_Ready(void);

// 最近一次失败原因 (UTF-8, 直接塞状态条)
const char* HW_LastError(void);

// 底部那行小字 (进程名/PID/类地址), UTF-8
const char* HW_StatusLine(void);

// ---- 查询 ---------------------------------------------------------------
int            HW_Count(void);
const HeroRow* HW_HeroAt(int i);

int            HW_SkinCount(int row);
int            HW_SkinIdAt(int row, int i);
// 返回宽字符显示名, 没有则返回空串
const wchar_t* HW_SkinName(int skinId);

// ---- 写入 ---------------------------------------------------------------
// fields 顺序: level, curHp, maxHp, str, dex, int, con, agi (对应原来的 v[0..7])
int HW_WriteRow(int row, const int fields[8]);
int HW_ApplyAll(int value);
int HW_WriteSkin(int row, int skinId);
int HW_RestoreOne(int row);
int HW_RestoreAll(void);

// 状态输出回调。传 NULL 表示不回调, 状态由 UI 自己的 status 缓冲承载。
typedef void (*HW_StatusFn)(const char* utf8);
void HW_SetStatusSink(HW_StatusFn fn);

// 工具: wchar_t* -> UTF-8, 返回 out。UI 里到处要用。
const char* WideToUtf8(const wchar_t* w, char* out, int cap);

#ifdef __cplusplus
}
#endif
