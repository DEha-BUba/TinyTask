// ============================================================
//  TinyTask Pro  –  Gelişmiş Makro Kaydedici
//  Derleme: g++ -std=c++17 -mwindows -o tinytask_pro.exe tinytask_pro.cpp
//              -lcomctl32 -lcomdlg32
// ============================================================
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

// ─────────────────────────────────────────────────────────────
//  Veri yapıları
// ─────────────────────────────────────────────────────────────
enum class EventType : uint8_t { Key, MouseMove, MouseButton, MouseWheel };

struct Event {
    EventType type{};
    DWORD     delayMs{};
    DWORD     vkCode{};
    DWORD     scanCode{};     // donanım scan kodu – oynatmada şart
    bool      keyUp{};
    bool      extendedKey{};  // ok tuşları, ins, del, home... için gerekli
    POINT     pt{};
    DWORD     mouseFlags{};
    DWORD     mouseData{};
};

// ─────────────────────────────────────────────────────────────
//  Kontrol kimlikleri
// ─────────────────────────────────────────────────────────────
constexpr int ID_BTN_RECORD   = 1001;
constexpr int ID_BTN_PLAY     = 1002;
constexpr int ID_BTN_PAUSE    = 1003;
constexpr int ID_BTN_STOP     = 1004;
constexpr int ID_CHK_LOOP     = 1005;
constexpr int ID_LIST_EVENTS  = 1006;
constexpr int ID_TRACK_SPEED  = 1007;
constexpr int ID_LBL_SPEED    = 1008;
constexpr int ID_LBL_STATUS   = 1009;
constexpr int ID_BTN_CLEAR    = 1010;
constexpr int ID_BTN_DELETE   = 1011;
constexpr int ID_BTN_SAVE     = 1012;
constexpr int ID_BTN_LOAD     = 1013;
constexpr int ID_EDIT_REPEAT  = 1014;
constexpr int ID_SPIN_REPEAT  = 1015;
constexpr int ID_LBL_COUNT    = 1016;
constexpr int ID_LBL_HOTKEYS  = 1017;

constexpr int ID_HK_TOGGLE_RECORD = 2001;
constexpr int ID_HK_PLAY          = 2002;
constexpr int ID_HK_PAUSE         = 2003;
constexpr int ID_HK_STOP          = 2004;

constexpr UINT WM_APP_UPDATE_STATUS = WM_APP + 1;
constexpr UINT WM_APP_UPDATE_COUNT  = WM_APP + 2;

enum StatusCode { STATUS_IDLE = 0, STATUS_RECORDING, STATUS_PLAYING, STATUS_PAUSED };

// ─────────────────────────────────────────────────────────────
//  Global değişkenler
// ─────────────────────────────────────────────────────────────
std::atomic<bool> g_recording{false};
std::atomic<bool> g_playing{false};
std::atomic<bool> g_stopPlayback{false};
std::atomic<bool> g_pausePlayback{false};
std::atomic<int>  g_speedPct{100};   // BUG-FIX: worker thread artık SendMessage kullanmıyor

HINSTANCE g_hInst      = nullptr;
HWND g_mainWnd         = nullptr;
HWND g_btnRecord       = nullptr;
HWND g_btnPlay         = nullptr;
HWND g_btnPause        = nullptr;
HWND g_btnStop         = nullptr;
HWND g_chkLoop         = nullptr;
HWND g_listEvents      = nullptr;
HWND g_trackSpeed      = nullptr;
HWND g_lblSpeed        = nullptr;
HWND g_lblStatus       = nullptr;
HWND g_btnClear        = nullptr;
HWND g_btnDelete       = nullptr;
HWND g_btnSave         = nullptr;
HWND g_btnLoad         = nullptr;
HWND g_editRepeat      = nullptr;
HWND g_spinRepeat      = nullptr;
HWND g_lblCount        = nullptr;

HHOOK g_keyboardHook   = nullptr;
HHOOK g_mouseHook      = nullptr;

std::vector<Event> g_recordedEvents;
std::mutex         g_eventsMutex;
std::chrono::steady_clock::time_point g_lastEventTs;  // BUG-FIX: {} sıfırlamayı güvenli yapar
std::thread        g_playbackThread;

