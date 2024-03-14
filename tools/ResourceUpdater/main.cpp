#include <filesystem>
#include <fstream>
#include <unordered_set>

#include "Utils/Ranges.hpp"
#include "Utils/StringMisc.hpp"
#include "Utils/Time.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 5054)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wdeprecated-anon-enum-enum-conversion"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
#include <opencv2/opencv.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include <meojson/json.hpp>

inline static void ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}

// trim from end (in place)
inline static void rtrim(std::string& s)
{
    s.erase(
        std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
            .base(),
        s.end());
}

// trim from both ends (in place)
inline static void trim(std::string& s)
{
    ltrim(s);
    rtrim(s);
}

bool update_items_data(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output_dir,
    bool with_imgs = true);
bool cvt_single_item_template(
    const std::filesystem::path& input,
    const std::filesystem::path& output);
bool update_infrast_data(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output_dir);
bool update_stages_data(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output_dir);
bool update_roguelike_recruit(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output_dir,
    const std::filesystem::path& solution_dir);
bool update_levels_json(
    const std::filesystem::path& input_file,
    const std::filesystem::path& output_dir);
bool update_infrast_templates(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output_dir);
bool generate_english_roguelike_stage_name_replacement(
    const std::filesystem::path& ch_file,
    const std::filesystem::path& en_file);
bool update_battle_chars_info(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& overseas_dir,
    const std::filesystem::path& output_dir);
bool update_recruitment_data(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output,
    bool is_base);
bool check_roguelike_replace_for_overseas(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& tasks_path,
    const std::filesystem::path& base_dir,
    const std::filesystem::path& output_dir);
bool update_version_info(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output_dir);

int main([[maybe_unused]] int argc, char** argv)
{
    // ---- PATH DECLARATION ----

    const char* str_exec_path = argv[0];
    const auto cur_path = std::filesystem::path(str_exec_path).parent_path();

    auto solution_dir = cur_path;
    for (int i = 0; i != 10; ++i) {
        solution_dir = solution_dir.parent_path();
        if (std::filesystem::exists(solution_dir / "resource")) {
            break;
        }
    }
    std::cout << "Temp dir: " << cur_path.string() << std::endl;
    std::cout << "Working dir: " << solution_dir.string() << std::endl;

    const auto official_data_dir = cur_path / "Official";
    const auto overseas_data_dir = cur_path / "Overseas";
    const auto resource_dir = solution_dir / "resource";

    std::unordered_map<std::filesystem::path, std::string> global_dirs = {
        { "en_US", "YoStarEN" },
        { "ja_JP", "YoStarJP" },
        { "ko_KR", "YoStarKR" },
        { "zh_TW", "txwy" },
    };

    // ---- METHODS CALLS ----

    // Update levels.json from ArknightsGameResource
    std::cout << "------- Update levels.json for Official -------" << std::endl;
    if (!update_levels_json(
            official_data_dir / "levels.json",
            resource_dir / "Arknights-Tile-Pos")) {
        std::cerr << "update levels.json failed" << std::endl;
        return -1;
    }
    else {
        std::cout << "Done" << std::endl;
    }

    // 这个 en_levels.json 是自己手动生成放进去的
    // Will never work without en_levels.json in proj_dir, commented for now
    // generate_english_roguelike_stage_name_replacement(official_data_dir / "levels.json", cur_path
    // / "en_levels.json");

    // Update infrast data from ArknightsGameResource
    std::cout << "------- Update infrast data for Official -------" << std::endl;
    if (!update_infrast_data(official_data_dir / "gamedata" / "excel", resource_dir)) {
        std::cerr << "Update infrast data failed" << std::endl;
        return -1;
    }
    else {
        std::cout << "Done" << std::endl;
    }

    // Update infrast templates from ArknightsGameResource
    std::cout << "------- Update infrast templates for Official -------" << std::endl;
    if (!update_infrast_templates(
            official_data_dir / "building_skill",
            resource_dir / "template" / "infrast")) {
        std::cerr << "Update infrast templates failed" << std::endl;
        return -1;
    }
    else {
        std::cout << "Done" << std::endl;
    }

    // Update roguelike recruit data from ArknightsGameResource
    // std::cout << "------- Update roguelike recruit data -------" << std::endl;
    // if (!update_roguelike_recruit(arkbot_res_dir, resource_dir, solution_dir)) {
    //     std::cerr << "Update roguelike recruit data failed" << std::endl;
    //     return -1;
    // }    else {
    //    std::cout << "Done" << std::endl;
    //}

    // Update base_name.json from Penguin Stats
    std::cout << "------- Update stage.json for Official -------" << std::endl;
    if (!update_stages_data(cur_path, resource_dir)) {
        std::cerr << "Update stages data failed" << std::endl;
        return -1;
    }
    else {
        std::cout << "Done" << std::endl;
    }

    // Update battle chars info for all clients
    std::cout << "------- Update battle chars info for all clients -------" << std::endl;
    if (!update_battle_chars_info(
            official_data_dir / "gamedata" / "excel",
            overseas_data_dir,
            resource_dir)) {
        std::cerr << "Update battle chars info failed" << std::endl;
        return -1;
    }
    else {
        std::cout << "Done" << std::endl;
    }

    // Update recruitment data from ArknightsGameResource
    std::cout << "------- Update recruitment data for Official -------" << std::endl;
    if (!update_recruitment_data(
            official_data_dir / "gamedata" / "excel",
            resource_dir / "recruitment.json",
            true)) {
        std::cerr << "Update recruitment data failed" << std::endl;
        return -1;
    }
    else {
        std::cout << "Done" << std::endl;
    }

    // Update recruitment data from ArknightsGameData_YoStar
    for (const auto& [in, out] : global_dirs) {
        std::cout << "------- Update recruitment data for " << out << "------- " << std::endl;
        if (!update_recruitment_data(
                overseas_data_dir / in / "gamedata" / "excel",
                resource_dir / "global" / out / "resource" / "recruitment.json",
                false)) {
            std::cerr << "Update recruitment data failed" << std::endl;
            return -1;
        }
        else {
            std::cout << "Done" << std::endl;
        }
    }

    // Update items template and json from ArknightsGameResource
    std::cout << "------- Update items template and json for Official -------" << std::endl;
    if (!update_items_data(official_data_dir, resource_dir)) {
        std::cerr << "Update items data failed" << std::endl;
        return -1;
    }
    else {
        std::cout << "Done" << std::endl;
    }

    // Update items template and json from ArknightsGameData_YoStar
    for (const auto& [in, out] : global_dirs) {
        std::cout << "------- Update items template and json for " << out << "------- "
                  << std::endl;
        if (!update_items_data(
                overseas_data_dir / in / "gamedata" / "excel",
                resource_dir / "global" / out / "resource",
                false)) {
            std::cerr << "Update items json failed" << std::endl;
            return -1;
        }
        else {
            std::cout << "Done" << std::endl;
        }
    }

    // Update roguelike replace for overseas from ArknightsGameData_YoStar
    for (const auto& [in, out] : global_dirs) {
        std::cout << "------- Update roguelike replace for " << out << "------- " << std::endl;
        if (!check_roguelike_replace_for_overseas(
                overseas_data_dir / in / "gamedata" / "excel",
                resource_dir / "global" / out / "resource" / "tasks.json",
                official_data_dir / "gamedata" / "excel",
                cur_path / in)) {
            std::cerr << "Update roguelike replace for overseas failed" << std::endl;
            return -1;
        }
        else {
            std::cout << "Done" << std::endl;
        }
    }

    // Update version info from ArknightsGameData
    std::cout << "------- Update version info for Official -------" << std::endl;
    if (!update_version_info(official_data_dir / "gamedata" / "excel", resource_dir)) {
        std::cerr << "Update version info failed" << std::endl;
        return -1;
    }
    else {
        std::cout << "Done" << std::endl;
    }

    // Update global version info from ArknightsGameData_Yostar
    for (const auto& [in, out] : global_dirs) {
        std::cout << "------- Update version info for " << out << "------- " << std::endl;
        if (!update_version_info(
                overseas_data_dir / in / "gamedata" / "excel",
                resource_dir / "global" / out / "resource")) {
            std::cerr << "Update version info failed" << std::endl;
            return -1;
        }
        else {
            std::cout << "Done" << std::endl;
        }
    }

    std::cout << "------- All success -------" << std::endl;
    return 0;
}

