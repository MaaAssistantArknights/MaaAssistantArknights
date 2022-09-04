#include "OcrPack.h"

#include <filesystem>

#include "NoWarningCV.h"
#include <PaddleOCR/paddle_ocr.h>

#include "AsstUtils.hpp"
#include "Logger.hpp"

#ifdef _WIN32
#include "SafeWindows.h"
#include <format>
static std::filesystem::path prepare_paddle_dir(const std::filesystem::path& dir, bool* is_temp);
#else
static std::filesystem::path prepare_paddle_dir(const std::filesystem::path& dir, bool* is_temp)
{
    *is_temp = false;
    return dir;
}
#endif

asst::OcrPack::OcrPack()
{
    Log.info("hardware_concurrency:", std::thread::hardware_concurrency());
    for (size_t i = 0; i != MaxBoxSize; ++i) {
        static constexpr size_t MaxTextSize = 1024;
        *(m_strs_buffer + i) = new char[MaxTextSize];
        // memset(*(m_strs_buffer + i), 0, MaxTextSize);
    }
}

asst::OcrPack::~OcrPack()
{
    for (size_t i = 0; i != MaxBoxSize; ++i) {
        delete[] *(m_strs_buffer + i);
    }

    PaddleOcrDestroy(m_ocr);

}

bool asst::OcrPack::load(const std::filesystem::path& path)
{
    bool use_temp_dir = false;
    auto paddle_dir = prepare_paddle_dir(path, &use_temp_dir);

    if (paddle_dir.empty() || !std::filesystem::exists(paddle_dir)) {
        return false;
    }

    constexpr static auto DetName = "det";
    // constexpr static const char* ClsName = "cls";
    constexpr static auto RecName = "rec";
    constexpr static auto KeysName = "ppocr_keys_v1.txt";

    const auto dst_filename = paddle_dir / asst::utils::path(DetName);
    // const std::string cls_filename = dir + ClsName;
    const auto rec_filename = paddle_dir / asst::utils::path(RecName);
    const auto keys_filename = paddle_dir / asst::utils::path(KeysName);

    if (m_ocr != nullptr) {
        PaddleOcrDestroy(m_ocr);
    }

    const auto det4paddle = asst::utils::path_to_ansi_string(dst_filename);
    const auto rec4paddle = asst::utils::path_to_ansi_string(rec_filename);
    const auto keys4paddle = asst::utils::path_to_ansi_string(keys_filename);

    if (det4paddle.empty() || rec4paddle.empty() || keys4paddle.empty()) {
        return false;
    }

    m_ocr = PaddleOcrCreate(det4paddle.c_str(), rec4paddle.c_str(), keys4paddle.c_str(), nullptr);

    if (use_temp_dir) {
        // files can be removed after load
        std::thread([paddle_dir]() {
            for (int i = 0; i < 50; i++) {
                if (std::filesystem::remove(paddle_dir)) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }).detach();
    }

    return m_ocr != nullptr;
}

std::vector<asst::TextRect> asst::OcrPack::recognize(const cv::Mat& image, const asst::TextRectProc& pred,
                                                     bool without_det)
{
    size_t size = 0;

    std::vector<uchar> buf;
    cv::imencode(".png", image, buf);

    if (!without_det) {
        Log.trace("Ocr System");
        PaddleOcrSystem(m_ocr, buf.data(), buf.size(), false, m_boxes_buffer, m_strs_buffer, m_scores_buffer, &size,
                        nullptr, nullptr);
    }
    else {
        Log.trace("Ocr Rec");
        PaddleOcrRec(m_ocr, buf.data(), buf.size(), m_strs_buffer, m_scores_buffer, &size, nullptr, nullptr);
    }

    std::vector<TextRect> result;
    std::string log_str_raw;
    std::string log_str_proc;

#ifdef ASST_DEBUG
    cv::Mat draw = image.clone();
#endif

    for (size_t i = 0; i != size; ++i) {
        // the box rect like ↓
        // 0 - 1
        // 3 - 2
        Rect rect;
        if (!without_det) {
            int* box = m_boxes_buffer + i * 8;
            int x_collect[4] = { *(box + 0), *(box + 2), *(box + 4), *(box + 6) };
            int y_collect[4] = { *(box + 1), *(box + 3), *(box + 5), *(box + 7) };
            int left = static_cast<int>(*std::min_element(x_collect, x_collect + 4));
            int right = static_cast<int>(*std::max_element(x_collect, x_collect + 4));
            int top = static_cast<int>(*std::min_element(y_collect, y_collect + 4));
            int bottom = static_cast<int>(*std::max_element(y_collect, y_collect + 4));
            rect = Rect(left, top, right - left, bottom - top);
        }
        std::string text(*(m_strs_buffer + i));
        float score = *(m_scores_buffer + i);
        if (score > 2.0) {
            score = 0;
        }

        TextRect tr { score, rect, text };
#ifdef ASST_DEBUG
        cv::rectangle(draw, utils::make_rect<cv::Rect>(rect), cv::Scalar(0, 0, 255), 2);
#endif
        log_str_raw += tr.to_string() + ", ";
        if (!pred || pred(tr)) {
            log_str_proc += tr.to_string() + ", ";
            result.emplace_back(std::move(tr));
        }
    }

    Log.trace("OcrPack::recognize | raw : ", log_str_raw);
    Log.trace("OcrPack::recognize | proc : ", log_str_proc);
    return result;
}

std::vector<asst::TextRect> asst::OcrPack::recognize(const cv::Mat& image, const Rect& roi,
                                                     const asst::TextRectProc& pred, bool without_det)
{
    auto rect_cor = [&roi, &pred, &without_det](TextRect& tr) -> bool {
        if (without_det) {
            tr.rect = roi;
        }
        else {
            tr.rect.x += roi.x;
            tr.rect.y += roi.y;
        }
        return pred(tr);
    };
    Log.trace("OcrPack::recognize | roi : ", roi.to_string());
    cv::Mat roi_img = image(utils::make_rect<cv::Rect>(roi));
    return recognize(roi_img, rect_cor, without_det);
}


#ifdef _WIN32

#define REPARSE_MOUNTPOINT_HEADER_SIZE 8

struct REPARSE_MOUNTPOINT_DATA_BUFFER
{
    DWORD ReparseTag;
    DWORD ReparseDataLength;
    WORD Reserved;
    WORD ReparseTargetLength;
    WORD ReparseTargetMaximumLength;
    WORD Reserved1;
    WCHAR ReparseTarget[1];
};

struct REPARSE_DATA_BUFFER
{
    DWORD ReparseTag;
    WORD ReparseDataLength;
    WORD Reserved;
    union
    {
        struct
        {
            WORD SubstituteNameOffset;
            WORD SubstituteNameLength;
            WORD PrintNameOffset;
            WORD PrintNameLength;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct
        {
            WORD SubstituteNameOffset;
            WORD SubstituteNameLength;
            WORD PrintNameOffset;
            WORD PrintNameLength;
            WCHAR PathBuffer[1];
        } MountPointReparseBuffer;
        struct
        {
            BYTE DataBuffer[1];
        } GenericReparseBuffer;
    };
};

#define REPARSE_DATA_BUFFER_HEADER_SIZE FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)

static HANDLE OpenDirectory(LPCWSTR pszPath, BOOL bReadWrite)
{
    // Obtain backup/restore privilege in case we don't have it
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
    LookupPrivilegeValueW(NULL, (bReadWrite ? SE_RESTORE_NAME : SE_BACKUP_NAME), &tp.Privileges[0].Luid);
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    CloseHandle(hToken);

    // Open the directory
    DWORD dwAccess = bReadWrite ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;
    HANDLE hDir = CreateFileW(pszPath, dwAccess, 0, NULL, OPEN_EXISTING,
                               FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);

    return hDir;
}

static std::filesystem::path prepare_paddle_dir(const std::filesystem::path& dir, bool* is_temp)
{
    static std::atomic<uint64_t> id{};

    *is_temp = false;
    auto path = asst::utils::path(asst::utils::string_replace_all(asst::utils::path_to_utf8_string(dir), "/", "\\"));
    if (!asst::utils::path_to_ansi_string(path).empty()) {
        // can be passed to paddle via path_to_ansi_string
        return path;
    }
    // fallback: create junction (reparse point) in user temp directory
    wchar_t tempbuf[MAX_PATH + 1];
    auto templen = GetTempPathW(MAX_PATH + 1, tempbuf);
    std::filesystem::path tempdir(std::wstring_view(tempbuf, templen));
    if (asst::utils::path_to_ansi_string(tempdir).empty()) {
        asst::Log.error("failed to escape unicode path: temp dir cannot be escaped");
        // cannot escape temp dir, no luck
        return {};
    }
    auto pid = GetCurrentProcessId();
    while (1) {
        auto dirname = std::format(L"MaaLink-{}-{}", pid, id++);
        auto linkdir = tempdir / dirname;
        if (CreateDirectoryW(linkdir.c_str(), nullptr)) {
            // prepare link target (NT path)

            auto normtarget = L"\\??\\" + std::filesystem::absolute(path).native();
            if (normtarget.back() != L'\\') normtarget.push_back(L'\\');

            // set reparse point
            auto hReparsePoint = OpenDirectory(linkdir.c_str(), TRUE);

            BYTE buf[sizeof(REPARSE_MOUNTPOINT_DATA_BUFFER) + MAX_PATH * sizeof(WCHAR)];
            REPARSE_MOUNTPOINT_DATA_BUFFER& ReparseBuffer = (REPARSE_MOUNTPOINT_DATA_BUFFER&)buf;

            // Prepare reparse point data
            memset(buf, 0, sizeof(buf));
            ReparseBuffer.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
            wcsncpy_s(ReparseBuffer.ReparseTarget, MAX_PATH + 1, normtarget.c_str(), MAX_PATH);
            ReparseBuffer.ReparseTargetMaximumLength = (WORD)((normtarget.size() + 1) * sizeof(WCHAR));
            ReparseBuffer.ReparseTargetLength = (WORD)(normtarget.size() * sizeof(WCHAR));
            ReparseBuffer.ReparseDataLength = ReparseBuffer.ReparseTargetLength + 12;

            // Attach reparse point
            auto success = DeviceIoControl(hReparsePoint, FSCTL_SET_REPARSE_POINT, &ReparseBuffer,
                                           ReparseBuffer.ReparseDataLength + REPARSE_MOUNTPOINT_HEADER_SIZE, nullptr, 0,
                                           nullptr, nullptr);

            CloseHandle(hReparsePoint);

            if (success) {
                *is_temp = true;
                return linkdir;
            }
            else {
                asst::Log.error("failed to escape unicode path: failed to create junction");
                return {};
            }

        } else {
            auto err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS) {
                // cannot create link, no luck
                asst::Log.error("failed to escape unicode path: failed to create junction");
                return {};
            }
        }
    }
}

#endif