// ─────────────────────────────────────────────────────────────
//  Tuş adı çözümleyici  (VK kodu → okunabilir isim)
// ─────────────────────────────────────────────────────────────
std::wstring VkToName(DWORD vk) {
    switch (vk) {
        case VK_BACK:      return L"Backspace";
        case VK_TAB:       return L"Tab";
        case VK_RETURN:    return L"Enter";
        case VK_SHIFT:     return L"Shift";
        case VK_CONTROL:   return L"Ctrl";
        case VK_MENU:      return L"Alt";
        case VK_CAPITAL:   return L"CapsLock";
        case VK_ESCAPE:    return L"Esc";
        case VK_SPACE:     return L"Boşluk";
        case VK_PRIOR:     return L"PageUp";
        case VK_NEXT:      return L"PageDown";
        case VK_END:       return L"End";
        case VK_HOME:      return L"Home";
        case VK_LEFT:      return L"Sol";
        case VK_UP:        return L"Yukari";
        case VK_RIGHT:     return L"Sag";
        case VK_DOWN:      return L"Asagi";
        case VK_SNAPSHOT:  return L"PrintScreen";
        case VK_INSERT:    return L"Insert";
        case VK_DELETE:    return L"Delete";
        case VK_LWIN:      return L"LWin";
        case VK_RWIN:      return L"RWin";
        case VK_APPS:      return L"Menu";
        case VK_NUMPAD0:   return L"Num0";
        case VK_NUMPAD1:   return L"Num1";
        case VK_NUMPAD2:   return L"Num2";
        case VK_NUMPAD3:   return L"Num3";
        case VK_NUMPAD4:   return L"Num4";
        case VK_NUMPAD5:   return L"Num5";
        case VK_NUMPAD6:   return L"Num6";
        case VK_NUMPAD7:   return L"Num7";
        case VK_NUMPAD8:   return L"Num8";
        case VK_NUMPAD9:   return L"Num9";
        case VK_MULTIPLY:  return L"Num*";
        case VK_ADD:       return L"Num+";
        case VK_SUBTRACT:  return L"Num-";
        case VK_DECIMAL:   return L"Num.";
        case VK_DIVIDE:    return L"Num/";
        case VK_NUMLOCK:   return L"NumLock";
        case VK_SCROLL:    return L"ScrollLock";
        case VK_LSHIFT:    return L"L-Shift";
        case VK_RSHIFT:    return L"R-Shift";
        case VK_LCONTROL:  return L"L-Ctrl";
        case VK_RCONTROL:  return L"R-Ctrl";
        case VK_LMENU:     return L"L-Alt";
        case VK_RMENU:     return L"R-Alt";
        case VK_OEM_1:     return L";";
        case VK_OEM_PLUS:  return L"=";
        case VK_OEM_COMMA: return L",";
        case VK_OEM_MINUS: return L"-";
        case VK_OEM_PERIOD:return L".";
        case VK_OEM_2:     return L"/";
        case VK_OEM_3:     return L"`";
        case VK_OEM_4:     return L"[";
        case VK_OEM_5:     return L"\\";
        case VK_OEM_6:     return L"]";
        case VK_OEM_7:     return L"'";
        case VK_F1:  return L"F1";  case VK_F2:  return L"F2";
        case VK_F3:  return L"F3";  case VK_F4:  return L"F4";
        case VK_F5:  return L"F5";  case VK_F6:  return L"F6";
        case VK_F7:  return L"F7";  case VK_F8:  return L"F8";
        case VK_F9:  return L"F9";  case VK_F10: return L"F10";
        case VK_F11: return L"F11"; case VK_F12: return L"F12";
        case VK_PAUSE: return L"Pause";
    }
    // Harf / rakam
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9'))
        return std::wstring(1, static_cast<wchar_t>(vk));

    wchar_t buf[16];
    swprintf_s(buf, 16, L"VK(0x%02X)", vk);
    return buf;
}

// ─────────────────────────────────────────────────────────────
//  Fare düğmesi açıklaması
// ─────────────────────────────────────────────────────────────
std::wstring MouseFlagName(DWORD flags) {
    if (flags & MOUSEEVENTF_LEFTDOWN)   return L"Sol-Bas";
    if (flags & MOUSEEVENTF_LEFTUP)     return L"Sol-Birak";
    if (flags & MOUSEEVENTF_RIGHTDOWN)  return L"Sag-Bas";
    if (flags & MOUSEEVENTF_RIGHTUP)    return L"Sag-Birak";
    if (flags & MOUSEEVENTF_MIDDLEDOWN) return L"Orta-Bas";
    if (flags & MOUSEEVENTF_MIDDLEUP)   return L"Orta-Birak";
    if (flags & MOUSEEVENTF_XDOWN)      return L"X-Bas";
    if (flags & MOUSEEVENTF_XUP)        return L"X-Birak";
    return L"?";
}

// ─────────────────────────────────────────────────────────────
//  Durum etiketi
// ─────────────────────────────────────────────────────────────
void SetStatusText(StatusCode code) {
    if (!g_lblStatus) return;
    switch (code) {
        case STATUS_RECORDING: SetWindowText(g_lblStatus, L"[REC] Kayit");          break;
        case STATUS_PLAYING:   SetWindowText(g_lblStatus, L"[>>>] Oynatiyor");      break;
        case STATUS_PAUSED:    SetWindowText(g_lblStatus, L"[||]  Duraklatildi");   break;
        default:               SetWindowText(g_lblStatus, L"[   ] Bekliyor");       break;
    }
}

void UpdateCountLabel() {
    if (!g_lblCount) return;
    size_t cnt;
    {
        std::lock_guard<std::mutex> lk(g_eventsMutex);
        cnt = g_recordedEvents.size();
    }
    wchar_t buf[32];
    swprintf_s(buf, 32, L"%zu olay", cnt);
    SetWindowText(g_lblCount, buf);
}

