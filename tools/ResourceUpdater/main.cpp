#include <filesystem>
#include <fstream>
#include <unordered_set>

#include "Utils/Ranges.hpp"
#include "Utils/StringMisc.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 5054)
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

static inline void ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s)
{
    ltrim(s);
    rtrim(s);
}

bool update_items_data(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir,
                       bool with_imgs = true);
bool cvt_single_item_template(const std::filesystem::path& input, const std::filesystem::path& output);

bool update_infrast_data(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir);
bool update_stages_data(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir);
bool update_roguelike_recruit(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir,
                              const std::filesystem::path& solution_dir);

bool update_levels_json(const std::filesystem::path& input_file, const std::filesystem::path& output_dir);

bool update_infrast_templates(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir);
bool generate_english_roguelike_stage_name_replacement(const std::filesystem::path& ch_file,
                                                       const std::filesystem::path& en_file);
bool update_battle_chars_info(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir);
bool update_recruitment_data(const std::filesystem::path& input_dir, const std::filesystem::path& output, bool is_base);

bool check_roguelike_replace_for_overseas(const std::filesystem::path& input_dir,
                                          const std::filesystem::path& tasks_path,
                                          const std::filesystem::path& base_dir,
                                          const std::filesystem::path& output_dir);

int main([[maybe_unused]] int argc, char** argv)
{
    const char* str_exec_path = argv[0];
    const auto cur_path = std::filesystem::path(str_exec_path).parent_path();
    const std::filesystem::path arkbot_res_dir = cur_path / "Arknights-Bot-Resource";

    std::cout << "------------Update Arknights-Bot-Resource------------" << std::endl;
    std::string git_cmd;
    if (!std::filesystem::exists(arkbot_res_dir)) {
        git_cmd = "git clone https://github.com/yuanyan3060/Arknights-Bot-Resource.git --depth=1 \"" +
                  arkbot_res_dir.string() + "\"";
    }
    else {
        git_cmd = "git -C \"" + arkbot_res_dir.string() + "\" pull";
    }
    int git_ret = system(git_cmd.c_str());
    if (git_ret != 0) {
        std::cout << "git cmd failed" << std::endl;
        return -1;
    }

    const auto solution_dir = std::filesystem::current_path().parent_path().parent_path();
    const auto resource_dir = solution_dir / "resource";

    /* Update levels.json from Arknights-Bot-Resource*/
    std::cout << "------------Update levels.json------------" << std::endl;
    if (!update_levels_json(arkbot_res_dir / "levels.json", resource_dir / "Arknights-Tile-Pos")) {
        std::cerr << "update levels.json failed" << std::endl;
        return -1;
    }

    // 这个 en_levels.json 是自己手动生成放进去的
    generate_english_roguelike_stage_name_replacement(arkbot_res_dir / "levels.json", cur_path / "en_levels.json");

    /* Update infrast data from Arknights-Bot-Resource*/
    std::cout << "------------Update infrast data------------" << std::endl;
    if (!update_infrast_data(arkbot_res_dir, resource_dir)) {
        std::cerr << "Update infrast data failed" << std::endl;
        return -1;
    }

    /* Update infrast templates from Arknights-Bot-Resource*/
    std::cout << "------------Update infrast templates------------" << std::endl;
    if (!update_infrast_templates(arkbot_res_dir / "building_skill", resource_dir / "template" / "infrast")) {
        std::cerr << "Update infrast templates failed" << std::endl;
        return -1;
    }

    ///* Update roguelike recruit data from Arknights-Bot-Resource*/
    // std::cout << "------------Update roguelike recruit data------------" << std::endl;
    // if (!update_roguelike_recruit(arkbot_res_dir, resource_dir, solution_dir)) {
    //     std::cerr << "Update roguelike recruit data failed" << std::endl;
    //     return -1;
    // }

    /* Update base_name.json from Penguin Stats*/
    std::cout << "------------Update stage.json------------" << std::endl;
    if (!update_stages_data(cur_path, resource_dir)) {
        std::cerr << "Update stages data failed" << std::endl;
        return -1;
    }

    /* Update battle chars info from Arknights-Bot-Resource*/
    std::cout << "------------Update battle chars info------------" << std::endl;
    if (!update_battle_chars_info(arkbot_res_dir, resource_dir)) {
        std::cerr << "Update battle chars info failed" << std::endl;
        return -1;
    }

    /* Git update ArknightsGameData */
    std::cout << "------------Update ArknightsGameData------------" << std::endl;
    const std::filesystem::path game_data_dir = cur_path / "ArknightsGameData";

    if (!std::filesystem::exists(game_data_dir)) {
        git_cmd =
            "git clone https://github.com/Kengxxiao/ArknightsGameData.git --depth=1 \"" + game_data_dir.string() + "\"";
    }
    else {
        git_cmd = "git -C \"" + game_data_dir.string() + "\" pull";
    }
    git_ret = system(git_cmd.c_str());
    if (git_ret != 0) {
        std::cout << "git cmd failed" << std::endl;
        return -1;
    }

    /* Update recruitment data from ArknightsGameData*/
    std::cout << "------------Update recruitment data------------" << std::endl;
    if (!update_recruitment_data(game_data_dir / "zh_CN" / "gamedata" / "excel", resource_dir / "recruitment.json",
                                 true)) {
        std::cerr << "Update recruitment data failed" << std::endl;
        return -1;
    }

    std::unordered_map<std::string, std::string> global_dirs = {
        // 繁中的gamedata很久没更新了，暂时不从这里拿数据了
        { "en_US", "YoStarEN" },
        { "ja_JP", "YoStarJP" },
        { "ko_KR", "YoStarKR" },
        /* { "zh_TW", "txwy" } */
    };
    for (const auto& [in, out] : global_dirs) {
        std::cout << "------------Update recruitment data for " << out << "------------" << std::endl;
        if (!update_recruitment_data(game_data_dir / in / "gamedata" / "excel",
                                     resource_dir / "global" / out / "resource" / "recruitment.json", false)) {
            std::cerr << "Update recruitment data failed" << std::endl;
            return -1;
        }
    }

    /* Update items template and json  from Arknights-Bot-Resource*/
    std::cout << "------------Update items template and json------------" << std::endl;
    if (!update_items_data(arkbot_res_dir, resource_dir)) {
        std::cerr << "Update items data failed" << std::endl;
        return -1;
    }
    /* Update items global json from ArknightsGameData*/
    for (const auto& [in, out] : global_dirs) {
        std::cout << "------------Update items json for " << out << "------------" << std::endl;
        if (!update_items_data(game_data_dir / in, resource_dir / "global" / out / "resource", false)) {
            std::cerr << "Update items json failed" << std::endl;
            return -1;
        }
    }

    for (const auto& [in, out] : global_dirs) {
        // 台服的gamedata很久没更新了，先不管了
        // 韩服maa还没支持肉鸽，也不管了
        if (in == "zh_TW" || in == "ko_KR") {
            continue;
        }
        if (!check_roguelike_replace_for_overseas(game_data_dir / in / "gamedata" / "excel",
                                                  resource_dir / "global" / out / "resource" / "tasks.json",
                                                  game_data_dir / "zh_CN" / "gamedata" / "excel", cur_path / in)) {
            std::cerr << "Update roguelike replace for overseas failed" << std::endl;
            return -1;
        }
    }

    std::cout << "------------All success------------" << std::endl;
    return 0;
}