// ---- METHODS DEFINITIONS ----

bool update_items_data(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output_dir,
    bool with_imgs)
{
    const auto input_json_path = with_imgs ? input_dir / "gamedata" / "excel" / "item_table.json"
                                           : input_dir / "item_table.json";

    auto parse_ret = json::open(input_json_path);
    if (!parse_ret) {
        std::cerr << "parse json failed" << std::endl;
        return false;
    }

    auto& input_json = parse_ret.value();
    json::value output_json;
    for (auto&& [item_id, item_info] : input_json["items"].as_object()) {
        static const std::vector<std::string> BlackListPrefix = {
            "LIMITED_TKT_GACHA_10", // 限定十连
            "p_char_",              // 角色信物（潜能）
            "class_p_char_",        // 角色中坚信物（潜能）
            "tier",                 // 职业潜能
            "voucher_",             // 干员晋升、皮肤自选券等
            "renamingCard",         // 改名卡
            "ap_item_",             // 干员发邮件送的东西
            "ap_supply_lt_100_202", // 干员发邮件送的理智（注意前半段id是理智小样，不能全过滤）
            "clue_",             // 火蓝之心活动的扭蛋什么的
            "2020recruitment10", // 周年自选券
            "2021recruitment10",
            "2022recruitment10",
            "2023recruitment10",
            "2024recruitment10",
            "2025recruitment10",
            "uni_set_",        // 家具组合包
            "medal_activity_", // 不知道是个啥
        };
        static const std::vector<std::string> BlackListSuffix = {
            "_rep_1", // 复刻活动的活动代币
        };

        bool is_blacklist = false;
        for (const auto& black : BlackListPrefix) {
            if (item_id.find(black) == 0) {
                is_blacklist = true;
                break;
            }
        }
        if (is_blacklist) {
            continue;
        }

        for (const auto& black : BlackListSuffix) {
            if (std::equal(black.rbegin(), black.rend(), item_id.rbegin())) {
                is_blacklist = true;
                break;
            }
        }
        if (is_blacklist) {
            continue;
        }

        auto input_icon_path = input_dir / "item" / (item_info["iconId"].as_string() + ".png");
        if (with_imgs && !std::filesystem::exists(input_icon_path)) {
            std::cout << input_icon_path.string() << " not exist" << std::endl;
            continue;
        }

        static const auto output_icon_path = output_dir / "template" / "items";
        std::string output_filename = item_id + ".png";
        if (with_imgs) {
            cvt_single_item_template(input_icon_path, output_icon_path / output_filename);
        }
        else if (!std::filesystem::exists(output_icon_path / output_filename)) {
            std::cout << (output_icon_path / output_filename).string() << " not exist" << std::endl;
            continue;
        }

        auto& output = output_json[item_id];
        output["name"] = item_info["name"];
        output["icon"] = output_filename;
        output["usage"] = item_info["usage"];
        output["description"] = item_info["description"];
        output["sortId"] = item_info["sortId"];
        output["classifyType"] = item_info["classifyType"];
    }
    auto output_json_path = output_dir / "item_index.json";
    std::ofstream ofs(output_json_path, std::ios::out);
    ofs << output_json.format();
    ofs.close();

    return true;
}