void UpdatePlaybackButtons() {
    const bool playing   = g_playing.load();
    const bool recording = g_recording.load();
    EnableWindow(g_btnPause,  playing ? TRUE : FALSE);
    EnableWindow(g_btnStop,   playing ? TRUE : FALSE);
    EnableWindow(g_btnPlay,   (!playing && !recording) ? TRUE : FALSE);
    EnableWindow(g_btnRecord, !playing ? TRUE : FALSE);
    EnableWindow(g_btnClear,  (!playing && !recording) ? TRUE : FALSE);
    EnableWindow(g_btnDelete, (!playing && !recording) ? TRUE : FALSE);
    EnableWindow(g_btnSave,   (!playing && !recording) ? TRUE : FALSE);
    EnableWindow(g_btnLoad,   (!playing && !recording) ? TRUE : FALSE);
    SetWindowText(g_btnPause, g_pausePlayback.load() ? L"Devam Et" : L"Duraklat");
}

// ─────────────────────────────────────────────────────────────
//  Zamanlama  (BUG-FIX: {} ile düzgün sıfırlama)
// ─────────────────────────────────────────────────────────────
DWORD NextDelayMs() {
    const auto now = std::chrono::steady_clock::now();
    if (g_lastEventTs == std::chrono::steady_clock::time_point{}) {
        g_lastEventTs = now;
        return 0;
    }
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastEventTs).count();
    g_lastEventTs = now;
    if (ms <= 0) return 0;
    if (ms > static_cast<long long>(MAXDWORD)) return MAXDWORD;
    return static_cast<DWORD>(ms);
}

// ─────────────────────────────────────────────────────────────
//  Koordinat normalleştirme (sanal masaüstü desteği)
// ─────────────────────────────────────────────────────────────
LONG NormalizeCoordVirtual(int value, int origin, int extent) {
    if (extent <= 1) return 0;
    long long rel    = static_cast<long long>(value) - origin;
    long long scaled = (rel * 65535LL) / (extent - 1);
    if (scaled < 0)     scaled = 0;
    if (scaled > 65535) scaled = 65535;
    return static_cast<LONG>(scaled);
}

// ─────────────────────────────────────────────────────────────
//  Olay metni
// ─────────────────────────────────────────────────────────────
std::wstring EventToText(const Event& ev, size_t idx) {
    std::wstringstream ss;
    ss << (idx + 1) << L". [" << ev.delayMs << L"ms]  ";
    switch (ev.type) {
        case EventType::Key:
            ss << L"TUS " << VkToName(ev.vkCode)
               << (ev.keyUp ? L"  [yukari]" : L"  [asagi]");
            break;
        case EventType::MouseMove:
            ss << L"FARE Hareket  (" << ev.pt.x << L", " << ev.pt.y << L")";
            break;
        case EventType::MouseButton:
            ss << L"FARE " << MouseFlagName(ev.mouseFlags)
               << L"  (" << ev.pt.x << L", " << ev.pt.y << L")";
            break;
        case EventType::MouseWheel:
            ss << L"FARE Tekerlek  delta="
               << static_cast<SHORT>(HIWORD(ev.mouseData))
               << L"  (" << ev.pt.x << L", " << ev.pt.y << L")";
            break;
    }
    return ss.str();
}