bool update_items_data(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir, bool with_imgs)
{
    const auto input_json_path = input_dir / "gamedata" / "excel" / "item_table.json";

    auto parse_ret = json::open(input_json_path);
    if (!parse_ret) {
        std::cout << "parse json failed" << std::endl;
        return false;
    }

    auto& input_json = parse_ret.value();
    json::value output_json;
    for (auto&& [item_id, item_info] : input_json["items"].as_object()) {
        static const std::vector<std::string> BlackListPrefix = {
            "LIMITED_TKT_GACHA_10", // 限定十连
            "p_char_",              // 角色信物（潜能）
            "tier",                 // 职业潜能
            "voucher_",             // 干员晋升、皮肤自选券等
            "renamingCard",         // 改名卡
            "ap_item_",             // 干员发邮件送的东西
            "ap_supply_lt_100_202", // 干员发邮件送的理智（注意前半段id是理智小样，不能全过滤）
            "clue_",                // 火蓝之心活动的扭蛋什么的
            "2020recruitment10",    // 周年自选券
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
            std::cout << input_icon_path << " not exist" << std::endl;
            continue;
        }

        auto& output = output_json[item_id];
        output["name"] = item_info["name"];
        std::string output_filename = item_id + ".png";
        output["icon"] = output_filename;
        output["usage"] = item_info["usage"];
        output["description"] = item_info["description"];
        output["sortId"] = item_info["sortId"];
        output["classifyType"] = item_info["classifyType"];

        if (with_imgs) {
            static const auto output_icon_path = output_dir / "template" / "items";
            cvt_single_item_template(input_icon_path, output_icon_path / output_filename);
        }
    }
    auto output_json_path = output_dir / "item_index.json";
    std::ofstream ofs(output_json_path, std::ios::out);
    ofs << output_json.format(true);
    ofs.close();

    return true;
}

