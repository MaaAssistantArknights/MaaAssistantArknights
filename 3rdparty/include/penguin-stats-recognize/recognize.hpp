#ifndef PENGUIN_RECOGNIZE_HPP_
#define PENGUIN_RECOGNIZE_HPP_

#include <limits>
#include <optional>
#include <queue>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "json.hpp"
#include "core.hpp"

using dict = nlohmann::ordered_json;

namespace penguin
{

enum class StatusFlags
{
    NORMAL = 0,
    HAS_WARNING = 1,
    HAS_ERROR = 2,
    WARNING = 3,
    ERROR = 4
};

enum ExcTypeFlags
{
    WARNING = 1,
    ERROR = 2
};

enum class ExcSubtypeFlags
{
    EXC_UNKNOWN = 0,
    EXC_FALSE = 1,
    EXC_NOTFOUND = 2,
    EXC_ILLEGAL = 3,
    EXC_LOWCONF = 4
};

enum class FontFlags
{
    UNDEFINED,
    NOVECENTO_WIDEBOLD,
    NOVECENTO_WIDEMEDIUM,
    SOURCE_HAN_SANS_CN_MEDIUM,
    NUBER_NEXT_DEMIBOLD_CONDENSED,
    RODIN_PRO_DB,
    SOURCE_HAN_SANS_KR_BOLD
};

enum FurniFlags
{
    FURNI_1 = 1,
    FURNI_2 = 2
};

enum WithoutExceptionFlags
{
    WITHOUT_EXCEPTION = 1
};

const std::map<FontFlags, std::string> Font2Str {
    {FontFlags::NOVECENTO_WIDEBOLD, "Novecento Wide Bold"},
    {FontFlags::NOVECENTO_WIDEMEDIUM, "Novecento Wide Medium"},
    {FontFlags::SOURCE_HAN_SANS_CN_MEDIUM, "Source Han Sans CN Medium"},
    {FontFlags::NUBER_NEXT_DEMIBOLD_CONDENSED, "Nuber Next Demibold Condensed"},
    {FontFlags::RODIN_PRO_DB, "Rodin Pro DB"},
    {FontFlags::SOURCE_HAN_SANS_KR_BOLD, "Source Han Sans KR Bold"}};

const std::map<std::string, FontFlags> Server2Font {
    {"CN", FontFlags::SOURCE_HAN_SANS_CN_MEDIUM},
    {"US", FontFlags::NUBER_NEXT_DEMIBOLD_CONDENSED},
    {"JP", FontFlags::RODIN_PRO_DB},
    {"KR", FontFlags::SOURCE_HAN_SANS_KR_BOLD}};

const std::map<StatusFlags, std::string> Status2Str {
    {StatusFlags::NORMAL, "normal"},
    {StatusFlags::HAS_WARNING, "hasWarning"},
    {StatusFlags::HAS_ERROR, "hasError"},
    {StatusFlags::WARNING, "warning"},
    {StatusFlags::ERROR, "error"}};

const std::map<std::string, cv::Scalar> Status2Color {
    {"normal", cv::Scalar(0, 153, 51)},
    {"hasWarning", cv::Scalar(51, 204, 153)},
    {"hasError", cv::Scalar(0, 204, 255)},
    {"warning", cv::Scalar(102, 153, 255)},
    {"error", cv::Scalar(0, 51, 204)}};

class Exception
{
public:
    std::string msg;
    const ExcTypeFlags type() const { return _exc_type; }
    const std::string where() const
    {
        std::string location;
        for (const auto& loc_ : _path)
        {
            location += (loc_ + ".");
        }
        if (!location.empty())
        {
            location.pop_back();
        }
        return location;
    }
    const dict detail() const { return _detail; }
    Exception() = delete;
    Exception(const ExcTypeFlags exc_type, const ExcSubtypeFlags exc_subtype, const dict& detail)
        : _exc_type(exc_type), _detail(detail)
    {
        switch (exc_subtype)
        {
        case ExcSubtypeFlags::EXC_UNKNOWN:
            msg = "Unknown";
            break;
        case ExcSubtypeFlags::EXC_FALSE:
            msg = "False";
            break;
        case ExcSubtypeFlags::EXC_NOTFOUND:
            msg = "NotFound";
            break;
        case ExcSubtypeFlags::EXC_ILLEGAL:
            msg = "Illegal";
            break;
        case ExcSubtypeFlags::EXC_LOWCONF:
            msg = "LowConfidence";
            break;
        }
    }
    void sign(const std::string& where) { _path.emplace_front(where); }

private:
    ExcTypeFlags _exc_type;
    std::list<std::string> _path;
    dict _detail;
};

class ItemTemplates
{
    struct Templ
    {
        std::string itemId;
        cv::Mat img;
        Templ(const std::string& itemId_, const cv::Mat& templimg_)
            : itemId(itemId_), img(templimg_) {}
        Templ(std::pair<const std::string, cv::Mat> templ)
            : itemId(templ.first), img(templ.second) {}
    };

public:
    const std::vector<Templ>& templ_list() const { return _templ_list; };
    ItemTemplates()
    {
        const auto& item_templs =
            resource.get<std::map<std::string, cv::Mat>>("item_templs");
        for (const auto& templ : item_templs)
        {
            _templ_list.emplace_back(templ);
        }
    }
    ItemTemplates(const std::string& stage_code, const std::string& difficulty = "NORMAL")
    {
        const auto& stage_drop =
            resource.get<dict>("stage_index")[stage_code][difficulty]["drops"];
        const auto& item_templs =
            resource.get<std::map<std::string, cv::Mat>>("item_templs");
        for (const auto& [_, itemId] : stage_drop.items())
        {
            auto it = item_templs.find((itemId));
            auto& templimg = item_templs.at(itemId);
            _templ_list.emplace_back(itemId, templimg);
        }
    }

private:
    std::vector<Templ> _templ_list;
};

class Widget
{
public:
    std::string widget_label;
    int x = 0;
    int y = 0;
    const int& width = _img.cols;
    const int& height = _img.rows;
    const cv::Mat img() const { return _img; }
    const StatusFlags status() const { return _status; }
    Widget() = default;
    Widget(const Widget& widget)
        : widget_label(widget.widget_label),
          x(widget.x),
          y(widget.y),
          _img(widget._img),
          _parent_widget(widget._parent_widget),
          _status(widget._status),
          _exception_list(widget._exception_list) {}
    Widget(Widget&& widget) noexcept
        : widget_label(std::move(widget.widget_label)),
          x(std::move(widget.x)),
          y(std::move(widget.y)),
          _img(std::move(widget._img)),
          _parent_widget(widget._parent_widget),
          _status(std::move(widget._status)),
          _exception_list(std::move(widget._exception_list)) {}
    Widget(const cv::Mat& img, const std::string& label)
    {
        widget_label = label;
        set_parent(nullptr);
        set_img(img);
    }
    Widget(const cv::Mat& img, const std::string& label, Widget* const parent_widget)
    {
        widget_label = label;
        set_parent(parent_widget);
        set_img(img);
    }
    Widget(const cv::Mat& img)
    {
        set_parent(nullptr);
        set_img(img);
    }
    Widget(const cv::Mat& img, Widget* const parent_widget)
    {
        set_parent(parent_widget);
        set_img(img);
    }
    Widget(const std::string& label)
    {
        widget_label = label;
        set_parent(nullptr);
    }
    Widget(const std::string& label, Widget* const parent_widget)
    {
        widget_label = label;
        set_parent(parent_widget);
    }
    Widget(Widget* const parent_widget) { set_parent(parent_widget); }
    virtual Widget& analyze() { return *this; }
    virtual void set_img(const cv::Mat& img)
    {
        _img = img;
        _carlibrate();
    }
    virtual void set_parent(Widget* const parent_widget)
    {
        _parent_widget = parent_widget;
        _carlibrate();
    }