void RefreshEventList() {
    if (!g_listEvents) return;
    SendMessage(g_listEvents, WM_SETREDRAW, FALSE, 0);
    SendMessage(g_listEvents, LB_RESETCONTENT, 0, 0);
    {
        std::lock_guard<std::mutex> lk(g_eventsMutex);
        for (size_t i = 0; i < g_recordedEvents.size(); ++i) {
            const std::wstring line = EventToText(g_recordedEvents[i], i);
            SendMessage(g_listEvents, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
        }
    }
    SendMessage(g_listEvents, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_listEvents, nullptr, TRUE);
}

void UpdateSpeedLabel() {
    if (!g_trackSpeed || !g_lblSpeed) return;
    const int spd = static_cast<int>(SendMessage(g_trackSpeed, TBM_GETPOS, 0, 0));
    g_speedPct.store(spd);  // worker thread için atomic güncelleme
    wchar_t buf[32];
    swprintf_s(buf, 32, L"Hiz: %%%d", spd);
    SetWindowText(g_lblSpeed, buf);
}

// ─────────────────────────────────────────────────────────────
//  Hook işlevleri
// ─────────────────────────────────────────────────────────────
void StopHooks();

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_recording.load()) {
        const auto* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        // Global kısayol tuşlarını kaydetme
        if (p->vkCode == VK_F8 || p->vkCode == VK_F9 ||
            p->vkCode == VK_F10 || p->vkCode == VK_F11) {
            return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
        }
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN ||
            wParam == WM_KEYUP   || wParam == WM_SYSKEYUP) {
            Event ev{};
            ev.type        = EventType::Key;
            ev.delayMs     = NextDelayMs();
            ev.vkCode      = p->vkCode;
            ev.scanCode    = p->scanCode;                          // donanım scan kodu
            ev.extendedKey = (p->flags & LLKHF_EXTENDED) != 0;    // ok/ins/del/home...
            ev.keyUp       = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
            {
                std::lock_guard<std::mutex> lk(g_eventsMutex);
                g_recordedEvents.push_back(ev);
            }
            PostMessage(g_mainWnd, WM_APP_UPDATE_COUNT, 0, 0);
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

DWORD MouseFlagsFromMsg(WPARAM msg) {
    switch (msg) {
        case WM_LBUTTONDOWN: return MOUSEEVENTF_LEFTDOWN;
        case WM_LBUTTONUP:   return MOUSEEVENTF_LEFTUP;
        case WM_RBUTTONDOWN: return MOUSEEVENTF_RIGHTDOWN;
        case WM_RBUTTONUP:   return MOUSEEVENTF_RIGHTUP;
        case WM_MBUTTONDOWN: return MOUSEEVENTF_MIDDLEDOWN;
        case WM_MBUTTONUP:   return MOUSEEVENTF_MIDDLEUP;
        case WM_XBUTTONDOWN: return MOUSEEVENTF_XDOWN;
        case WM_XBUTTONUP:   return MOUSEEVENTF_XUP;
        default:             return 0;
    }
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_recording.load()) {
        const auto* p = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        Event ev{};
        ev.delayMs = NextDelayMs();
        ev.pt      = p->pt;
        bool push  = true;

        if (wParam == WM_MOUSEMOVE) {
            ev.type       = EventType::MouseMove;
            ev.mouseFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
        } else if (wParam == WM_MOUSEWHEEL) {
            ev.type       = EventType::MouseWheel;
            ev.mouseFlags = MOUSEEVENTF_WHEEL | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
            ev.mouseData  = p->mouseData;
        } else {
            const DWORD bf = MouseFlagsFromMsg(wParam);
            if (bf != 0) {
                ev.type       = EventType::MouseButton;
                ev.mouseFlags = bf | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
                if (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP)
                    ev.mouseData = p->mouseData;
            } else {
                push = false;
            }
        }
        if (push) {
            std::lock_guard<std::mutex> lk(g_eventsMutex);
            g_recordedEvents.push_back(ev);
            PostMessage(g_mainWnd, WM_APP_UPDATE_COUNT, 0, 0);
        }
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

void StartHooks() {
    if (!g_keyboardHook)
        g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, g_hInst, 0);
    if (!g_mouseHook)
        g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, g_hInst, 0);
}

void StopHooks() {
    if (g_keyboardHook) { UnhookWindowsHookEx(g_keyboardHook); g_keyboardHook = nullptr; }
    if (g_mouseHook)    { UnhookWindowsHookEx(g_mouseHook);    g_mouseHook    = nullptr; }
}

// ─────────────────────────────────────────────────────────────
//  Giriş gönderme
// ─────────────────────────────────────────────────────────────
void SendEvent(const Event& ev) {
    INPUT in{};
    if (ev.type == EventType::Key) {
        in.type       = INPUT_KEYBOARD;
        in.ki.wVk     = static_cast<WORD>(ev.vkCode);
        // Scan code: hook'tan kaydedilen değeri kullan,
        // yoksa VK'dan dönüştür (eski kayıtlar için yedek)
        in.ki.wScan   = ev.scanCode
                            ? static_cast<WORD>(ev.scanCode)
                            : static_cast<WORD>(MapVirtualKey(ev.vkCode, MAPVK_VK_TO_VSC));
        in.ki.dwFlags = 0;
        if (ev.keyUp)       in.ki.dwFlags |= KEYEVENTF_KEYUP;
        if (ev.extendedKey) in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        SendInput(1, &in, sizeof(INPUT));
        return;
    }
    const int vl = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int vt = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    in.type         = INPUT_MOUSE;
    in.mi.dx        = NormalizeCoordVirtual(ev.pt.x, vl, vw);
    in.mi.dy        = NormalizeCoordVirtual(ev.pt.y, vt, vh);
    in.mi.dwFlags   = ev.mouseFlags | MOUSEEVENTF_VIRTUALDESK;
    in.mi.mouseData = ev.mouseData;
    SendInput(1, &in, sizeof(INPUT));
}