bool cvt_single_item_template(
    const std::filesystem::path& input,
    const std::filesystem::path& output)
{
    cv::Mat src = cv::imread(input.string(), -1);
    if (src.empty()) {
        std::cerr << input << " is empty" << std::endl;
        return false;
    }
    cv::Mat dst;
    cv::cvtColor(src, dst, cv::COLOR_BGRA2BGR);
    for (int c = 0; c != src.cols; ++c) {
        for (int r = 0; r != src.rows; ++r) {
            auto p = src.at<cv::Vec4b>(c, r);
            if (p[3] != 255) {
                dst.at<cv::Vec3b>(c, r) = cv::Vec3b(0, 0, 0);
            }
        }
    }
    cv::Mat mask;
    cv::cvtColor(dst, mask, cv::COLOR_BGR2GRAY);
    cv::inRange(mask, 1, 255, mask);
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, element);

    cv::Mat src_without_alpha;
    cv::cvtColor(src, src_without_alpha, cv::COLOR_BGRA2BGR);
    src_without_alpha.copyTo(dst, mask);

    cv::Mat dst_resized;
    const double scale = 720.0 / 1080.0 * 0.975;
    cv::resize(dst, dst_resized, cv::Size(), scale, scale, cv::INTER_AREA);

    cv::Mat dst_gray;
    cv::cvtColor(dst_resized, dst_gray, cv::COLOR_BGR2GRAY);
    dst_resized = dst_resized(cv::boundingRect(dst_gray));

    if (std::filesystem::exists(output)) {
        cv::Mat pre = cv::imread(output.string());
        if (pre.size() == dst_resized.size()) {
            cv::Mat matched;
            cv::matchTemplate(dst_resized, pre, matched, cv::TM_CCORR_NORMED);
            double max_val = 0, min_val = 0;
            cv::Point max_loc {}, min_loc {};
            cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc);

            if (max_val > 0.95) {
                std::cout << "Same item templ, skip: " << output.string() << ", score: " << max_val
                          << std::endl;
                return true;
            }
            else {
                std::cout << "Update item templ: " << output.string() << ", score: " << max_val
                          << std::endl;
            }
        }
        else {
            std::cout << "Update item templ: " << output.string() << " because sizes are different."
                      << std::endl;
        }
    }
    else {
        std::cout << "New item templ: " << output.string() << std::endl;
    }

    cv::imwrite(output.string(), dst_resized);
    return true;
}

void remove_xml(std::string& text)
{
    if (text.empty() || text.size() < 2) {
        return;
    }
    // find the first of '<'
    auto first_iter = text.end();
    for (auto it = text.begin(); it != text.end(); ++it) {
        if (*it == '<') {
            first_iter = it;
            break;
        }
    }
    if (first_iter == text.end()) {
        return;
    }
    bool in_xml = true;

    // move forward all non-xml elements
    auto end_r1_iter = text.end() - 1;
    auto next_iter = first_iter;
    while (++first_iter != end_r1_iter) {
        auto move_it = [&]() {
            *next_iter = *first_iter;
            ++next_iter;
        };

        if (*first_iter != '>' && in_xml) {
            ;
        }
        else if (*first_iter == '>' && in_xml) {
            in_xml = false;
        }
        else if (*first_iter == '<' && !in_xml) {
            in_xml = true;
        }
        else {
            move_it();
        }
    }
    *next_iter = *end_r1_iter;
    if (*next_iter != '>') {
        ++next_iter;
    }
    text.erase(next_iter, text.end());
}

bool update_infrast_data(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output_dir)
{
    const auto input_file = input_dir / "building_data.json";
    const auto output_file = output_dir / "infrast.json";

    json::value input_json;
    {
        auto opt = json::open(input_file);
        if (!opt) {
            std::cerr << input_file << " parse error" << std::endl;
            return false;
        }
        input_json = std::move(opt.value());
    }

    json::value old_json;
    {
        auto opt = json::open(output_file);
        if (!opt) {
            std::cerr << output_file << " parse error" << std::endl;
            return false;
        }
        old_json = std::move(opt.value());
    }

    auto buffs_opt = input_json.find<json::object>("buffs");
    if (!buffs_opt) {
        return false;
    }
    auto& buffs = buffs_opt.value();

    // 这里面有些是手动修改的，要保留
    json::value& root = old_json;
    std::unordered_set<std::string> rooms;
    for (auto& buff_obj : buffs | asst::views::values) {
        std::string raw_room_type = static_cast<std::string>(buff_obj["roomType"]);

        // 为了兼容老版本的字段 orz
        static const std::unordered_map<std::string, std::string> RoomTypeMapping = {
            { "POWER", "Power" },       { "CONTROL", "Control" }, { "DORMITORY", "Dorm" },
            { "WORKSHOP", "" },         { "MANUFACTURE", "Mfg" }, { "TRADING", "Trade" },
            { "MEETING", "Reception" }, { "HIRE", "Office" },     { "TRAINING", "" },
        };

        std::string room_type = RoomTypeMapping.at(raw_room_type);
        if (room_type.empty()) {
            continue;
        }

        rooms.emplace(room_type);

        std::string raw_key = static_cast<std::string>(buff_obj["skillIcon"]);
        std::string name = static_cast<std::string>(buff_obj["buffName"]);
        // 这玩意里面有类似 xml 的东西，全删一下
        std::string desc = static_cast<std::string>(buff_obj["description"]);
        remove_xml(desc);

        std::string json_key = raw_key;
        // https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/5123#issuecomment-1589425675
        if (json_key == "bskill_man_exp4") {
            json_key = "bskill_man_exp0";
        }
        auto& skill = root[room_type]["skills"][json_key];
        auto& name_arr = skill["name"].as_array();
        bool new_name = true;
        for (auto& name_obj : name_arr) {
            if (name_obj.as_string() == name) {
                new_name = false;
                break;
            }
        }
        if (new_name) {
            skill["name"].emplace(name);
            skill["desc"].emplace(desc);
        }

        // 历史遗留问题，以前的图片是从wiki上爬的，都是大写开头
        // Windows下不区分大小写，现在新的小写文件名图片没法覆盖
        // 所以干脆全用大写开头算了
        std::string filename = raw_key + ".png";
        filename[0] -= 32;
        skill["template"] = std::move(filename);
    }
    root["roomType"] = json::array(rooms);

    std::ofstream ofs(output_file, std::ios::out);
    ofs << root.format();
    ofs.close();

    return true;
}