    virtual const bool empty() const { return width <= 0 || height <= 0; }
    virtual const dict report(bool debug = false)
    {
        dict rpt = dict::object();
        if (_parent_widget == nullptr)
        {
            rpt["exceptions"] = _exception_list;
        }
        if (!debug)
        {
        }
        else
        {
            if (!_img.empty())
            {
                rpt["rect"] = {x, y, width, height};
            }
            else
            {
                rpt["rect"] = "empty";
            }
            rpt["status"] = Status2Str.at(_status);
        }
        return rpt;
    }
    void push_exception(Exception& exc)
    {
        auto status = exc.type();
        if (exc.where().empty())
        {
            if ((int)status + 2 > (int)_status)
            {
                _status = static_cast<StatusFlags>(status + 2);
            }
        }
        else
        {
            if ((int)status > (int)_status)
            {
                _status = static_cast<StatusFlags>(status);
            }
        }
        if (const auto& label = widget_label; !label.empty())
        {
            exc.sign(label);
        }

        std::string type;
        if (status == WARNING)
        {
            type = "WARNING";
        }
        else if (status == ERROR)
        {
            type = "ERROR";
        }
        _exception_list.push_back({{"type", type},
                                   {"where", exc.where()},
                                   {"what", exc.msg},
                                   {"detail", exc.detail()}});
        auto& parent = *_parent_widget;
        if (_parent_widget != nullptr)
        {
            parent.push_exception(exc);
        }
    }
    void push_exception(ExcTypeFlags type, ExcSubtypeFlags what, dict detail = dict::object())
    {
        _status = static_cast<StatusFlags>(type + 2);
        if (detail.empty())
        {
            detail = report(true);
        }
        Exception exc = {type, what, detail};
        push_exception(exc);
    }
    Widget& operator=(Widget& widget)
    {
        if (this != &widget)
        {
            widget_label = widget.widget_label;
            x = widget.x;
            y = widget.y;
            _img = widget._img;
            _status = widget._status;
            _exception_list = widget._exception_list;
        }
        return *this;
    }
    Widget& operator=(Widget&& widget) noexcept
    {
        if (this != &widget)
        {
            widget_label = std::move(widget.widget_label);
            x = std::move(widget.x);
            y = std::move(widget.y);
            _img = std::move(widget._img);
            _status = std::move(widget._status);
            _exception_list = std::move(widget._exception_list);
        }
        return *this;
    }

private:
    void _carlibrate()
    {
        x = y = 0;
        if (const auto& parent = *_parent_widget; _parent_widget != nullptr)
        {
            if (const auto& parent_img = parent.img();
                !parent_img.empty() && !_img.empty())
            {
                cv::Size _;
                cv::Point topleft_child, topleft_parent;
                _img.locateROI(_, topleft_child);
                parent_img.locateROI(_, topleft_parent);
                x = topleft_child.x - topleft_parent.x;
                y = topleft_child.y - topleft_parent.y;
                _relate(parent);
            }
        }
    }

protected:
    cv::Mat _img;
    Widget* _parent_widget = nullptr;
    StatusFlags _status = StatusFlags::NORMAL;
    dict _exception_list = dict::array();
    void _relate(const Widget& widget)
    {
        auto& self = *this;
        self.x += widget.x;
        self.y += widget.y;
    }
    void _relate(const cv::Point& topleft)
    {
        auto& self = *this;
        self.x += topleft.x;
        self.y += topleft.y;
    }
};

template <typename keyType, typename measurement>
class WidgetWithCandidate : public Widget
{
protected:
    struct Candidate
    {
        keyType key;
        measurement measure;
        Candidate(const keyType& key_, measurement measure_)
            : key(key_), measure(measure_) {}
    };

public:
    const int candidate_index() const { return _candidate_index; }
    const std::vector<Candidate>& candidates() const { return _candidates; }
    using Widget::Widget;
    virtual bool _next_candidate()
    {
        return _set_candidate(_candidate_index + 1);
    }
    virtual bool _set_candidate(int index)
    {
        if (index < _candidates.size())
        {
            _candidate_index = index;
            return true;
        }
        return false;
    }

protected:
    int _candidate_index = 0;
    std::vector<Candidate> _candidates;
    const keyType& _key() const
    {
        return _key(_candidate_index);
    }
    const keyType& _key(const int index) const
    {
        return _candidates[index].key;
    }
    const measurement _measure() const
    {
        return _measure(_candidate_index);
    }
    const measurement _measure(const int index) const
    {
        return _candidates[index].measure;
    }
    virtual void _get_candidates() = 0;
};

class Widget_Character : public WidgetWithCandidate<std::string, int>
{
public:
    const std::string& chr() const { return _key(); }
    const int dist() const { return _measure(); }
    const int candidate_index() const { return _candidate_index; }
    const std::vector<Candidate>& candidates() const { return _candidates; }
    Widget_Character() = default;
    Widget_Character(const cv::Mat& img_bin, FontFlags flag, const std::string& label, Widget* const parent_widget = nullptr)
        : WidgetWithCandidate(img_bin, label, parent_widget), font(flag) {}
    Widget_Character& analyze(const bool without_exception = false)
    {
        if (!_img.empty())
        {
            _get_candidates();
            if (dist() > 64 && without_exception == false)
            {
                push_exception(WARNING, ExcSubtypeFlags::EXC_LOWCONF);
            }
        }
        else
        {
            // add exception empty
        }
        return *this;
    }
    const dict report(bool debug = false)
    {
        dict rpt = dict::object();
        rpt.merge_patch(Widget::report(debug));
        if (!debug)
        {
            rpt["char"] = chr();
        }
        else
        {
            rpt["char"] = chr();
            rpt["hash"] = _hash;
            for (const auto& candidate : _candidates)
            {
                rpt["dist"][candidate.key] = candidate.measure;
            }
        }
        return rpt;
    }

private:
    std::string _hash;
    FontFlags font = FontFlags::UNDEFINED;
    void _get_candidates()
    {
        auto& self = *this;
        auto charrect = cv::boundingRect(_img);
        if (charrect.empty())
        {
            return;
        }
        _img = _img(charrect);
        self._relate(charrect.tl());
        auto charimg = _img;
        squarize(charimg);
        _hash = shash(charimg);
        std::string chr;
        dict char_dict;
        if (const auto& hash_index = resource.get<dict>("hash_index");
            font == FontFlags::NOVECENTO_WIDEBOLD)
        {
            char_dict = hash_index["stage"];
        }
        else if (font == FontFlags::NOVECENTO_WIDEMEDIUM)
        {
            char_dict = hash_index["stage_new"];
        }
        else
            char_dict = hash_index["item"][server];
        for (const auto& [kchar, vhash] : char_dict.items())
        {
            int dist = hamming(_hash, vhash, HammingFlags::HAMMING64);
            _candidates.emplace_back(kchar, dist);
        }

        std::sort(_candidates.begin(), _candidates.end(),
                  [](const Candidate& val1, const Candidate& val2) {
                      return val1.measure < val2.measure;
                  });
        _candidates = std::vector<Candidate>(_candidates.cbegin(), _candidates.cbegin() + 5);
    }
};

class Widget_ItemQuantity : public Widget
{
public:
    const int quantity() const { return _quantity; }
    Widget_ItemQuantity() = default;
    Widget_ItemQuantity(Widget* const parent_widget)
        : Widget("quantity", parent_widget) {}
    Widget_ItemQuantity(const int quantity,
                        Widget* const parent_widget = nullptr)
        : Widget("quantity", parent_widget), _quantity(quantity) {}
    Widget_ItemQuantity(const cv::Mat& img,
                        Widget* const parent_widget = nullptr)
        : Widget("quantity", parent_widget)
    {
        set_img(img);
    }
    Widget_ItemQuantity& analyze()
    {
        if (!_img.empty())
        {
            _get_quantity();
        }
        if (_quantity == 0)
        {
            push_exception(ERROR, ExcSubtypeFlags::EXC_NOTFOUND);
        }
        return *this;
    }
    void set_quantity(const int quantity) { _quantity = quantity; }
    void set_img(const cv::Mat& img)
    {
        Widget::set_img(img);
        if (_img.channels() == 3)
        {
            cv::cvtColor(_img, _img, cv::COLOR_BGR2GRAY);
            cv::threshold(_img, _img, 127, 255, cv::THRESH_BINARY);
        }
    }
    const bool empty() const { return _quantity; }
    const dict report(bool debug = false)
    {
        dict rpt = dict::object();
        rpt.merge_patch(Widget::report(debug));

        if (!debug)
        {
            rpt["quantity"] = _quantity;
        }
        else
        {
            rpt["quantity"] = _quantity;
            rpt["font"] = Font2Str.at(Server2Font.at(server));
            for (auto& chr : _characters)
            {
                rpt["chars"].push_back(chr.report(debug));
            }
        }
        return rpt;
    }

private:
    int _quantity = 0;
    std::vector<Widget_Character> _characters;
    void _get_quantity()
    {
        auto& self = *this;
        auto qtyimg = _img;
        std::string quantity_str;
        auto sp = separate(qtyimg, DirectionFlags::RIGHT);
        sp.erase(
            std::remove_if(sp.begin(), sp.end(),
                           [&](const cv::Range& range) {
                               int length = range.end - range.start;
                               return length > height * 0.7 || length < height * 0.1;
                           }),
            sp.end());
        for (auto it = sp.cbegin(); it != sp.end();)
        {
            const auto& range = *it;
            int length = range.end - range.start;
            bool quantity_empty = quantity_str.empty();
            auto charimg = qtyimg(cv::Rect(range.start, 0, length, height));

            if (!quantity_empty && it != sp.cbegin())
            {
                const auto& prev_range = *(std::prev(it));
                int dist = prev_range.start - range.end;
                if (dist > 0.5 * height)
                {
                    if (quantity_empty)
                    {
                        it = sp.erase(it);
                        continue;
                    }
                    else
                    {
                        it = sp.erase(it, sp.cend());
                        break;
                    }
                }
            }

            cv::Mat _;
            cv::Mat1i stats;
            cv::Mat1d centroids;
            int ccomps =
                cv::connectedComponentsWithStats(charimg, _, stats, centroids);
            if (ccomps - 1 != 1)
            {
                if (quantity_empty)
                {
                    it = sp.erase(it);
                    continue;
                }
                else
                {
                    it = sp.erase(it, sp.cend());
                    break;
                }
            }
            if (auto ccomp = cv::Rect(
                    stats(1, cv::CC_STAT_LEFT), stats(1, cv::CC_STAT_TOP),
                    stats(1, cv::CC_STAT_WIDTH), stats(1, cv::CC_STAT_HEIGHT));
                ccomp.width > ccomp.height ||
                ccomp.height / (double)height < _ITEM_CHR_HEIGHT_PROP)
            {
                if (quantity_empty)
                {
                    it = sp.erase(it);
                    continue;
                }
                else
                {
                    it = sp.erase(it, sp.cend());
                    break;
                }
            }
            double charimg_area = charimg.cols * charimg.rows;
            if (auto area_ratio = stats(1, cv::CC_STAT_AREA) / charimg_area;
                area_ratio < 0.15 || area_ratio > 0.75)
            {
                if (quantity_empty)
                {
                    it = sp.erase(it);
                    continue;
                }
                else
                {
                    it = sp.erase(it, sp.cend());
                    break;
                }
            }
            std::string label =
                "char.-" + std::to_string(_characters.size() + 1);
            auto chr =
                Widget_Character(charimg, Server2Font.at(server), label, this)
                    .analyze();
            _characters.emplace_back(chr);
            quantity_str.insert(0, chr.chr());
            ++it;
        }
        std::reverse(_characters.begin(), _characters.end());
        if (!quantity_str.empty())
        {
            _quantity = std::stoi(quantity_str);
            cv::Point topleft = cv::Point(sp.back().start, 0);
            cv::Point bottomright = cv::Point(sp.front().end, height - 1);
            _img = qtyimg(cv::Rect(topleft, bottomright));
            self._relate(topleft);
        }
        else
        {
            _img = cv::Mat();
        }
    }
};

class Widget_Item : public Widget
{
    struct ItemConfidence
    {
        std::string itemId;
        double confidence;
        ItemConfidence(const std::string& itemId_, double conf_)
            : itemId(itemId_), confidence(conf_) {}
    };

public:
    const std::string& itemId() const { return _itemId; }
    const int quantity() const { return _quantity.quantity(); }
    const double confidence() const { return _confidence; }
    const cv::Mat quantity_img() const { return _quantity.img(); }
    Widget_Item() = default;
    Widget_Item(FurniFlags furni, const std::string& label, Widget* const parent_widget = nullptr)
        : Widget(label, parent_widget),
          _itemId("furni"),
          _confidence(1),
          _quantity(Widget_ItemQuantity(furni)) {}
    Widget_Item(const cv::Mat& img, int diameter, const std::string& label, Widget* const parent_widget = nullptr)
        : Widget(img, label, parent_widget), _diameter(diameter) {}
    Widget_Item& analyze(const ItemTemplates& templs = ItemTemplates(),
                         const bool without_exception = false)
    {
        if (!_img.empty())
        {
            _get_item(templs);
            _get_quantity();
            if (_confidence < _CONFIDENCE_THRESHOLD &&
                without_exception == false)
            {
                push_exception(ERROR, ExcSubtypeFlags::EXC_LOWCONF);
            }
        }
        else
        {
            // add exception empty
        }

        return *this;
    }
    const dict report(bool debug = false)
    {
        dict rpt = dict::object();
        rpt.merge_patch(Widget::report(debug));
        if (!debug)
        {
            rpt["itemId"] = _itemId;
            rpt["quantity"] = _quantity.report()["quantity"];
        }
        else
        {
            rpt["itemId"] = _itemId;
            rpt["quantity"] = _quantity.report(debug);
            for (const auto& conf : _confidence_list)
            {
                rpt["confidence"][conf.itemId] = conf.confidence;
            }
        }
        return rpt;
    }

private:
    std::string _itemId;
    double _confidence = 0;
    Widget_ItemQuantity _quantity = {this};
    int _diameter = 0;
    std::vector<ItemConfidence> _confidence_list;
    void _get_item(const ItemTemplates& templs)
    {
        auto& self = *this;
        auto itemimg = _img;
        int coeff_multiinv = width / ITEM_RESIZED_WIDTH;
        double coeff = 1.0 / coeff_multiinv;
        if (coeff < 1)
        {
            resize(itemimg, itemimg, cv::Size(), coeff, coeff, cv::INTER_AREA);
        }
        std::map<std::string, cv::Point> _tmp_itemId2loc;
        for (const auto& templ : templs.templ_list())
        {
            const std::string& itemId = templ.itemId;
            cv::Mat templimg = templ.img;
            double fx = _diameter * coeff / TEMPLATE_DIAMETER;
            resize(templimg, templimg, cv::Size(), fx, fx, cv::INTER_AREA);
            cv::Mat mask =
                cv::Mat(templimg.cols, templimg.rows, CV_8UC1, cv::Scalar(0));
            cv::circle(mask, cv::Point(templimg.cols / 2, templimg.rows / 2),
                       int(0.9 * TEMPLATE_DIAMETER * fx / 2), cv::Scalar(255),
                       -1);
            cv::Mat resimg;
            cv::matchTemplate(itemimg, templimg, resimg, cv::TM_CCOEFF_NORMED,
                              mask);
            double minval, maxval;
            cv::Point minloc, maxloc;
            cv::minMaxLoc(resimg, &minval, &maxval, &minloc, &maxloc);
            _confidence_list.emplace_back(itemId, maxval);
            _tmp_itemId2loc[itemId] = maxloc;
        }
        std::sort(_confidence_list.begin(), _confidence_list.end(),
                  [](const ItemConfidence& val1, const ItemConfidence& val2) {
                      return val1.confidence > val2.confidence;
                  });
        std::string itemId = _confidence_list.front().itemId;
        cv::Point topleft_new = _tmp_itemId2loc[itemId];
        topleft_new.x *= coeff_multiinv;
        topleft_new.y *= coeff_multiinv;
        cv::Size size_new = cv::Size(
            static_cast<int>(round(TEMPLATE_WIDTH * ((double)_diameter / TEMPLATE_DIAMETER))),
            static_cast<int>(round(TEMPLATE_HEIGHT * ((double)_diameter / TEMPLATE_DIAMETER))));
        if (topleft_new.x + size_new.width > width)
        {
            size_new.width = width - topleft_new.x;
        }
        if (topleft_new.y + size_new.height > height)
        {
            size_new.height = height - topleft_new.y;
        }
        _img = _img(cv::Rect(topleft_new, size_new));
        self._relate(topleft_new);
        _confidence = _confidence_list.front().confidence;
        if (_confidence > _CONFIDENCE_THRESHOLD)
        {
            _itemId = itemId;
        }
    }
    void _get_quantity()
    {
        cv::Rect quantityrect =
            cv::Rect(0,
                     static_cast<int>(round(height * _ITEM_QTY_Y_PROP)),
                     static_cast<int>(round(width * _ITEM_QTY_WIDTH_PROP)),
                     static_cast<int>(round(height * _ITEM_QTY_HEIGHT_PROP)));
        cv::Mat quantityimg = _img(quantityrect);
        _quantity.set_img(quantityimg);
        _quantity.analyze();
    }
};
} // namespace penguin

#endif // PENGUIN_RECOGNIZE_HPP_
