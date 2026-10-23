#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <algorithm>
#include <string>
#include <vector>

#include "RepairWorkshop.h"

namespace {
constexpr int CustomerEdit = 101, DeviceEdit = 102, IssueEdit = 103;
constexpr int AddButton = 201, ServeButton = 202, ClearButton = 203;
constexpr COLORREF Canvas = RGB(247, 246, 242), Surface = RGB(255, 255, 255);
constexpr COLORREF Text = RGB(42, 50, 55), Muted = RGB(112, 122, 126);
constexpr COLORREF Line = RGB(222, 225, 221), Teal = RGB(31, 112, 99);
constexpr COLORREF TealWash = RGB(231, 243, 239), Red = RGB(165, 62, 61);
constexpr COLORREF RedWash = RGB(250, 235, 233), Green = RGB(43, 128, 93);
constexpr COLORREF Amber = RGB(157, 105, 28);
constexpr UINT BaseDpi = 96;

RepairWorkshop workshop;
HWND customerEdit = nullptr, deviceEdit = nullptr, issueEdit = nullptr;
HWND addButton = nullptr, serveButton = nullptr, clearButton = nullptr;
HBRUSH editBrush = nullptr;
HFONT editFont = nullptr;
UINT currentDpi = BaseDpi;
std::string banner = "Ready to register a repair request.";
COLORREF bannerColor = Muted;
std::string customerError, deviceError, issueError;

struct Rect { int x, y, w, h; };
struct Layout {
    int width = 0, height = 0, margin = 28;
    Rect form{}, queue{}, customer{}, device{}, issue{}, add{}, clear{}, serve{};
};
Layout layout;

int dp(int logicalPixels) { return MulDiv(logicalPixels, static_cast<int>(currentDpi), static_cast<int>(BaseDpi)); }

std::wstring wide(const std::string& value) {
    if (value.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (n <= 0) return {};
    std::wstring result(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), n);
    return result;
}
std::string utf8(HWND edit) {
    const int length = GetWindowTextLengthW(edit);
    std::wstring value(static_cast<std::size_t>(length) + 1, L'\0');
    GetWindowTextW(edit, value.data(), length + 1);
    value.resize(static_cast<std::size_t>(length));
    const int n = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), length, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string result(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), length, result.data(), n, nullptr, nullptr);
    return result;
}
HFONT makeFont(HDC dc, int points, bool bold) {
    return CreateFontW(-MulDiv(points, GetDeviceCaps(dc, LOGPIXELSY), 72), 0, 0, 0,
        bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}
void label(HDC dc, int x, int y, int width, const std::string& value, COLORREF color = Text,
           int points = 10, bool bold = false, UINT flags = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS) {
    SetTextColor(dc, color); SetBkMode(dc, TRANSPARENT);
    HFONT font = makeFont(dc, points, bold); auto old = SelectObject(dc, font);
    const auto rendered = wide(value); RECT r{x, y, x + width, y + dp(28)};
    DrawTextW(dc, rendered.c_str(), static_cast<int>(rendered.size()), &r, flags | DT_NOPREFIX);
    SelectObject(dc, old); DeleteObject(font);
}
void fieldError(HDC dc, int x, int y, int width, const std::string& value) {
    if (value.empty()) return;
    SetTextColor(dc, Red); SetBkMode(dc, TRANSPARENT);
    HFONT font = makeFont(dc, 8, false); auto old = SelectObject(dc, font);
    const auto rendered = wide(value); RECT r{x, y, x + width, y + dp(16)};
    DrawTextW(dc, rendered.c_str(), static_cast<int>(rendered.size()), &r,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(dc, old); DeleteObject(font);
}
void panel(HDC dc, Rect r, COLORREF color = Surface) {
    HBRUSH brush = CreateSolidBrush(color); HPEN pen = CreatePen(PS_SOLID, std::max(1, dp(1)), Line);
    auto oldBrush = SelectObject(dc, brush); auto oldPen = SelectObject(dc, pen);
    Rectangle(dc, r.x, r.y, r.x + r.w, r.y + r.h);
    SelectObject(dc, oldBrush); SelectObject(dc, oldPen); DeleteObject(brush); DeleteObject(pen);
}
void hline(HDC dc, int x, int y, int width, COLORREF color = Line) {
    HPEN pen = CreatePen(PS_SOLID, std::max(1, dp(1)), color); auto old = SelectObject(dc, pen);
    MoveToEx(dc, x, y, nullptr); LineTo(dc, x + width, y);
    SelectObject(dc, old); DeleteObject(pen);
}
void centerLabel(HDC dc, Rect r, const std::string& value, COLORREF color, int points, bool bold = false) {
    SetTextColor(dc, color); SetBkMode(dc, TRANSPARENT);
    HFONT font = makeFont(dc, points, bold); auto old = SelectObject(dc, font);
    const auto rendered = wide(value); RECT box{r.x, r.y, r.x + r.w, r.y + r.h};
    DrawTextW(dc, rendered.c_str(), static_cast<int>(rendered.size()), &box,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(dc, old); DeleteObject(font);
}
void setBanner(std::string message, COLORREF color) { banner = std::move(message); bannerColor = color; }

void updateButtons() {
    const auto view = workshop.queueView();
    if (serveButton) EnableWindow(serveButton, view.size != 0);
    if (addButton) EnableWindow(addButton, view.size != CircularTicketQueue::Capacity);
}

void updateLayout(int width, int height) {
    layout.width = width; layout.height = height;
    layout.margin = dp(28);
    const int m = layout.margin;
    const int gap = dp(18);
    const int formWidth = std::clamp(width * 34 / 100, dp(350), dp(430));
    const int rightX = m + formWidth + gap;
    const int rightWidth = std::max(dp(300), width - rightX - m);
    const int workTop = dp(270);
    const int footerTop = height - dp(44);
    const int workBottom = footerTop - dp(14);
    const int workHeight = std::max(dp(330), workBottom - workTop);
    layout.form = {m, workTop, formWidth, workHeight};
    layout.queue = {rightX, workTop, rightWidth, workHeight};

    const int fieldX = m + dp(20);
    const int fieldWidth = formWidth - dp(40);
    const int fieldHeight = dp(31);
    layout.customer = {fieldX, workTop + dp(81), fieldWidth, fieldHeight};
    layout.device = {fieldX, workTop + dp(159), fieldWidth, fieldHeight};
    const int buttonY = workTop + workHeight - dp(42);
    const int issueHeight = std::clamp(buttonY - (workTop + dp(231)) - dp(20), dp(46), dp(106));
    layout.issue = {fieldX, workTop + dp(231), fieldWidth, issueHeight};
    const int clearWidth = std::min(dp(88), fieldWidth / 3);
    layout.add = {fieldX, buttonY, fieldWidth - clearWidth - dp(10), dp(34)};
    layout.clear = {fieldX + fieldWidth - clearWidth, buttonY, clearWidth, dp(34)};
    layout.serve = {rightX + rightWidth - dp(174), workTop + dp(15), dp(154), dp(34)};

    if (customerEdit) MoveWindow(customerEdit, layout.customer.x, layout.customer.y, layout.customer.w, layout.customer.h, TRUE);
    if (deviceEdit) MoveWindow(deviceEdit, layout.device.x, layout.device.y, layout.device.w, layout.device.h, TRUE);
    if (issueEdit) MoveWindow(issueEdit, layout.issue.x, layout.issue.y, layout.issue.w, layout.issue.h, TRUE);
    if (addButton) MoveWindow(addButton, layout.add.x, layout.add.y, layout.add.w, layout.add.h, TRUE);
    if (clearButton) MoveWindow(clearButton, layout.clear.x, layout.clear.y, layout.clear.w, layout.clear.h, TRUE);
    if (serveButton) MoveWindow(serveButton, layout.serve.x, layout.serve.y, layout.serve.w, layout.serve.h, TRUE);
}

HFONT createEditFont() {
    return CreateFontW(-MulDiv(9, static_cast<int>(currentDpi), 72), 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}
void applyEditFont() {
    HFONT replacement = createEditFont();
    for (HWND h : {customerEdit, deviceEdit, issueEdit}) if (h) SendMessageW(h, WM_SETFONT, (WPARAM)replacement, TRUE);
    if (editFont) DeleteObject(editFont);
    editFont = replacement;
}

void createControls(HWND hwnd) {
    const DWORD editStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL;
    customerEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", editStyle, 0, 0, 0, 0, hwnd, (HMENU)CustomerEdit, nullptr, nullptr);
    deviceEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", editStyle, 0, 0, 0, 0, hwnd, (HMENU)DeviceEdit, nullptr, nullptr);
    issueEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL,
                                0, 0, 0, 0, hwnd, (HMENU)IssueEdit, nullptr, nullptr);
    addButton = CreateWindowExW(0, L"BUTTON", L"Register ticket", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                0, 0, 0, 0, hwnd, (HMENU)AddButton, nullptr, nullptr);
    clearButton = CreateWindowExW(0, L"BUTTON", L"Clear", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                  0, 0, 0, 0, hwnd, (HMENU)ClearButton, nullptr, nullptr);
    serveButton = CreateWindowExW(0, L"BUTTON", L"Serve next ticket", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                  0, 0, 0, 0, hwnd, (HMENU)ServeButton, nullptr, nullptr);
    SendMessageW(customerEdit, EM_SETLIMITTEXT, 60, 0);
    SendMessageW(deviceEdit, EM_SETLIMITTEXT, 40, 0);
    SendMessageW(issueEdit, EM_SETLIMITTEXT, 200, 0);
    SendMessageW(customerEdit, EM_SETCUEBANNER, FALSE, (LPARAM)L"e.g. Harry Fernando");
    SendMessageW(deviceEdit, EM_SETCUEBANNER, FALSE, (LPARAM)L"e.g. iPhone 15");
    SendMessageW(issueEdit, EM_SETCUEBANNER, FALSE, (LPARAM)L"Describe the reported problem");
    applyEditFont();
    updateLayout(dp(1200), dp(760));
    updateButtons();
}

void drawDashboard(HWND hwnd) {
    PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
    RECT client; GetClientRect(hwnd, &client);
    HBRUSH background = CreateSolidBrush(Canvas); FillRect(dc, &client, background); DeleteObject(background);
    const auto s = [](int value) { return dp(value); };
    const int w = layout.width, h = layout.height, m = layout.margin;

    label(dc, m, s(16), s(230), "MendoraQ", Text, 22, true);
    label(dc, m + s(152), s(22), std::max(s(300), w - m - s(180)), "REPAIR WORKSHOP  /  SERVICE QUEUE", Muted, 9, true);
    hline(dc, m, s(58), w - 2 * m, Teal);

    const auto view = workshop.queueView();
    const auto tickets = workshop.waitingTickets();
    const auto count = view.size;
    const int statWidth = (w - 2 * m) / 3;
    const int summaryY = s(70);
    const std::string status = count == 0 ? "Empty" : (count == CircularTicketQueue::Capacity ? "Full" : "Available");
    label(dc, m + s(4), summaryY, statWidth - s(16), "WAITING TICKETS", Muted, 8, true);
    label(dc, m + s(4), summaryY + s(22), statWidth - s(16), std::to_string(count) + " / 8", Teal, 18, true);
    hline(dc, m + statWidth, summaryY + s(5), s(1), Line);
    label(dc, m + statWidth + s(18), summaryY, statWidth - s(28), "COMPLETED JOBS", Muted, 8, true);
    label(dc, m + statWidth + s(18), summaryY + s(22), statWidth - s(28), std::to_string(workshop.completedJobs()), Text, 18, true);
    hline(dc, m + statWidth * 2, summaryY + s(5), s(1), Line);
    label(dc, m + statWidth * 2 + s(18), summaryY, statWidth - s(28), "QUEUE STATUS", Muted, 8, true);
    label(dc, m + statWidth * 2 + s(18), summaryY + s(22), statWidth - s(28), status,
          count == 0 ? Muted : (count == CircularTicketQueue::Capacity ? Amber : Teal), 18, true);

    const Rect diagram{m, s(142), w - 2 * m, s(112)};
    panel(dc, diagram);
    label(dc, diagram.x + s(15), diagram.y + s(7), s(190), "CIRCULAR BUFFER", Text, 9, true);
    label(dc, diagram.x + s(15), diagram.y + s(31), s(170), "Physical slot index", Muted, 8);
    label(dc, diagram.x + s(15), diagram.y + s(53), s(175), "REAR = next insertion", Muted, 8);
    const int slotsX = diagram.x + s(194);
    const int available = diagram.w - s(210);
    const int slotStep = available / static_cast<int>(CircularTicketQueue::Capacity);
    for (std::size_t i = 0; i < CircularTicketQueue::Capacity; ++i) {
        const int sx = slotsX + static_cast<int>(i) * slotStep;
        const int slotWidth = std::max(s(48), slotStep - s(5));
        const bool occupied = i < view.slotTicketIds.size() && !view.slotTicketIds[i].empty();
        const bool front = i == view.frontIndex;
        const bool rear = i == view.rearIndex;
        Rect cell{sx, diagram.y + s(30), slotWidth, s(51)};
        panel(dc, cell, occupied ? (front ? TealWash : RGB(246, 249, 247)) : Surface);
        centerLabel(dc, {sx, diagram.y + s(31), slotWidth, s(17)}, std::to_string(i), Muted, 8, true);
        centerLabel(dc, {sx + s(2), diagram.y + s(49), slotWidth - s(4), s(20)}, occupied ? view.slotTicketIds[i] : "—",
                    occupied ? (front ? Teal : Text) : Muted, 9, occupied);
        std::string marker;
        if (front && rear) marker = "FRONT / REAR";
        else if (front) marker = "FRONT";
        else if (rear) marker = "REAR";
        if (!marker.empty()) centerLabel(dc, {sx - s(3), diagram.y + s(82), slotWidth + s(1), s(20)}, marker,
                                        front ? Teal : Amber, 7, true);
    }

    panel(dc, layout.form);
    label(dc, layout.form.x + s(20), layout.form.y + s(12), layout.form.w - s(40), "Register a repair", Text, 15, true);
    label(dc, layout.form.x + s(20), layout.form.y + s(39), layout.form.w - s(40), "New requests join the rear of the queue.", Muted, 9);
    label(dc, layout.form.x + s(20), layout.form.y + s(60), layout.form.w - s(40), "CUSTOMER NAME  ·  2–60", Muted, 8, true);
    fieldError(dc, layout.form.x + s(20), layout.customer.y + layout.customer.h + s(1), layout.form.w - s(40), customerError);
    label(dc, layout.form.x + s(20), layout.device.y - s(23), layout.form.w - s(40), "DEVICE / ITEM  ·  2–40", Muted, 8, true);
    fieldError(dc, layout.form.x + s(20), layout.device.y + layout.device.h + s(1), layout.form.w - s(40), deviceError);
    label(dc, layout.form.x + s(20), layout.issue.y - s(23), layout.form.w - s(40), "REPORTED ISSUE  ·  5–200", Muted, 8, true);
    fieldError(dc, layout.form.x + s(20), layout.issue.y + layout.issue.h + s(1), layout.form.w - s(40), issueError);

    panel(dc, layout.queue);
    label(dc, layout.queue.x + s(18), layout.queue.y + s(12), layout.queue.w - s(220), "Waiting queue", Text, 15, true);
    label(dc, layout.queue.x + s(18), layout.queue.y + s(42), layout.queue.w - s(36), "FIFO order  ·  FRONT points to the next ticket served", Muted, 9);
    const int tableX = layout.queue.x + s(18);
    const int tableY = layout.queue.y + s(76);
    const int tableWidth = layout.queue.w - s(36);
    const int idW = std::max(s(68), tableWidth * 13 / 100);
    const int nameW = tableWidth * 25 / 100;
    const int deviceW = tableWidth * 21 / 100;
    const int issueW = tableWidth - idW - nameW - deviceW;
    label(dc, tableX, tableY, idW, "TICKET ID", Muted, 8, true);
    label(dc, tableX + idW, tableY, nameW, "CUSTOMER", Muted, 8, true);
    label(dc, tableX + idW + nameW, tableY, deviceW, "DEVICE", Muted, 8, true);
    label(dc, tableX + idW + nameW + deviceW, tableY, issueW, "REPORTED ISSUE", Muted, 8, true);
    hline(dc, tableX, tableY + s(25), tableWidth);
    if (tickets.empty()) {
        label(dc, tableX, tableY + s(91), tableWidth, "No tickets are waiting", Muted, 12, true, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        label(dc, tableX, tableY + s(120), tableWidth, "Register a repair to begin the FIFO queue.", Muted, 9, false,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    const int rowHeight = std::clamp((layout.queue.h - s(115)) / 8, s(25), s(38));
    int rowY = tableY + s(31);
    for (std::size_t i = 0; i < tickets.size(); ++i) {
        const bool next = i == 0;
        if (next) { HBRUSH rowBrush = CreateSolidBrush(TealWash); RECT rr{tableX, rowY, tableX + tableWidth, rowY + rowHeight}; FillRect(dc, &rr, rowBrush); DeleteObject(rowBrush); }
        label(dc, tableX + s(7), rowY, idW - s(9), tickets[i].id, next ? Teal : Text, 9, next);
        label(dc, tableX + idW, rowY, nameW - s(5), tickets[i].customerName, Text, 9);
        label(dc, tableX + idW + nameW, rowY, deviceW - s(5), tickets[i].deviceName, Text, 9);
        label(dc, tableX + idW + nameW + deviceW, rowY, issueW - s(2), tickets[i].issueDescription, Muted, 9);
        rowY += rowHeight;
        hline(dc, tableX, rowY, tableWidth, RGB(237, 239, 236));
    }

    const int footerY = h - s(40);
    label(dc, m, footerY, w - 2 * m, banner, bannerColor, 9, true);
    label(dc, m, footerY + s(15), w - 2 * m,
          "CIRCULAR FIFO  ·  ENQUEUE AND DEQUEUE ARE O(1)  ·  REAR MARKS THE NEXT INSERTION POSITION",
          Muted, 7, false);
    EndPaint(hwnd, &ps);
}

void submit(HWND hwnd) {
    const auto validation = validateTicketFields(utf8(customerEdit), utf8(deviceEdit), utf8(issueEdit));
    customerError = validation.customerError; deviceError = validation.deviceError; issueError = validation.issueError;
    if (!validation.valid()) {
        setBanner("Correct the highlighted fields. Nothing was added to the queue.", Red);
        InvalidateRect(hwnd, nullptr, FALSE); return;
    }
    std::string id;
    if (!workshop.registerTicket(validation.customerName, validation.deviceName, validation.issueDescription, id)) {
        setBanner("Registration was rejected. The queue may be full; no ID was consumed.", Red);
        InvalidateRect(hwnd, nullptr, FALSE); return;
    }
    SetWindowTextW(customerEdit, L""); SetWindowTextW(deviceEdit, L""); SetWindowTextW(issueEdit, L"");
    customerError.clear(); deviceError.clear(); issueError.clear();
    setBanner("Ticket " + id + " registered. It was placed in the next rear slot.", Green);
    updateButtons(); InvalidateRect(hwnd, nullptr, FALSE);
}

void drawButton(const DRAWITEMSTRUCT& item) {
    const bool enabled = IsWindowEnabled(item.hwndItem) != FALSE;
    const bool primary = item.CtlID == AddButton;
    const bool clear = item.CtlID == ClearButton;
    const bool pressed = (item.itemState & ODS_SELECTED) != 0;
    const bool focused = (item.itemState & ODS_FOCUS) != 0;
    const bool hovered = (item.itemState & ODS_HOTLIGHT) != 0;
    const COLORREF fillColor = !enabled ? RGB(235, 236, 233) :
        (primary ? (pressed ? RGB(22, 88, 78) : (hovered ? RGB(25, 96, 85) : Teal)) :
         (clear ? (hovered ? RGB(245, 247, 244) : Surface) : (hovered ? RGB(224, 239, 232) : RGB(236, 244, 240))));
    const COLORREF ink = !enabled ? RGB(160, 164, 160) : (primary ? Surface : (clear ? Muted : Teal));
    HBRUSH brush = CreateSolidBrush(fillColor); HPEN pen = CreatePen(PS_SOLID, std::max(1, dp(1)), clear ? Line : fillColor);
    auto oldBrush = SelectObject(item.hDC, brush); auto oldPen = SelectObject(item.hDC, pen);
    Rectangle(item.hDC, item.rcItem.left, item.rcItem.top, item.rcItem.right, item.rcItem.bottom);
    SelectObject(item.hDC, oldBrush); SelectObject(item.hDC, oldPen); DeleteObject(brush); DeleteObject(pen);
    const wchar_t* caption = item.CtlID == AddButton ? L"Register ticket" : (item.CtlID == ClearButton ? L"Clear" : L"Serve next ticket");
    SetBkMode(item.hDC, TRANSPARENT); SetTextColor(item.hDC, ink);
    HFONT font = makeFont(item.hDC, 9, !clear); auto oldFont = SelectObject(item.hDC, font);
    RECT rc = item.rcItem; DrawTextW(item.hDC, caption, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (focused) { InflateRect(&rc, -dp(5), -dp(5)); DrawFocusRect(item.hDC, &rc); }
    SelectObject(item.hDC, oldFont); DeleteObject(font);
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    switch (message) {
    case WM_CREATE: {
        currentDpi = GetDpiForWindow(hwnd);
        if (currentDpi == 0) currentDpi = BaseDpi;
        editBrush = CreateSolidBrush(Surface);
        createControls(hwnd);
        return 0;
    }
    case WM_SIZE:
        updateLayout(LOWORD(lp), HIWORD(lp));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lp);
        info->ptMinTrackSize.x = dp(1100);
        info->ptMinTrackSize.y = dp(710);
        return 0;
    }
    case WM_DPICHANGED: {
        const UINT newDpi = LOWORD(wp) ? LOWORD(wp) : HIWORD(wp);
        if (newDpi != 0) currentDpi = newDpi;
        applyEditFont();
        const auto* suggested = reinterpret_cast<const RECT*>(lp);
        SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
                     suggested->right - suggested->left, suggested->bottom - suggested->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        RECT client{}; GetClientRect(hwnd, &client);
        updateLayout(client.right - client.left, client.bottom - client.top);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    case WM_PAINT: drawDashboard(hwnd); return 0;
    case WM_CTLCOLOREDIT: {
        HDC dc = reinterpret_cast<HDC>(wp);
        SetTextColor(dc, Text); SetBkColor(dc, Surface); SetDCBrushColor(dc, Surface);
        return reinterpret_cast<LRESULT>(editBrush);
    }
    case WM_DRAWITEM:
        drawButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lp));
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp) == AddButton && HIWORD(wp) == BN_CLICKED) { submit(hwnd); return 0; }
        if (LOWORD(wp) == ClearButton && HIWORD(wp) == BN_CLICKED) {
            SetWindowTextW(customerEdit, L""); SetWindowTextW(deviceEdit, L""); SetWindowTextW(issueEdit, L"");
            customerError.clear(); deviceError.clear(); issueError.clear();
            setBanner("Form cleared.", Muted); SetFocus(customerEdit); InvalidateRect(hwnd, nullptr, FALSE); return 0;
        }
        if (LOWORD(wp) == ServeButton && HIWORD(wp) == BN_CLICKED) {
            RepairTicket ticket;
            if (workshop.serveNextTicket(ticket)) setBanner("Ticket " + ticket.id + " served for " + ticket.customerName + ". Completed job recorded.", Green);
            else setBanner("There are no tickets to serve.", Red);
            updateButtons(); InvalidateRect(hwnd, nullptr, FALSE); return 0;
        }
        break;
    case WM_DESTROY:
        if (editBrush) DeleteObject(editBrush);
        if (editFont) DeleteObject(editFont);
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, message, wp, lp);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    currentDpi = GetDpiForSystem();
    if (currentDpi == 0) currentDpi = BaseDpi;
    const wchar_t* className = L"MendoraQWorkshopWindow";
    WNDCLASSW wc{}; wc.lpfnWndProc = windowProc; wc.hInstance = instance; wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW); wc.hbrBackground = nullptr;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassW(&wc)) return 1;
    HWND hwnd = CreateWindowExW(0, className, L"MendoraQ — Repair Workshop Manager",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, dp(1220), dp(710), nullptr, nullptr, instance, nullptr);
    if (!hwnd) return 1;
    ShowWindow(hwnd, show); UpdateWindow(hwnd);
    MSG msg{}; while (GetMessageW(&msg, nullptr, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return static_cast<int>(msg.wParam);
}
#endif
