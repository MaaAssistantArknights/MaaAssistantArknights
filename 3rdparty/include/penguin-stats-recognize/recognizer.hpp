#ifndef PENGUIN_RECOGNIZER_HPP_
#define PENGUIN_RECOGNIZER_HPP_

#define PENGUIN_VERSION

#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <opencv2/core/version.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "core.hpp"
#include "recognize.hpp"
#include "depot.hpp"
#include "result.hpp"

#ifdef PENGUIN_RECOGNIZER_RELEASE_VERSION
#define PENGUIN_RECOGNIZER_VERSION_STRING PENGUIN_RECOGNIZER_RELEASE_VERSION
#else
#define PENGUIN_RECOGNIZER_VERSION_STRING "v0.0.0"
#endif

static const std::string version = PENGUIN_RECOGNIZER_VERSION_STRING;
static const std::string opencv_version = CV_VERSION;

cv::Mat decode(std::string JSarrayBuffer)
{
    std::vector<uint8_t> buf(std::make_move_iterator(JSarrayBuffer.begin()),
                             std::make_move_iterator(JSarrayBuffer.end()));
    return cv::imdecode(buf, cv::IMREAD_COLOR);
}

void load_server(std::string server)
{
    penguin::server = server;
}

void load_stage_index() // local
{
    dict stage_index;
    std::ifstream f("../resources/json/stage_index.json");
    f >> stage_index;
    penguin::resource.add("stage_index", stage_index);
}

void wload_stage_index(std::string stage_index) // wasm
{
    auto& resource = penguin::resource;
    resource.add("stage_index", dict::parse(stage_index));
}

void load_hash_index() // local
{
    dict hash_index;
    std::ifstream f("../resources/json/hash_index.json");
    f >> hash_index;
    penguin::resource.add("hash_index", hash_index);
}

void wload_hash_index(std::string hash_index) // wasm
{
    auto& resource = penguin::resource;
    resource.add("hash_index", dict::parse(hash_index));
}

void load_templs() // local
{
    std::map<std::string, cv::Mat> item_templs;
    for (auto& templ : std::filesystem::directory_iterator(
             "../resources/icon/items"))
    {
        std::string itemId = templ.path().stem().string();
        cv::Mat templimg = cv::imread(templ.path().string());
        item_templs.insert_or_assign(itemId, templimg);
    }
    penguin::resource.add("item_templs", item_templs);
}

void wload_templs(std::string itemId, std::string JSarrayBuffer) // wasm
{
    auto& resource = penguin::resource;
    if (!resource.contains<std::map<std::string, cv::Mat>>("item_templs"))
    {
        resource.add("item_templs", std::map<std::string, cv::Mat>());
    }
    auto& item_templs =
        resource.get<std::map<std::string, cv::Mat>>("item_templs");
    item_templs.insert_or_assign(itemId, decode(std::move(JSarrayBuffer)));
}

const bool env_check()
{
    return penguin::env_check(); // to review
}

class Recognizer
{
public:
    Recognizer(std::string mode)
        : _mode(mode) {}

    dict recognize(cv::Mat img, bool detail = false)
    {
        _set_img(img);
        _recognize();
        return _get_report(detail);
    }
    std::string wrecognize(std::string JSarrayBuffer, bool detail, bool pretty_print)
    {
        _wset_img(JSarrayBuffer);
        _recognize();
        return _wget_report(detail, pretty_print);
    }

    cv::Mat get_debug_img()
    {
        cv::Mat img = _img.clone();
        _make_debug_img(img, _get_report(true));
        return img;
    }
#ifdef PENGUIN_RECOGNIZER_WASM_CPP_
#include <emscripten.h>
#include <emscripten/val.h>
    emscripten::val wget_debug_img()
    {
        cv::Mat img = get_debug_img();
        static std::vector<uint8_t> buf;
        cv::imencode(".png", img, buf);
        return emscripten::val(
            emscripten::typed_memory_view(buf.size(), buf.data()));
    }
#endif // PENGUIN_RECOGNIZER_WASM_CPP_

private:
    std::string _mode;
    cv::Mat _img;
    penguin::Result _result;
    penguin::Result_New _result_new;
    penguin::Depot _depot;
    dict _report;
    std::string _md5;
    double _decode_time = 0;
    double _recognize_time = 0;

    static cv::Rect _get_rect(dict arr)
    {
        return cv::Rect(arr[0], arr[1], arr[2], arr[3]);
    }

    void _set_img(cv::Mat img) // local
    {
        _img = img;
    }
    void _wset_img(std::string JSarrayBuffer) // wasm
    {
        int64 start = cv::getTickCount();
        _img = decode(std::move(JSarrayBuffer));
        int64 end = cv::getTickCount();
        _decode_time = (end - start) / cv::getTickFrequency() * 1000;
    }

    void _recognize()
    {
        if (_mode == "RESULT" && penguin::server != "CN")
        {
            int64 start = cv::getTickCount();
            _result.set_img(_img);
            _result.analyze();
            int64 end = cv::getTickCount();
            _md5 = _result.get_md5();
            _recognize_time = (end - start) / cv::getTickFrequency() * 1000;
        }
        else if (_mode == "RESULT" && penguin::server == "CN")
        {
            int64 start = cv::getTickCount();
            _result_new.set_img(_img);
            _result_new.analyze();
            int64 end = cv::getTickCount();
            _md5 = _result_new.get_md5();
            _recognize_time = (end - start) / cv::getTickFrequency() * 1000;
        }
    }

    dict _get_report(bool detail = false)
    {
        dict report;
        if (_mode == "RESULT" && penguin::server != "CN")
        {
            if (detail)
            {
                report.merge_patch(_result.report(true));
            }
            else
            {
                report.merge_patch(_result.report());
            }
            report["md5"] = _md5;
            if (const auto& decode_time = _decode_time; decode_time)
            {
                report["cost"]["decode"] = decode_time;
            }
            report["cost"]["recognize"] = _recognize_time;
        }
        else if (_mode == "RESULT" && penguin::server == "CN")
        {
            if (detail)
            {
                report.merge_patch(_result_new.report(true));
            }
            else
            {
                report.merge_patch(_result_new.report());
            }
            report["md5"] = _md5;
            if (const auto& decode_time = _decode_time; decode_time)
            {
                report["cost"]["decode"] = decode_time;
            }
            report["cost"]["recognize"] = _recognize_time;
        }
        return report;
    }
    std::string _wget_report(bool detail, bool pretty_print)
    {
        if (pretty_print)
        {
            return _get_report(detail).dump(4);
        }
        else
        {
            return _get_report(detail).dump();
        }
    }

    static void _make_debug_img(cv::Mat& img, const dict& report)
    {
        if (report.empty())
        {
            return;
        }
        if (report.contains("rect") && report["rect"] != "empty")
        {
            cv::Rect rect = _get_rect(report["rect"]);
            cv::Scalar color = penguin::Status2Color.at(report["status"]);
            cv::rectangle(img, rect, color, 2);
        }
        for (const auto& [key, subreport] : report.items())
        {
            if (subreport.is_array() || subreport.is_object())
            {
                if (key == "exceptions")
                {
                    continue;
                }
                _make_debug_img(img, subreport);
            }
        }
    }
};

#endif // PENGUIN_RECOGNIZER_HPP_