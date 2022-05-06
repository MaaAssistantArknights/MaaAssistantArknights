#ifndef PENGUIN_CORE_HPP_
#define PENGUIN_CORE_HPP_

#include <any>
#include <list>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "json.hpp"

using dict = nlohmann::ordered_json;

namespace penguin
{
const int TEMPLATE_WIDTH = 183;
const int TEMPLATE_HEIGHT = 183;
const int TEMPLATE_DIAMETER = 163;
const int ITEM_RESIZED_WIDTH = 50;
const int HSV_DIST_MAX = 180;
const double _ITEM_QTY_Y_PROP = 0.71;
const double _ITEM_QTY_WIDTH_PROP = 0.83;
const double _ITEM_QTY_HEIGHT_PROP = 0.16;
const double _ITEM_CHR_HEIGHT_PROP = 0.6;
const double _CONFIDENCE_THRESHOLD = 0.6;

enum class DirectionFlags
{
    TOP = 0,
    BOTTOM = 1,
    LEFT = 2,
    RIGHT = 3
};

enum RangeFlags
{
    BEGIN = 0,
    END = 1
};

enum class ResizeFlags
{
    RESIZE_W16_H16 = 0,
    RESIZE_W32_H8 = 1,
    RESIZE_W8_H32 = 2
};

enum class HammingFlags
{
    HAMMING16 = 16,
    HAMMING64 = 64
};

static std::string server;

class Resource
{
public:
    static Resource& _get_instance()
    {
        static Resource instance;
        return instance;
    }
    void add(const std::string& key, const std::any& src)
    {
        if (!key.empty())
        {
            _resource.insert_or_assign(key, src);
        }
    }
    template <typename ResourceType>
    ResourceType& get(const std::string& key)
    {
        if (contains(key))
        { // use contains in C++20
            ResourceType& res = std::any_cast<ResourceType&>(_resource[key]);
            return res;
        }
        else
        {
            ResourceType& res = std::any_cast<ResourceType&>(_resource[""]);
            return res;
        }
    }
    const bool contains(const std::string& key) const
    {
        auto it = _resource.find(key);
        return it != _resource.cend();
    }
    template <typename ResourceType>
    const bool contains(const std::string& key)
    {
        if (contains(key))
        {
            return _resource[key].type() == typeid(ResourceType);
        }
        else
        {
            return false;
        }
    }

private:
    std::map<std::string, std::any> _resource;
    Resource() { _resource[""] = std::any(); };
    Resource(const Resource&) = delete;
    Resource(Resource&&) = delete;
    Resource& operator=(const Resource&) = delete;
    Resource& operator=(Resource&&) = delete;
};
auto& resource = Resource::_get_instance();

const bool env_check()
{
    bool pass = true;
    if (server.empty())
    {
        pass = false;
    }
#ifdef PENGUIN_RECOGNIZE_HPP_
    if (!resource.contains<std::map<std::string, cv::Mat>>("item_templs") ||
        !resource.contains<dict>("hash_index"))
    {
        pass = false;
    }
#endif
#ifdef PENGUIN_RESULT_HPP_
    if (!resource.contains<dict>("stage_index"))
    {
        pass = false;
    }
#endif
    return pass;
}

std::vector<cv::Range> separate(const cv::Mat& src_bin, DirectionFlags direc, int n = 0)
{
    std::vector<cv::Range> sp;
    bool isodd = false;
    int begin = 0;
    if (direc == DirectionFlags::TOP)
    {
        for (int ro = 0; ro < src_bin.rows; ro++)
        {
            uchar* pix = src_bin.data + ro * src_bin.step;
            int co = 0;
            for (; co < src_bin.cols; co++)
            {
                if ((bool)*pix && !isodd)
                {
                    begin = ro;
                    isodd = !isodd;
                    break;
                }
                else if ((bool)*pix && isodd)
                    break;
                pix++;
            }
            if (co == src_bin.cols && isodd)
            {
                int end = ro;
                sp.emplace_back(cv::Range(begin, end));
                isodd = !isodd;
                if (sp.size() == n)
                    break;
            }
        }
        if (isodd)
        {
            int end = src_bin.rows;
            sp.emplace_back(cv::Range(begin, end));
        }
    }
    else if (direc == DirectionFlags::BOTTOM)
    {
        for (int ro = src_bin.rows - 1; ro >= 0; ro--)
        {
            uchar* pix = src_bin.data + ro * src_bin.step;
            int co = 0;
            for (; co < src_bin.cols; co++)
            {
                if ((bool)*pix && !isodd)
                {
                    begin = ro + 1;
                    isodd = !isodd;
                    break;
                }
                else if ((bool)*pix && isodd)
                    break;
                pix++;
            }
            if (co == src_bin.cols && isodd)
            {
                int end = ro + 1;
                sp.emplace_back(cv::Range(end, begin));
                isodd = !isodd;
                if (sp.size() == n)
                    break;
            }
        }
        if (isodd)
        {
            int end = 0;
            sp.emplace_back(cv::Range(end, begin));
        }
    }
    else if (direc == DirectionFlags::LEFT)
    {
        for (int co = 0; co < src_bin.cols; co++)
        {
            uchar* pix = src_bin.data + co;
            int ro = 0;
            for (; ro < src_bin.rows; ro++)
            {
                if ((bool)*pix && !isodd)
                {
                    begin = co;
                    isodd = !isodd;
                    break;
                }
                else if ((bool)*pix && isodd)
                    break;
                pix = pix + src_bin.step;
            }
            if (ro == src_bin.rows && isodd)
            {
                int end = co;
                sp.emplace_back(cv::Range(begin, end));
                isodd = !isodd;
                if (sp.size() == n)
                    break;
            }
        }
        if (isodd)
        {
            int end = src_bin.cols;
            sp.emplace_back(cv::Range(begin, end));
        }
    }
    else if (direc == DirectionFlags::RIGHT)
    {
        for (int co = src_bin.cols - 1; co >= 0; co--)
        {
            uchar* pix = src_bin.data + co;
            int ro = 0;
            for (; ro < src_bin.rows; ro++)
            {
                if ((bool)*pix && !isodd)
                {
                    begin = co + 1;
                    isodd = !isodd;
                    break;
                }
                else if ((bool)*pix && isodd)
                    break;
                pix = pix + src_bin.step;
            }
            if (ro == src_bin.rows && isodd)
            {
                int end = co + 1;
                sp.emplace_back(cv::Range(end, begin));
                isodd = !isodd;
                if (sp.size() == n)
                    break;
            }
        }
        if (isodd)
        {
            int end = 0;
            sp.emplace_back(cv::Range(end, begin));
        }
    }
    return sp;
}

void squarize(cv::Mat& src_bin)
{
    const int h = src_bin.rows, w = src_bin.cols;
    if (h > w)
    {
        int d = h - w;
        hconcat(cv::Mat(h, d / 2, CV_8UC1, cv::Scalar(0)), src_bin, src_bin);
        hconcat(src_bin, cv::Mat(h, d - d / 2, CV_8UC1, cv::Scalar(0)),
                src_bin);
    }
    if (w > h)
    {
        int d = w - h;
        vconcat(cv::Mat(d / 2, w, CV_8UC1, cv::Scalar(0)), src_bin, src_bin);
        vconcat(src_bin, cv::Mat(d - d / 2, w, CV_8UC1, cv::Scalar(0)),
                src_bin);
    }
}

std::string shash(cv::Mat src_bin, ResizeFlags flag = ResizeFlags::RESIZE_W16_H16)
{
    cv::Size size_;
    switch (flag)
    {
    case ResizeFlags::RESIZE_W16_H16:
        size_ = cv::Size(16, 16);
        break;
    case ResizeFlags::RESIZE_W32_H8:
        size_ = cv::Size(32, 8);
        break;
    case ResizeFlags::RESIZE_W8_H32:
        size_ = cv::Size(8, 32);
        break;
    default:
        break;
    }

    cv::resize(src_bin, src_bin, size_);
    std::stringstream hash_value;
    uchar* pix = src_bin.data;
    int tmp_dec = 0;
    for (int ro = 0; ro < 256; ro++)
    {
        tmp_dec = tmp_dec << 1;
        if ((bool)*pix)
            tmp_dec++;
        if (ro % 4 == 3)
        {
            hash_value << std::hex << tmp_dec;
            tmp_dec = 0;
        }
        pix++;
    }
    return hash_value.str();
}

int hamming(std::string hash1, std::string hash2, HammingFlags flag = HammingFlags::HAMMING64)
{
    hash1.insert(hash1.begin(), int(flag) - hash1.size(), '0');
    hash2.insert(hash2.begin(), int(flag) - hash2.size(), '0');
    int dist = 0;
    for (int i = 0; i < int(flag); i = i + 16)
    {
        unsigned long long x = strtoull(hash1.substr(i, 16).c_str(), NULL, 16) ^
                               strtoull(hash2.substr(i, 16).c_str(), NULL, 16);
        while (x)
        {
            dist++;
            x = x & (x - 1);
        }
    }
    return dist;
}
} // namespace penguin

#endif // PENGUIN_CORE_HPP_