bool update_stages_data(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output_dir)
{
    // 国内访问可以改成 .cn 的域名
    const std::string PenguinAPI = R"(https://penguin-stats.io/PenguinStats/api/v2/stages?server=)";
    const std::vector<std::string> PenguinServers = { "CN", "US", "JP", "KR" };

    const std::filesystem::path TempFile = input_dir / "stages.json";

    struct DropInfo
    {
        std::string item_id;
        std::string drop_type;
        bool operator<(const DropInfo& rhs) const
        {
            return item_id + drop_type < rhs.item_id + drop_type;
        }
        bool operator==(const DropInfo& rhs) const
        {
            return item_id == rhs.item_id && drop_type == rhs.drop_type;
        }
    };

    std::map<std::string, std::set<DropInfo>> drop_infos;
    std::map<std::string, json::value> stage_basic_infos;
    for (const auto& server : PenguinServers) {
        int stage_request_ret =
            system(("curl -o \"" + TempFile.string() + "\" " + PenguinAPI + server).c_str());
        if (stage_request_ret != 0) {
            std::cerr << "Request Penguin Stats failed" << std::endl;
            return false;
        }

        auto parse_ret = json::open(TempFile);
        if (!parse_ret) {
            std::cerr << "parse stages.json failed" << std::endl;
            return false;
        }
        auto& stage_json = parse_ret.value();

        for (auto& stage : stage_json.as_array()) {
            if (!stage.contains("dropInfos")) {
                continue;
            }
            std::string stage_id = stage.at("stageId").as_string();
            for (auto& drop_json : stage["dropInfos"].as_array()) {
                DropInfo drop;
                drop.item_id = drop_json.get("itemId", std::string());
                if (drop.item_id.empty()) {
                    continue;
                }

                drop.drop_type = drop_json.at("dropType").as_string();
                drop_infos[stage_id].emplace(std::move(drop));
            }

            json::value basic_json;
            basic_json["apCost"] = stage["apCost"];
            basic_json["code"] = stage["code"];
            basic_json["stageId"] = stage["stageId"];
            stage_basic_infos.emplace(stage_id, std::move(basic_json));
        }
    }

    json::array result;
    for (const auto& [stage_id, drops] : drop_infos) {
        json::array drops_json;
        for (const auto& d : drops) {
            drops_json.emplace_back(json::object {
                { "itemId", d.item_id },
                { "dropType", d.drop_type },
            });
        }
        auto& stage = stage_basic_infos.at(stage_id);
        stage["dropInfos"] = std::move(drops_json);
        result.emplace_back(std::move(stage));
    }

    std::ofstream ofs(output_dir / "stages.json", std::ios::out);
    ofs << result.format();
    ofs.close();

    return true;
}

bool update_infrast_templates(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output_dir)
{
    for (auto&& entry : std::filesystem::directory_iterator(input_dir)) {
        if (entry.path().extension() != ".png") {
            continue;
        }
        const std::string& stem = entry.path().stem().string();

        const std::vector<std::string> BlackList = { "[style]",
                                                     "bskill_dorm",
                                                     "bskill_train",
                                                     "bskill_ws" };

        bool is_blacklist = false;
        for (const auto& bl : BlackList) {
            if (stem.find(bl) != std::string::npos) {
                is_blacklist = true;
                break;
            }
        }
        if (is_blacklist) {
            continue;
        }

        cv::Mat image = cv::imread(entry.path().string(), -1);
        cv::Mat dst;
        cv::cvtColor(image, dst, cv::COLOR_BGRA2BGR);
        for (int c = 0; c != image.cols; ++c) {
            for (int r = 0; r != image.rows; ++r) {
                auto p = image.at<cv::Vec4b>(c, r);
                if (p[3] != 255) {
                    dst.at<cv::Vec3b>(c, r) = cv::Vec3b(0, 0, 0);
                }
            }
        }
        std::string filename = entry.path().filename().string();
        // 历史遗留问题，以前的图片是从wiki上爬的，都是大写开头
        // Windows下不区分大小写，现在新的小写文件名图片没法覆盖
        // 所以干脆全用大写开头算了
        filename[0] -= 32;
        std::string out_file = (output_dir / filename).string();

        if (std::filesystem::exists(out_file)) {
            cv::Mat pre = cv::imread(out_file);
            if (pre.size() == dst.size()) {
                cv::Mat matched;
                cv::matchTemplate(dst, pre, matched, cv::TM_CCORR_NORMED);
                double max_val = 0, min_val = 0;
                cv::Point max_loc {}, min_loc {};
                cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc);

                if (max_val > 0.95) {
                    std::cout << "Same infrast templ, skip: " << out_file << ", score: " << max_val
                              << std::endl;
                    continue;
                }
                else {
                    std::cout << "Update infrast templ: " << out_file << ", score: " << max_val
                              << std::endl;
                }
            }
            else {
                std::cout << "Update infrast templ: " << out_file << " because sizes are different."
                          << std::endl;
            }
        }
        else {
            std::cout << "New infrast templ: " << out_file << std::endl;
        }
        cv::imwrite(out_file, dst);
    }
    return true;
}

bool update_roguelike_recruit(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output_dir,
    const std::filesystem::path& solution_dir)
{
    std::string python_cmd;
    std::filesystem::path python_file =
        solution_dir / "tools" / "RoguelikeResourceUpdater" / "generate_roguelike_recruit.py";
    python_cmd = "python " + python_file.string() + " --input=\"" + input_dir.string()
                 + "\" --output=\"" + output_dir.string() + "\"";
    int python_ret = system(python_cmd.c_str());
    if (python_ret != 0) {
        return false;
    }
    return true;
}