// BUG-FIX: pause/stop kontrollü uyku – 15 ms parçalara böler
bool SleepWithControl(DWORD totalMs) {
    DWORD waited = 0;
    while (waited < totalMs) {
        if (g_stopPlayback.load()) return false;
        while (g_pausePlayback.load()) {
            if (g_stopPlayback.load()) return false;
            Sleep(10);
        }
        const DWORD chunk = std::min<DWORD>(15, totalMs - waited);
        Sleep(chunk);
        waited += chunk;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────
//  Tekrar sayısı (UI thread'den önce okunur)
// ─────────────────────────────────────────────────────────────
int ReadRepeatCount() {
    wchar_t buf[16]{};
    GetWindowText(g_editRepeat, buf, 16);
    int n = _wtoi(buf);
    return (n < 1) ? 1 : n;
}

// ─────────────────────────────────────────────────────────────
//  Oynatma worker'ı  (ayrı thread'de çalışır)
//  BUG-FIX: hız için atomic okur, UI thread'e SendMessage göndermez
// ─────────────────────────────────────────────────────────────
void PlaybackWorker(int speedPct, bool loop, int repeatCount) {
    PostMessage(g_mainWnd, WM_APP_UPDATE_STATUS, STATUS_PLAYING, 0);

    int  iteration = 0;
    bool aborted   = false;

    while (!aborted && !g_stopPlayback.load()) {
        // Anlık snapshot al – kilidi hemen serbest bırak
        std::vector<Event> snapshot;
        {
            std::lock_guard<std::mutex> lk(g_eventsMutex);
            snapshot = g_recordedEvents;
        }

        for (const auto& ev : snapshot) {
            // Duraklatma döngüsü
            while (g_pausePlayback.load()) {
                if (g_stopPlayback.load()) { aborted = true; break; }
                Sleep(10);
            }
            if (aborted || g_stopPlayback.load()) { aborted = true; break; }

            // Hız ayarına göre gecikme
            const unsigned long long delay =
                (static_cast<unsigned long long>(ev.delayMs) * 100ULL) /
                static_cast<unsigned long long>(std::max(speedPct, 1));

            if (delay > 0 && !SleepWithControl(static_cast<DWORD>(delay))) {
                aborted = true;
                break;
            }
            SendEvent(ev);
        }

        if (!aborted) {
            ++iteration;
            if (!loop && iteration >= repeatCount) break;
        }
    }

    g_playing.store(false);
    PostMessage(g_mainWnd, WM_APP_UPDATE_STATUS, STATUS_IDLE, 0);
}

void StartPlayback() {
    if (g_playing.load()) return;
    {
        std::lock_guard<std::mutex> lk(g_eventsMutex);
        if (g_recordedEvents.empty()) {
            MessageBox(g_mainWnd, L"Oynatilacak kayit yok.", L"Bilgi",
                       MB_OK | MB_ICONINFORMATION);
            return;
        }
    }
    // UI parametrelerini ana thread'de oku
    const int  speed  = g_speedPct.load();
    const bool loop   = (SendMessage(g_chkLoop, BM_GETCHECK, 0, 0) == BST_CHECKED);
    const int  repeat = ReadRepeatCount();

    g_stopPlayback.store(false);
    g_pausePlayback.store(false);
    g_playing.store(true);
    UpdatePlaybackButtons();

    if (g_playbackThread.joinable()) g_playbackThread.join();
    g_playbackThread = std::thread(PlaybackWorker, speed, loop, repeat);
}

void StopPlayback() {
    g_stopPlayback.store(true);
    g_pausePlayback.store(false);
    if (g_playbackThread.joinable()) g_playbackThread.join();
    g_playing.store(false);
    UpdatePlaybackButtons();
    if (!g_recording.load()) SetStatusText(STATUS_IDLE);
}

void TogglePausePlayback() {
    if (!g_playing.load()) return;
    const bool paused = !g_pausePlayback.load();
    g_pausePlayback.store(paused);
    UpdatePlaybackButtons();
    SetStatusText(paused ? STATUS_PAUSED : STATUS_PLAYING);
}

// ─────────────────────────────────────────────────────────────
//  Kayıt başlat / durdur
// ─────────────────────────────────────────────────────────────
void ToggleRecording() {
    const bool nowRec = !g_recording.load();
    g_recording.store(nowRec);

    if (nowRec) {
        StopPlayback();
        {
            std::lock_guard<std::mutex> lk(g_eventsMutex);
            g_recordedEvents.clear();
        }
        g_lastEventTs = {};          // BUG-FIX: value-init ile güvenli sıfırlama
        StartHooks();
        SetWindowText(g_btnRecord, L"Kaydi Durdur");
        SetWindowText(g_mainWnd,   L"TinyTask Pro  [KAYIT]");
        SendMessage(g_listEvents, LB_RESETCONTENT, 0, 0);
        SetStatusText(STATUS_RECORDING);
    } else {
        StopHooks();
        SetWindowText(g_btnRecord, L"Kaydi Baslat");
        SetWindowText(g_mainWnd,   L"TinyTask Pro");
        RefreshEventList();
        SetStatusText(STATUS_IDLE);
    }
    UpdatePlaybackButtons();
    UpdateCountLabel();
}

// ─────────────────────────────────────────────────────────────
//  Dosya kaydet / yükle  (özel ikili format .ttk)
// ─────────────────────────────────────────────────────────────
constexpr uint32_t FILE_MAGIC   = 0x4B325454u; // "TTK2"
constexpr uint32_t FILE_VERSION = 2u;           // v2: scanCode + extendedKey eklendi

bool SaveRecording(const wchar_t* path) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    std::lock_guard<std::mutex> lk(g_eventsMutex);
    const uint32_t magic   = FILE_MAGIC;
    const uint32_t version = FILE_VERSION;
    const uint32_t count   = static_cast<uint32_t>(g_recordedEvents.size());
    f.write(reinterpret_cast<const char*>(&magic),   4);
    f.write(reinterpret_cast<const char*>(&version), 4);
    f.write(reinterpret_cast<const char*>(&count),   4);
    for (const auto& ev : g_recordedEvents) {
        const uint8_t t   = static_cast<uint8_t>(ev.type);
        const uint8_t ku  = ev.keyUp ? 1u : 0u;
        const uint8_t ext = ev.extendedKey ? 1u : 0u;
        f.write(reinterpret_cast<const char*>(&t),             1);
        f.write(reinterpret_cast<const char*>(&ev.delayMs),    4);
        f.write(reinterpret_cast<const char*>(&ev.vkCode),     4);
        f.write(reinterpret_cast<const char*>(&ev.scanCode),   4);
        f.write(reinterpret_cast<const char*>(&ku),            1);
        f.write(reinterpret_cast<const char*>(&ext),           1);
        f.write(reinterpret_cast<const char*>(&ev.pt.x),       4);
        f.write(reinterpret_cast<const char*>(&ev.pt.y),       4);
        f.write(reinterpret_cast<const char*>(&ev.mouseFlags), 4);
        f.write(reinterpret_cast<const char*>(&ev.mouseData),  4);
    }
    return f.good();
}

bool LoadRecording(const wchar_t* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    uint32_t magic{}, version{}, count{};
    f.read(reinterpret_cast<char*>(&magic),   4);
    f.read(reinterpret_cast<char*>(&version), 4);
    f.read(reinterpret_cast<char*>(&count),   4);
    if (!f || magic != FILE_MAGIC || version != FILE_VERSION) return false;
    if (count > 2'000'000u) return false;    // akıl dışı büyüklük koruması

    std::vector<Event> events;
    events.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        Event ev{};
        uint8_t t{}, ku{}, ext{};
        f.read(reinterpret_cast<char*>(&t),             1);
        f.read(reinterpret_cast<char*>(&ev.delayMs),    4);
        f.read(reinterpret_cast<char*>(&ev.vkCode),     4);
        f.read(reinterpret_cast<char*>(&ev.scanCode),   4);
        f.read(reinterpret_cast<char*>(&ku),            1);
        f.read(reinterpret_cast<char*>(&ext),           1);
        f.read(reinterpret_cast<char*>(&ev.pt.x),       4);
        f.read(reinterpret_cast<char*>(&ev.pt.y),       4);
        f.read(reinterpret_cast<char*>(&ev.mouseFlags), 4);
        f.read(reinterpret_cast<char*>(&ev.mouseData),  4);
        if (!f) return false;
        ev.type        = static_cast<EventType>(t);
        ev.keyUp       = (ku  != 0);
        ev.extendedKey = (ext != 0);
        events.push_back(ev);
    }
    {
        std::lock_guard<std::mutex> lk(g_eventsMutex);
        g_recordedEvents = std::move(events);
    }
    return true;
}