bool cvt_single_item_template(const std::filesystem::path& input, const std::filesystem::path& output)
{
    cv::Mat src = cv::imread(input.string(), -1);
    if (src.empty()) {
        std::cout << input << " is empty" << std::endl;
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
    cv::resize(dst, dst_resized, cv::Size(), 720.0 / 1080.0, 720.0 / 1080.0, cv::INTER_AREA);
    cv::Rect quantity_roi(dst_resized.cols - 80, dst_resized.rows - 50, 80, 50);
    dst_resized(quantity_roi).setTo(0);

    dst_resized = dst_resized(cv::Rect(15, 15, 92, 92));

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

bool update_infrast_data(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir)
{
    const auto input_file = input_dir / "gamedata" / "excel" / "building_data.json";
    const auto output_file = output_dir / "infrast.json";

    json::value input_json;
    {
        auto opt = json::open(input_file);
        if (!opt) {
            std::cout << input_file << " parse error" << std::endl;
            return false;
        }
        input_json = std::move(opt.value());
    }

    json::value old_json;
    {
        auto opt = json::open(output_file);
        if (!opt) {
            std::cout << output_file << " parse error" << std::endl;
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

        std::string key = static_cast<std::string>(buff_obj["skillIcon"]);
        std::string name = static_cast<std::string>(buff_obj["buffName"]);
        // 这玩意里面有类似 xml 的东西，全删一下
        std::string desc = static_cast<std::string>(buff_obj["description"]);
        remove_xml(desc);

        auto& skill = root[room_type]["skills"][key];
        auto& name_arr = skill["name"].as_array();
        bool new_name = true;
        for (auto& name_obj : name_arr) {
            if (name_obj.as_string() == name) {
                new_name = false;
                break;
            }
        }
        if (new_name) {
            skill["name"].array_emplace(name);
            skill["desc"].array_emplace(desc);
        }

        // 历史遗留问题，以前的图片是从wiki上爬的，都是大写开头
        // Windows下不区分大小写，现在新的小写文件名图片没法覆盖
        // 所以干脆全用大写开头算了
        std::string filename = key + ".png";
        filename[0] -= 32;
        skill["template"] = std::move(filename);
    }
    root["roomType"] = json::array(rooms);

    std::ofstream ofs(output_file, std::ios::out);
    ofs << root.format(true);
    ofs.close();

    return true;
}

bool update_stages_data(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir)
{
    // 国内访问可以改成 .cn 的域名
    const std::string PenguinAPI = R"(https://penguin-stats.io/PenguinStats/api/v2/stages)";
    const std::filesystem::path TempFile = input_dir / "stages.json";
    int stage_request_ret = system(("curl -o \"" + TempFile.string() + "\" " + PenguinAPI).c_str());
    if (stage_request_ret != 0) {
        std::cerr << "Request Penguin Stats failed" << std::endl;
        return false;
    }

    auto parse_ret = json::open(TempFile);
    if (!parse_ret) {
        std::cout << "parse stages.json failed" << std::endl;
        return false;
    }

    auto& stage_json = parse_ret.value();

    std::ofstream ofs(output_dir / "stages.json", std::ios::out);
    ofs << stage_json.format(true);
    ofs.close();

    return true;
}

bool update_infrast_templates(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir)
{
    for (auto&& entry : std::filesystem::directory_iterator(input_dir)) {
        if (entry.path().extension() != ".png") {
            continue;
        }
        const std::string& stem = entry.path().stem().string();

        const std::vector<std::string> BlackList = { "[style]", "bskill_dorm", "bskill_train", "bskill_ws" };

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
        cv::Mat cvt;
        cv::cvtColor(image, cvt, cv::COLOR_BGRA2BGR);
        for (int c = 0; c != image.cols; ++c) {
            for (int r = 0; r != image.rows; ++r) {
                auto p = image.at<cv::Vec4b>(c, r);
                if (p[3] != 255) {
                    cvt.at<cv::Vec3b>(c, r) = cv::Vec3b(0, 0, 0);
                }
            }
        }
        std::string filename = entry.path().filename().string();
        // 历史遗留问题，以前的图片是从wiki上爬的，都是大写开头
        // Windows下不区分大小写，现在新的小写文件名图片没法覆盖
        // 所以干脆全用大写开头算了
        filename[0] -= 32;
        std::string out_file = (output_dir / filename).string();

        if (!std::filesystem::exists(out_file)) {
            std::cout << "New infrast templ: " << out_file << std::endl;
        }
        cv::imwrite(out_file, cvt);
    }
    return true;
}

bool update_roguelike_recruit(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir,
                              const std::filesystem::path& solution_dir)
{
    std::string python_cmd;
    std::filesystem::path python_file =
        solution_dir / "tools" / "RoguelikeResourceUpdater" / "generate_roguelike_recruit.py";
    python_cmd = "python " + python_file.string() + " --input=\"" + input_dir.string() + "\" --output=\"" +
                 output_dir.string() + "\"";
    int python_ret = system(python_cmd.c_str());
    if (python_ret != 0) {
        return false;
    }
    return true;
}

bool update_levels_json(const std::filesystem::path& input_file, const std::filesystem::path& output_dir)
{
    auto json_opt = json::open(input_file);
    if (!json_opt) {
        std::cerr << input_file << " parse failed" << std::endl;
        return false;
    }
    auto& root = json_opt.value();
    for (auto& stage_info : root.as_array()) {
        std::string filename = stage_info["stageId"].as_string() + "-" + stage_info["levelId"].as_string() + ".json";
        asst::utils::string_replace_all_in_place(filename, "/", "-");
        std::ofstream ofs(output_dir / filename, std::ios::out);
        ofs << stage_info.format(true);
        ofs.close();
    }
    return true;
}

bool generate_english_roguelike_stage_name_replacement(const std::filesystem::path& ch_file,
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

bool update_battle_chars_info(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir)
{
    const auto& input_chars_file = input_dir / "gamedata" / "excel" / "character_table.json";
    const auto& input_range_file = input_dir / "gamedata" / "excel" / "range_table.json";

    auto chars_opt = json::open(input_chars_file);
    auto range_opt = json::open(input_range_file);
    if (!chars_opt || !range_opt) {
        return false;
    }
    auto& chars_json = chars_opt.value();
    auto& range_json = range_opt.value();

    json::value result;
    auto& range = result["ranges"].as_object();
    for (auto& [id, range_data] : range_json.as_object()) {
        if (int direction = range_data["direction"].as_integer(); direction != 1) {
            // 现在都是 1，朝右的，以后不知道会不会改，加个warning，真遇到再说
            std::cout << "!!!Warning!!! range_id: " << id << " 's direction is " << std::to_string(direction)
                      << std::endl;
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
    for (auto& [id, char_data] : chars_json.as_object()) {
        json::value char_new_data;
        std::string name = char_data["name"].as_string();

        char_new_data["name"] = name;
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
        chars.object_emplace(id, std::move(char_new_data));
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
    Amiya_data["profession"] = "WARRIOR";
    Amiya_data["rangeId"] = json::array { "1-1", "1-1", "1-1" };
    Amiya_data["rarity"] = 5;
    Amiya_data["position"] = "MELEE";
    chars.object_emplace("char_1001_amiya2", std::move(Amiya_data));

    const auto& out_file = output_dir / "battle_data.json";
    std::ofstream ofs(out_file, std::ios::out);
    ofs << result.format(true) << std::endl;

    return true;
}

bool update_recruitment_data(const std::filesystem::path& input_dir, const std::filesystem::path& output, bool is_base)
{
    using asst::ranges::find_if, asst::ranges::range;
    using asst::utils::string_replace_all_in_place;
    using asst::views::filter, asst::views::split, asst::views::transform, asst::views::drop_while;

    auto not_empty = []<range Rng>(Rng str) -> bool { return !str.empty(); };
    auto make_string_view = []<range Rng>(Rng str) -> std::string_view { return asst::utils::make_string_view(str); };

    const auto& recruitment_file = input_dir / "gacha_table.json";
    const auto& operators_file = input_dir / "character_table.json";

    auto recruitment_opt = json::open(recruitment_file);
    auto operatros_opt = json::open(operators_file);

    if (!recruitment_opt || !operatros_opt) {
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
        drop_while([&](std::string_view str) { return str.find("Lancet-2") == std::string_view::npos; }) |
        // 按照 \n 分割，若非空则取第一个元素
        transform([&](auto str) { return str | split('\n') | filter(not_empty); }) | filter(not_empty) |
        transform([&](auto strs) { return make_string_view(strs.front()); });

    for (std::string_view s : items) {
        for (std::string_view n : s | split('/') | filter(not_empty) | transform(make_string_view)) {
            std::string name(n);
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

    for (auto& [id, char_data] : operatros_opt->as_object()) {
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
                info.tags.emplace_back("支援机械");
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

        opers.array_emplace(json::object { { "id", id },
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
        tags.object_emplace(base_name, tag);
    }

    std::ofstream ofs(output, std::ios::out);
    ofs << result.format(true) << std::endl;

    return true;
}

bool check_roguelike_replace_for_overseas(const std::filesystem::path& input_dir,
                                          const std::filesystem::path& tasks_path,
                                          const std::filesystem::path& base_dir,
                                          const std::filesystem::path& output_dir)
{
    static std::unordered_map</*id*/ std::string, /*base_name*/ std::string> base_stage_names;
    static std::unordered_map</*id*/ std::string, /*base_name*/ std::string> base_item_names;

    if (base_stage_names.empty() || base_item_names.empty()) {
        auto rg_opt = json::open(base_dir / "roguelike_topic_table.json");
        if (!rg_opt) {
            std::cerr << "Failed to open roguelike_topic_table for" << base_dir << std::endl;
            return false;
        }
        auto& rg_json = rg_opt.value();
        for (auto&& [id, stage_obj] : rg_json["details"]["rogue_1"]["stages"].as_object()) {
            base_stage_names.emplace(id, stage_obj["name"].as_string());
        }
        for (auto&& [id, item_obj] : rg_json["details"]["rogue_1"]["items"].as_object()) {
            base_item_names.emplace(id, item_obj["name"].as_string());
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

    auto& rg_json = rg_opt.value();
    for (auto&& [id, stage_obj] : rg_json["details"]["rogue_1"]["stages"].as_object()) {
        stage_names.emplace(id, stage_obj["name"].as_string());
    }
    for (auto&& [id, item_obj] : rg_json["details"]["rogue_1"]["items"].as_object()) {
        item_names.emplace(id, item_obj["name"].as_string());
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

    auto proc = [&output_dir](json::array& replace_array, const std::unordered_map<std::string, std::string>& base_map,
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
            std::cout << "Roguelike add new field: " << base_name << ", " << output_dir << std::endl;
            replace_array.emplace_back(json::array { name, base_name });
        }
    };
    std::filesystem::create_directories(output_dir);

    {
        auto& stage_arr = task_json["BattleStageName"]["ocrReplace"].as_array();
        proc(stage_arr, base_stage_names, stage_names);
        std::ofstream ofs(output_dir / "stage.json", std::ios::out);
        ofs << stage_arr.format(true) << std::endl;
    }

    {
        auto& chars_arr = task_json["CharsNameOcrReplace"]["ocrReplace"].as_array();
        proc(chars_arr, base_char_names, char_names);
        std::ofstream ofs(output_dir / "chars.json", std::ios::out);
        ofs << chars_arr.format(true) << std::endl;
    }
    {
        auto& shopping_arr = task_json["RoguelikeTraderShoppingOcr"]["ocrReplace"].as_array();
        proc(shopping_arr, base_item_names, item_names);
        std::ofstream ofs(output_dir / "shopping.json", std::ios::out);
        ofs << shopping_arr.format(true) << std::endl;
    }

    return true;
}