bool update_levels_json(
    const std::filesystem::path& input_file,
    const std::filesystem::path& output_dir)
{
    auto json_opt = json::open(input_file);
    if (!json_opt) {
        std::cerr << input_file << " parse failed" << std::endl;
        return false;
    }
    auto& root = json_opt.value();

    auto overview_path = output_dir / "overview.json";
    json::value overview = json::open(overview_path).value_or(json::value());

    for (auto& stage_info : root.as_array()) {
        std::string stem =
            stage_info["stageId"].as_string() + "-" + stage_info["levelId"].as_string();
        std::string filename = stem + ".json";
        asst::utils::string_replace_all_in_place(filename, "/", "-");
        auto filepath = output_dir / filename;
        // 仓库里之前已有的，一般都是手动的patch，以原来的为准
        if (auto pre_stage_opt = json::open(filepath)) {
            stage_info = std::move(*pre_stage_opt);
        }
        else {
            std::ofstream ofs(filepath, std::ios::out);
            ofs << stage_info.format();
            ofs.close();
        }

        auto& stage_obj = stage_info.as_object();
        stage_obj.erase("tiles");
        stage_obj.erase("view");
        stage_obj["filename"] = filename;
        overview[std::move(stem)] = std::move(stage_obj);
    }

    std::ofstream ofs(overview_path, std::ios::out);
    ofs << overview.format();
    ofs.close();

    return true;
}

bool generate_english_roguelike_stage_name_replacement(
    const std::filesystem::path& ch_file,
    const std::filesystem::path& en_file)
{
    auto ch_opt = json::open(ch_file);
    auto en_opt = json::open(en_file);

    if (!ch_opt || !en_opt) {
        return false;
    }

    auto& ch_json = ch_opt.value();
    auto& en_json = en_opt.value();

    std::unordered_map<std::string, std::string> ch_levelid_name;
    for (auto& stage_obj : ch_json.as_array()) {
        // 肉鸽关卡全叫这个
        if (stage_obj["code"].as_string() != "ISW-NO") {
            continue;
        }
        ch_levelid_name[stage_obj["levelId"].as_string()] = stage_obj["name"].as_string();
    }

    json::array en_to_ch_vec;
    for (auto& stage_obj : en_json.as_array()) {
        // 肉鸽关卡全叫这个
        if (stage_obj["code"].as_string() != "ISW-NO") {
            continue;
        }
        std::string level_id = stage_obj["levelId"].as_string();
        auto it = ch_levelid_name.find(level_id);
        if (it == ch_levelid_name.cend()) {
            std::cerr << "Unknown en stage id: " << level_id << std::endl;
        }
        json::array arr;
        arr.emplace_back(stage_obj["name"].as_string());
        arr.emplace_back(it->second);
        en_to_ch_vec.emplace_back(std::move(arr));
    }
    std::ofstream ofs(en_file.parent_path() / "en_replace.json", std::ios::out);
    ofs << en_to_ch_vec.format();
    ofs.close();

    return true;
}

bool update_battle_chars_info(
    const std::filesystem::path& official_dir,
    const std::filesystem::path& overseas_dir,
    const std::filesystem::path& output_dir)
{
    std::string to_char_json = "gamedata/excel/character_table.json";

    auto range_opt = json::open(official_dir / "range_table.json");
    auto chars_cn_opt = json::open(official_dir / "character_table.json");
    auto chars_en_opt = json::open(overseas_dir / "en_US" / to_char_json);
    auto chars_jp_opt = json::open(overseas_dir / "ja_JP" / to_char_json);
    auto chars_kr_opt = json::open(overseas_dir / "ko_KR" / to_char_json);
    auto chars_tw_opt = json::open(overseas_dir / "zh_TW" / to_char_json);

    if (!chars_cn_opt || !chars_en_opt || !chars_jp_opt || !chars_kr_opt || !chars_tw_opt
        || !range_opt) {
        return false;
    }

    auto& range_json = range_opt.value();

    std::vector<std::pair<json::value, std::string>> chars_json = {
        { chars_cn_opt.value(), "name" },
        { chars_en_opt.value(), "name_en" },
        { chars_jp_opt.value(), "name_jp" },
        { chars_kr_opt.value(), "name_kr" },
        { chars_tw_opt.value(), "name_tw" }
    };

    json::value result;
    auto& range = result["ranges"].as_object();
    for (auto& [id, range_data] : range_json.as_object()) {
        if (int direction = range_data["direction"].as_integer(); direction != 1) {
            // 现在都是 1，朝右的，以后不知道会不会改，加个warning，真遇到再说
            std::cerr << "!!!Warning!!! range_id: " << id << " 's direction is "
                      << std::to_string(direction) << std::endl;
        }
        json::array points;
        for (auto& grids : range_data["grids"].as_array()) {
            int x = grids["col"].as_integer();
            int y = grids["row"].as_integer();
            points.emplace_back(json::array { x, y });
        }
        range.emplace(id, std::move(points));
    }

    auto& chars = result["chars"];
    std::map<std::string, std::vector<std::string>> tokens;
    for (auto& [id, char_data] : chars_json[0].first.as_object()) {
        json::value char_new_data;

        for (auto& [data, name] : chars_json) {
            char_new_data[name] = data.get(id, "name", char_data["name"].as_string());
        }

        char_new_data["profession"] = char_data["profession"];
        const std::string& default_range = char_data.get("phases", 0, "rangeId", "0-1");
        char_new_data["rangeId"] = json::array {
            default_range,
            char_data.get("phases", 1, "rangeId", default_range),
            char_data.get("phases", 2, "rangeId", default_range),
        };
        char_new_data["rarity"] = static_cast<int>(char_data["rarity"]) + 1;
        char_new_data["position"] = char_data["position"];

        if (auto token_opt = char_data.find<std::string>("tokenKey")) {
            tokens[id].emplace_back(*token_opt);
        }
        if (auto skill_opt = char_data.find<json::array>("skills")) {
            for (const auto& skill_obj : *skill_opt) {
                if (auto token_opt = skill_obj.find<std::string>("overrideTokenKey")) {
                    tokens[id].emplace_back(*token_opt);
                }
            }
        }
        chars.emplace(id, std::move(char_new_data));
    }
    for (const auto& [oper_id, token_id_list] : tokens) {
        std::vector<std::string> token_names_list;
        for (const auto& token_id : token_id_list) {
            std::string token_name = chars.get(token_id, "name", std::string());
            if (!token_name.empty()) {
                token_names_list.emplace_back(token_name);
            }
        }
        chars[oper_id]["tokens"] = json::array(token_names_list);
    }

    json::value Amiya_data;
    Amiya_data["name"] = "阿米娅-WARRIOR";
    Amiya_data["name_en"] = "Amiya-WARRIOR";
    Amiya_data["name_jp"] = "アーミヤ-WARRIOR";
    Amiya_data["name_kr"] = "아미야-WARRIOR";
    Amiya_data["name_tw"] = "阿米婭-WARRIOR";
    Amiya_data["profession"] = "WARRIOR";
    Amiya_data["rangeId"] = json::array { "1-1", "1-1", "1-1" };
    Amiya_data["rarity"] = 5;
    Amiya_data["position"] = "MELEE";
    chars.emplace("char_1001_amiya2", std::move(Amiya_data));

    const auto& out_file = output_dir / "battle_data.json";
    std::ofstream ofs(out_file, std::ios::out);
    ofs << result.format() << std::endl;
    ofs.close();

    return true;
}