void DoSave() {
    wchar_t path[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = g_mainWnd;
    ofn.lpstrFilter = L"TinyTask Kaydi (*.ttk)\0*.ttk\0Tum Dosyalar\0*.*\0";
    ofn.lpstrDefExt = L"ttk";
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (GetSaveFileNameW(&ofn)) {
        if (!SaveRecording(path))
            MessageBox(g_mainWnd, L"Dosya kaydedilemedi!", L"Hata", MB_OK | MB_ICONERROR);
        else
            MessageBox(g_mainWnd, L"Kayit basariyla kaydedildi.", L"Tamam", MB_OK | MB_ICONINFORMATION);
    }
}

void DoLoad() {
    wchar_t path[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = g_mainWnd;
    ofn.lpstrFilter = L"TinyTask Kaydi (*.ttk)\0*.ttk\0Tum Dosyalar\0*.*\0";
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&ofn)) {
        if (LoadRecording(path)) {
            RefreshEventList();
            UpdateCountLabel();
        } else {
            MessageBox(g_mainWnd, L"Dosya yuklenemedi veya gecersiz format.",
                       L"Hata", MB_OK | MB_ICONERROR);
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  Kontrolleri oluştur
// ─────────────────────────────────────────────────────────────
void CreateControls(HWND hwnd) {
    // ── Satır 1: düğmeler (y=10) ─────────────────────────────
    g_btnRecord = CreateWindow(L"BUTTON", L"Kaydi Baslat",
        WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
        10, 10, 145, 30, hwnd, (HMENU)ID_BTN_RECORD, g_hInst, nullptr);

    g_btnPlay = CreateWindow(L"BUTTON", L"Oynat (F9)",
        WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        162, 10, 90, 30, hwnd, (HMENU)ID_BTN_PLAY, g_hInst, nullptr);

    g_btnPause = CreateWindow(L"BUTTON", L"Duraklat",
        WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        258, 10, 90, 30, hwnd, (HMENU)ID_BTN_PAUSE, g_hInst, nullptr);

    g_btnStop = CreateWindow(L"BUTTON", L"Durdur (F11)",
        WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        354, 10, 100, 30, hwnd, (HMENU)ID_BTN_STOP, g_hInst, nullptr);

    g_chkLoop = CreateWindow(L"BUTTON", L"Loop",
        WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX,
        462, 16, 60, 22, hwnd, (HMENU)ID_CHK_LOOP, g_hInst, nullptr);

    // ── Satır 2: hız + durum (y=50) ──────────────────────────
    g_lblSpeed = CreateWindow(L"STATIC", L"Hiz: %100",
        WS_VISIBLE|WS_CHILD,
        10, 53, 90, 20, hwnd, (HMENU)ID_LBL_SPEED, g_hInst, nullptr);

    g_trackSpeed = CreateWindow(TRACKBAR_CLASS, nullptr,
        WS_VISIBLE|WS_CHILD|TBS_AUTOTICKS|TBS_TOOLTIPS,
        100, 48, 290, 28, hwnd, (HMENU)ID_TRACK_SPEED, g_hInst, nullptr);
    SendMessage(g_trackSpeed, TBM_SETRANGE,   TRUE, MAKELONG(25, 400));
    SendMessage(g_trackSpeed, TBM_SETPOS,     TRUE, 100);
    SendMessage(g_trackSpeed, TBM_SETTICFREQ, 25,   0);

    g_lblStatus = CreateWindow(L"STATIC", L"[   ] Bekliyor",
        WS_VISIBLE|WS_CHILD,
        400, 53, 160, 20, hwnd, (HMENU)ID_LBL_STATUS, g_hInst, nullptr);

    // ── Satır 3: araçlar (y=86) ───────────────────────────────
    g_btnClear = CreateWindow(L"BUTTON", L"Temizle",
        WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        10, 86, 72, 26, hwnd, (HMENU)ID_BTN_CLEAR, g_hInst, nullptr);

    g_btnDelete = CreateWindow(L"BUTTON", L"Secileni Sil",
        WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        88, 86, 95, 26, hwnd, (HMENU)ID_BTN_DELETE, g_hInst, nullptr);

    g_btnSave = CreateWindow(L"BUTTON", L"Kaydet",
        WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        190, 86, 72, 26, hwnd, (HMENU)ID_BTN_SAVE, g_hInst, nullptr);

    g_btnLoad = CreateWindow(L"BUTTON", L"Yukle",
        WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        268, 86, 72, 26, hwnd, (HMENU)ID_BTN_LOAD, g_hInst, nullptr);

    CreateWindow(L"STATIC", L"Tekrar:",
        WS_VISIBLE|WS_CHILD,
        352, 92, 46, 18, hwnd, nullptr, g_hInst, nullptr);

    g_editRepeat = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"1",
        WS_TABSTOP|WS_VISIBLE|WS_CHILD|ES_NUMBER|ES_RIGHT,
        400, 89, 44, 22, hwnd, (HMENU)ID_EDIT_REPEAT, g_hInst, nullptr);

    // UpDown buddy
    g_spinRepeat = CreateWindow(UPDOWN_CLASS, nullptr,
        WS_VISIBLE|WS_CHILD|UDS_SETBUDDYINT|UDS_ALIGNRIGHT|UDS_AUTOBUDDY|UDS_ARROWKEYS,
        0, 0, 0, 0, hwnd, (HMENU)ID_SPIN_REPEAT, g_hInst, nullptr);
    SendMessage(g_spinRepeat, UDM_SETRANGE32, 1, 9999);
    SendMessage(g_spinRepeat, UDM_SETPOS32,   0, 1);

    g_lblCount = CreateWindow(L"STATIC", L"0 olay",
        WS_VISIBLE|WS_CHILD|SS_RIGHT,
        450, 92, 110, 18, hwnd, (HMENU)ID_LBL_COUNT, g_hInst, nullptr);

    // ── Olay listesi (y=122) ──────────────────────────────────
    g_listEvents = CreateWindow(L"LISTBOX", nullptr,
        WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL|LBS_NOTIFY,
        10, 122, 560, 272, hwnd, (HMENU)ID_LIST_EVENTS, g_hInst, nullptr);

    // ── Kısayol ipucu (alt) ───────────────────────────────────
    CreateWindow(L"STATIC",
        L"F8 = Kayit Baslat/Durdur     F9 = Oynat     F10 = Duraklat/Devam     F11 = Durdur",
        WS_VISIBLE|WS_CHILD|SS_CENTER,
        10, 402, 560, 18, hwnd, (HMENU)ID_LBL_HOTKEYS, g_hInst, nullptr);

    UpdatePlaybackButtons();
}

// ─────────────────────────────────────────────────────────────
//  Ana pencere mesaj döngüsü
// ─────────────────────────────────────────────────────────────
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_CREATE:
        CreateControls(hwnd);
        RegisterHotKey(hwnd, ID_HK_TOGGLE_RECORD, MOD_NOREPEAT, VK_F8);
        RegisterHotKey(hwnd, ID_HK_PLAY,          MOD_NOREPEAT, VK_F9);
        RegisterHotKey(hwnd, ID_HK_PAUSE,         MOD_NOREPEAT, VK_F10);
        RegisterHotKey(hwnd, ID_HK_STOP,          MOD_NOREPEAT, VK_F11);
        return 0;

    case WM_HOTKEY:
        switch (static_cast<int>(wParam)) {
            case ID_HK_TOGGLE_RECORD: ToggleRecording();     break;
            case ID_HK_PLAY:          StartPlayback();       break;
            case ID_HK_PAUSE:         TogglePausePlayback(); break;
            case ID_HK_STOP:          StopPlayback();        break;
        }
        return 0;

    case WM_HSCROLL:
        if (reinterpret_cast<HWND>(lParam) == g_trackSpeed)
            UpdateSpeedLabel();
        return 0;

    case WM_APP_UPDATE_STATUS:
        SetStatusText(static_cast<StatusCode>(wParam));
        UpdatePlaybackButtons();
        return 0;

    case WM_APP_UPDATE_COUNT:
        UpdateCountLabel();
        return 0;

    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        switch (id) {
            case ID_BTN_RECORD: ToggleRecording();     break;
            case ID_BTN_PLAY:   StartPlayback();       break;
            case ID_BTN_PAUSE:  TogglePausePlayback(); break;
            case ID_BTN_STOP:   StopPlayback();        break;

            case ID_BTN_CLEAR:
                if (!g_playing.load() && !g_recording.load()) {
                    if (MessageBox(hwnd, L"Tum olaylar silinsin mi?", L"Onay",
                                   MB_YESNO|MB_ICONQUESTION) == IDYES) {
                        {
                            std::lock_guard<std::mutex> lk(g_eventsMutex);
                            g_recordedEvents.clear();
                        }
                        SendMessage(g_listEvents, LB_RESETCONTENT, 0, 0);
                        UpdateCountLabel();
                    }
                }
                break;

            case ID_BTN_DELETE: {
                if (!g_playing.load() && !g_recording.load()) {
                    const int sel = static_cast<int>(
                        SendMessage(g_listEvents, LB_GETCURSEL, 0, 0));
                    if (sel != LB_ERR) {
                        {
                            std::lock_guard<std::mutex> lk(g_eventsMutex);
                            if (sel < static_cast<int>(g_recordedEvents.size()))
                                g_recordedEvents.erase(g_recordedEvents.begin() + sel);
                        }
                        RefreshEventList();
                        UpdateCountLabel();
                        const int newCnt = static_cast<int>(
                            SendMessage(g_listEvents, LB_GETCOUNT, 0, 0));
                        if (newCnt > 0)
                            SendMessage(g_listEvents, LB_SETCURSEL,
                                        std::min(sel, newCnt - 1), 0);
                    }
                }
                break;
            }

            case ID_BTN_SAVE:
                if (!g_playing.load() && !g_recording.load()) DoSave();
                break;
            case ID_BTN_LOAD:
                if (!g_playing.load() && !g_recording.load()) DoLoad();
                break;
        }
        return 0;
    }

    case WM_DESTROY:
        UnregisterHotKey(hwnd, ID_HK_TOGGLE_RECORD);
        UnregisterHotKey(hwnd, ID_HK_PLAY);
        UnregisterHotKey(hwnd, ID_HK_PAUSE);
        UnregisterHotKey(hwnd, ID_HK_STOP);
        g_recording.store(false);
        StopHooks();
        StopPlayback();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ─────────────────────────────────────────────────────────────
//  Giriş noktası
// ─────────────────────────────────────────────────────────────
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    g_hInst = hInst;

    // DPI farkındalığı
    if (HMODULE u32 = GetModuleHandleW(L"user32.dll")) {
        using FnCtx  = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
        using FnAwar = BOOL(WINAPI*)();
        if (auto f = reinterpret_cast<FnCtx>(GetProcAddress(u32, "SetProcessDpiAwarenessContext")))
            f(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        else if (auto g = reinterpret_cast<FnAwar>(GetProcAddress(u32, "SetProcessDPIAware")))
            g();
    }

    INITCOMMONCONTROLSEX icex{ sizeof(icex), ICC_BAR_CLASSES | ICC_UPDOWN_CLASS };
    InitCommonControlsEx(&icex);

    WNDCLASSEX wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"TinyTaskProWin";
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClassEx(&wc);

    g_mainWnd = CreateWindow(
        L"TinyTaskProWin", L"TinyTask Pro",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 595, 460,
        nullptr, nullptr, hInst, nullptr);
    if (!g_mainWnd) return 1;

    ShowWindow(g_mainWnd, nCmdShow);
    UpdateWindow(g_mainWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}