bool update_recruitment_data(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output,
    bool is_base)
{
    using asst::ranges::find_if, asst::ranges::range;
    using asst::utils::string_replace_all_in_place;
    using asst::views::filter, asst::views::split, asst::views::transform, asst::views::drop_while;

    auto not_empty = []<range Rng>(Rng str) -> bool {
        return !str.empty();
    };
    auto make_string_view = []<range Rng>(Rng str) -> std::string_view {
        return asst::utils::make_string_view(str);
    };

    auto recruitment_opt = json::open(input_dir / "gacha_table.json");
    auto operators_opt = json::open(input_dir / "character_table.json");

    if (!recruitment_opt || !operators_opt) {
        std::cerr << "Failed to parse recruitment or operators file" << std::endl;
        return false;
    }

    std::vector<std::string> chars_list;
    std::string recruitment_details = recruitment_opt->at("recruitDetail").as_string();
    remove_xml(recruitment_details);
    string_replace_all_in_place(recruitment_details, "\\n", "");
    constexpr std::string_view star_delim = "★";

    auto items =
        // 按照 ★ 分割
        recruitment_details | split(star_delim) | filter(not_empty) | transform(make_string_view) |
        // 忽略 Lancet-2 之前的东西
        drop_while(
            [&](std::string_view str) { return str.find("Lancet-2") == std::string_view::npos; })
        |
        // 按照 \n 分割，若非空则取第一个元素
        transform([&](auto str) { return str | split('\n') | filter(not_empty); })
        | filter(not_empty) | transform([&](auto strs) { return make_string_view(strs.front()); });

    for (std::string_view s : items) {
        for (std::string_view n :
             s | split('/') | filter(not_empty) | transform(make_string_view)) {
            std::string name(n);
            string_replace_all_in_place(
                name,
                "　",
                " "); // "　" is full-width space, replacing with common " "
            trim(name);
            if (name == "Justice Knight") {
                name = "'Justice Knight'";
            }
            chars_list.emplace_back(name);
        }
    }

    struct RecruitmentInfo
    {
        int rarity = 0;
        std::vector<std::string> tags;
    };

    static std::unordered_map</*id*/ std::string, RecruitmentInfo> base_chars_info;

    std::unordered_map</*name*/ std::string, /*id*/ std::string> chars_id_list;

    for (auto& [id, char_data] : operators_opt->as_object()) {
        if (is_base) {
            RecruitmentInfo info;
            info.rarity = char_data["rarity"].as_integer() + 1;
            for (const auto& tag : char_data["tagList"].as_array()) {
                info.tags.emplace_back(tag.as_string());
            }
            std::string position = char_data["position"].as_string();
            if (position == "MELEE") {
                info.tags.emplace_back("近战位");
            }
            else if (position == "RANGED") {
                info.tags.emplace_back("远程位");
            }
            else {
                continue;
            }
            if (info.rarity == 1) {
                // 2023/01/17, yj 又把支援机械加上了，我们就不额外添加了
                // info.tags.emplace_back("支援机械");
            }
            else if (info.rarity == 5) {
                info.tags.emplace_back("资深干员");
            }
            else if (info.rarity == 6) {
                info.tags.emplace_back("高级资深干员");
            }

            static const std::unordered_map<std::string, std::string> RoleMap = {
                { "CASTER", "术师干员" }, { "MEDIC", "医疗干员" },   { "PIONEER", "先锋干员" },
                { "SNIPER", "狙击干员" }, { "SPECIAL", "特种干员" }, { "SUPPORT", "辅助干员" },
                { "TANK", "重装干员" },   { "WARRIOR", "近卫干员" },
            };
            auto role_iter = RoleMap.find(char_data["profession"].as_string());
            if (role_iter == RoleMap.cend()) {
                continue;
            }
            info.tags.emplace_back(role_iter->second);
            base_chars_info.insert_or_assign(id, std::move(info));
        }
        chars_id_list.insert_or_assign(char_data["name"].as_string(), id);
    }

    json::value result;
    auto& opers = result["operators"];
    for (const std::string& name : chars_list) {
        auto id_iter = chars_id_list.find(name);
        if (id_iter == chars_id_list.cend()) {
            std::cerr << "Failed to find char: " << name << std::endl;
            return false;
        }

        const std::string& id = id_iter->second;
        auto info_iter = base_chars_info.find(id);
        if (info_iter == base_chars_info.cend()) {
            std::cerr << "Failed to find char's info: " << id << name << std::endl;
            return false;
        }

        opers.emplace(json::object { { "id", id },
                                     { "name", name },
                                     { "rarity", info_iter->second.rarity },
                                     { "tags", json::array(info_iter->second.tags) } });
    }

    static std::unordered_map</*id*/ int, /*tag*/ std::string> base_tags_name;
    std::unordered_map</*id*/ int, /*tag*/ std::string> tags_name;

    for (const auto& tag_json : recruitment_opt->at("gachaTags").as_array()) {
        int id = tag_json.at("tagId").as_integer();
        std::string name = tag_json.at("tagName").as_string();
        if (is_base) {
            base_tags_name.insert_or_assign(id, name);
        }
        tags_name.insert_or_assign(id, name);
    }

    auto& tags = result["tags"];
    for (const auto& [id, tag] : tags_name) {
        std::string base_name = base_tags_name.at(id);
        tags.emplace(base_name, tag);
    }

    std::ofstream ofs(output, std::ios::out);
    ofs << result.format() << std::endl;
    ofs.close();

    return true;
}

bool check_roguelike_replace_for_overseas(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& tasks_path,
    const std::filesystem::path& base_dir,
    const std::filesystem::path& output_dir)
{
    static std::unordered_map</*id*/ std::string, /*base_name*/ std::string> base_stage_names;
    static std::unordered_map</*id*/ std::string, /*base_name*/ std::string> base_item_names;
    static std::unordered_map</*id*/ std::string, /*base_name*/ std::string> base_totem_names;
    static std::unordered_map</*id*/ std::string, /*base_name*/ std::string> base_encounter_names;

    if (base_stage_names.empty() || base_item_names.empty() || base_totem_names.empty()
        || base_encounter_names.empty()) {
        auto rg_opt = json::open(base_dir / "roguelike_topic_table.json");
        if (!rg_opt) {
            std::cerr << "Failed to open roguelike_topic_table for" << base_dir << std::endl;
            return false;
        }
        auto& rg_json = rg_opt.value();
        for (auto& [rogue_index, rogue_details] : rg_json["details"].as_object()) {
            for (auto&& [id, stage_obj] : rogue_details["stages"].as_object()) {
                if (!id.starts_with("ro1_e_") && !id.starts_with("ro2_e_")
                    && !id.starts_with("ro3_e_")) {
                    base_stage_names.emplace(id, stage_obj["name"].as_string());
                }
            }
            for (auto&& [id, item_obj] : rogue_details["items"].as_object()) {
                // limits only buyable items
                // (08/03/2024 items 516 extracted items vs 514 shopping.json items)
                if (!id.starts_with("rogue_1_relic_c") && !id.starts_with("rogue_1_relic_m")) {
                    if (id.starts_with(rogue_index + "_recruit")
                        || id.starts_with(rogue_index + "_upgrade")
                        || id.starts_with(rogue_index + "_relic")
                        || id.starts_with(rogue_index + "_active") || id.ends_with("_item")) {
                        base_item_names.emplace(id, item_obj["name"].as_string());
                    }
                    if (id.starts_with(rogue_index + "_totem")) {
                        base_totem_names.emplace(id, item_obj["name"].as_string());
                    }
                }
            }
            for (auto&& [id, encounter_obj] : rogue_details["choiceScenes"].as_object()) {
                // very complicated way to reduce dupes. Will probably brake sooner or later.
                if (id.ends_with("_enter")) {
                    if (!id.starts_with("scene_ro3_rest")) {
                        if (!id.starts_with("scene_ro3_portal")
                            || id.starts_with("scene_ro3_portalsample")) {
                            base_encounter_names.emplace(id, encounter_obj["title"].as_string());
                        }
                    }
                }
            }
        }
    }

    static std::unordered_map</*id*/ std::string, /*base_name*/ std::string> base_char_names;
    if (base_char_names.empty()) {
        auto char_opt = json::open(base_dir / "character_table.json");
        if (!char_opt.has_value()) {
            std::cerr << "Failed to open character_table for" << base_dir << std::endl;
            return false;
        }

        auto& char_json = char_opt.value();
        for (auto&& [id, char_obj] : char_json.as_object()) {
            base_char_names.emplace(id, char_obj["name"].as_string());
        }
    }

    auto rg_opt = json::open(input_dir / "roguelike_topic_table.json");
    if (!rg_opt) {
        std::cerr << "Failed to open roguelike_topic_table for " << input_dir << std::endl;
        return false;
    }

    std::unordered_map</*id*/ std::string, /*name*/ std::string> stage_names;
    std::unordered_map</*id*/ std::string, /*name*/ std::string> item_names;
    std::unordered_map</*id*/ std::string, /*name*/ std::string> totem_names;
    std::unordered_map</*id*/ std::string, /*name*/ std::string> encounter_names;

    auto& rg_json = rg_opt.value();
    std::string encounter_nospace;
    for (auto& [rogue_index, rogue_details] : rg_json["details"].as_object()) {
        for (auto&& [id, stage_obj] : rogue_details["stages"].as_object()) {
            if (!id.starts_with("ro1_e_") && !id.starts_with("ro2_e_")
                && !id.starts_with("ro3_e_")) {
                stage_names.emplace(id, stage_obj["name"].as_string());
            }
        }
        for (auto&& [id, item_obj] : rogue_details["items"].as_object()) {
            // limits only buyable items
            // (08/03/2024 items 516 extracted items vs 514 shopping.json items)
            if (!id.starts_with("rogue_1_relic_c") && !id.starts_with("rogue_1_relic_m")) {
                if (id.starts_with(rogue_index + "_recruit")
                    || id.starts_with(rogue_index + "_upgrade")
                    || id.starts_with(rogue_index + "_relic")
                    || id.starts_with(rogue_index + "_active") || id.ends_with("_item")) {
                    item_names.emplace(id, item_obj["name"].as_string());
                }
                if (id.starts_with(rogue_index + "_totem")) {
                    totem_names.emplace(id, item_obj["name"].as_string());
                }
            }
        }
        for (auto&& [id, encounter_obj] : rogue_details["choiceScenes"].as_object()) {
            // very complicated way to reduce dupes. Will probably break sooner or later.
            if (id.ends_with("_enter")) {
                if (!id.starts_with("scene_ro3_rest")) {
                    if (!id.starts_with("scene_ro3_portal")
                        || id.starts_with("scene_ro3_portalsample")) {
                        encounter_nospace = encounter_obj["title"].as_string();
                        if (input_dir.string().ends_with("ko_KR\\gamedata\\excel")) {
                            encounter_nospace.erase(
                                std::remove(
                                    encounter_nospace.begin(),
                                    encounter_nospace.end(),
                                    ' '),
                                encounter_nospace.end());
                        }
                        encounter_names.emplace(id, encounter_nospace);
                    }
                }
            }
        }
    }

    std::unordered_map</*id*/ std::string, /*name*/ std::string> char_names;
    auto char_opt = json::open(input_dir / "character_table.json");
    if (!char_opt.has_value()) {
        std::cerr << "Failed to open character_table for " << input_dir << std::endl;
        return false;
    }

    auto& char_json = char_opt.value();
    for (auto&& [id, char_obj] : char_json.as_object()) {
        char_names.emplace(id, char_obj["name"].as_string());
    }

    auto task_opt = json::open(tasks_path);
    if (!task_opt) {
        std::cerr << "Failed to open tasks file: " << tasks_path << std::endl;
        return false;
    }
    auto& task_json = task_opt.value();

    auto proc = [&output_dir](
                    json::array& replace_array,
                    const std::unordered_map<std::string, std::string>& base_map,
                    const std::unordered_map<std::string, std::string>& cur_map) {
        std::unordered_map<std::string, std::string> exists_replace;
        for (const auto& replace : replace_array) {
            exists_replace.emplace(replace.as_array()[1], replace.as_array()[0]);
        }

        for (const auto& [id, base_name] : base_map) {
            // 国服有，但是外服还没有这个关的，跳过
            if (!cur_map.contains(id)) {
                continue;
            }
            // 国服和外服都有，但是名字一样的，跳过
            const std::string name = cur_map.at(id);
            if (base_name == name) {
                continue;
            }
            // 之前手动写的替换，跳过
            if (exists_replace.contains(base_name)) {
                continue;
            }
            replace_array.emplace_back(json::array { name, base_name });
        }
    };

    proc(task_json["BattleStageName"]["ocrReplace"].as_array(), base_stage_names, stage_names);
    proc(task_json["CharsNameOcrReplace"]["ocrReplace"].as_array(), base_char_names, char_names);
    proc(
        task_json["RoguelikeTraderShoppingOcr"]["ocrReplace"].as_array(),
        base_item_names,
        item_names);
    proc(
        task_json["Sami@Roguelike@FoldartalGainOcr"]["ocrReplace"].as_array(),
        base_totem_names,
        totem_names);
    proc (
        task_json["Sami@Roguelike@FoldartalUseOcr"]["ocrReplace"].as_array(),
        base_totem_names,
        totem_names);
    proc(
        task_json["Roguelike@StageEncounterOcr"]["ocrReplace"].as_array(),
        base_encounter_names,
        encounter_names);

    std::ofstream ofs(tasks_path, std::ios::out);
    ofs << task_json.format() << std::endl;
    ofs.close();

    return true;
}

bool update_version_info(
    const std::filesystem::path& input_dir,
    const std::filesystem::path& output_dir)
{
    uint64_t current_time = (unsigned long)time(NULL);

    json::value result;
    {
        const auto json_path = input_dir / "gacha_table.json";
        auto gacha_json_opt = json::open(json_path);
        if (!gacha_json_opt) {
            std::cerr << "faild to parse " << json_path;
            return false;
        }
        auto& gacha_json = *gacha_json_opt;

        std::unordered_map<std::string, size_t> pool_count;
        for (auto& gacha_info : gacha_json["gachaPoolClient"].as_array()) {
            pool_count[gacha_info["gachaPoolName"].as_string()]++;
        }

        uint64_t time_var = 0;
        std::string pool;
        for (auto& gacha_info : gacha_json["gachaPoolClient"].as_array()) {
            const auto& pool_name = gacha_info["gachaPoolName"].as_string();
            if (pool_count[pool_name] > 5) { // 把常驻池过滤掉
                continue;
            }
            auto pool_time = gacha_info["openTime"].as_unsigned_long_long();
            if ((time_var < pool_time) && (current_time > pool_time)) {
                time_var = pool_time;
                pool = pool_name;
            }
        }

        result["gacha"]["time"] = time_var;
        result["gacha"]["pool"] = pool;
    }
    {
        const auto json_path = input_dir / "activity_table.json";
        auto activity_json_opt = json::open(json_path);
        if (!activity_json_opt) {
            std::cerr << "faild to parse " << json_path;
            return false;
        }

        uint64_t time_var = 0;
        std::string name;

        auto& activity_json = *activity_json_opt;
        for (const auto& [_, act] : activity_json["basicInfo"].as_object()) {
            if (!act.at("displayOnHome").as_boolean()) {
                continue;
            }
            auto activity_time = act.at("startTime").as_unsigned_long_long();
            if ((time_var < activity_time) && (current_time > activity_time)) {
                time_var = activity_time;
                name = act.at("name").as_string();
            }
        }

        result["activity"]["time"] = time_var;
        result["activity"]["name"] = name;
    }
    static auto time = asst::utils::get_format_time();
    result["last_updated"] = time;

    std::ofstream ofs(output_dir / "version.json", std::ios::out);
    ofs << result.format() << std::endl;
    ofs.close();

    return true;
}
