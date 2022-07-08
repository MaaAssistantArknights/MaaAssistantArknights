#include <filesystem>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <meojson/json.hpp>

bool update_items_data(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir);
bool cvt_single_item_template(const std::filesystem::path& input, const std::filesystem::path& output);

bool update_stages_data(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir);

bool update_infrast_templates(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir);

int main(int argc, char** argv)
{
    const char* str_exec_path = argv[0];
    const auto cur_path = std::filesystem::path(str_exec_path).parent_path();
    const std::filesystem::path input_dir = cur_path / "Arknights-Bot-Resource";

    std::cout << "------------Update Arknights-Bot-Resource------------" << std::endl;
    int git_ret = 0;
    if (!std::filesystem::exists(input_dir)) {
        git_ret = system(("git clone https://github.com/yuanyan3060/Arknights-Bot-Resource.git --depth=1 " + input_dir.string()).c_str());
    }
    else {
        git_ret = system(("git -C " + input_dir.string() + " pull").c_str());
    }
    if (git_ret != 0) {
        std::cout << "git cmd failed" << std::endl;
        return -1;
    }

    const auto solution_dir = std::filesystem::current_path().parent_path().parent_path();
    const auto resource_dir = solution_dir / "resource";
    const auto third_resource_dir = solution_dir / "3rdparty" / "resource";

    /* Update items template and json  from Arknights-Bot-Resource*/
    std::cout << "------------Update items template and json------------" << std::endl;
    if (!update_items_data(input_dir, resource_dir)) {
        std::cerr << "Update items data failed" << std::endl;
        return -1;
    }

    /* Update levels.json from Arknights-Bot-Resource*/
    std::cout << "------------Update levels.json------------" << std::endl;
    if (!std::filesystem::copy_file(
        input_dir / "levels.json",
        third_resource_dir / "Arknights-Tile-Pos" / "levels.json",
        std::filesystem::copy_options::overwrite_existing)) {
        std::cerr << "update levels.json failed" << std::endl;
        return -1;
    }

    /* Update infrast templates from Arknights-Bot-Resource*/
    std::cout << "------------Update infrast templates------------" << std::endl;
    if (!update_infrast_templates(input_dir / "building_skill", resource_dir / "template" / "infrast")) {
        std::cerr << "Update infrast templates failed" << std::endl;
        return -1;
    }

    /* Update stage.json from Penguin Stats*/
    std::cout << "------------Update stage.json------------" << std::endl;
    if (!update_stages_data(cur_path, solution_dir / "resource")) {
        std::cerr << "Update stages data failed" << std::endl;
        return -1;
    }

    std::cout << "------------All success------------" << std::endl;
    return 0;
}

bool update_items_data(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir)
{
    const auto input_json_path = input_dir / "gamedata" / "excel" / "item_table.json";

    std::ifstream ifs(input_json_path, std::ios::in);
    if (!ifs.is_open()) {
        std::cout << "open json failed" << std::endl;
        return false;
    }
    std::stringstream iss;
    iss << ifs.rdbuf();
    ifs.close();
    auto parse_ret = json::parse(iss.str());
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
            "uni_set_"              // 家具组合包
        };
        static const std::vector<std::string> BlackListSuffix = {
            "_rep_1",               // 复刻活动的活动代币
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
        if (!std::filesystem::exists(input_icon_path)) {
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

        static const auto output_icon_path = output_dir / "template" / "items";

        cvt_single_item_template(input_icon_path, output_icon_path / output_filename);
    }
    auto output_json_path = output_dir / "item_index.json";
    std::ofstream ofs(output_json_path, std::ios::out);
    ofs << output_json.format();
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
    cv::resize(dst, dst_resized, cv::Size(), 720.0 / 1080.0, 720.0 / 1080.0, cv::INTER_LINEAR_EXACT);
    cv::Rect quantity_roi(dst_resized.cols - 80, dst_resized.rows - 50, 80, 50);
    dst_resized(quantity_roi).setTo(0);

    dst_resized = dst_resized(cv::Rect(15, 15, 92, 92));

    cv::imwrite(output.string(), dst_resized);
    return true;
}

bool update_stages_data(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir)
{
    // 国内访问可以改成 .cn 的域名
    const std::string PenguinAPI = R"(https://penguin-stats.io/PenguinStats/api/v2/stages)";
    const std::filesystem::path TempFile = input_dir / "stages.json";
    int stage_request_ret = system(("curl -o " + TempFile.string() + " " + PenguinAPI).c_str());
    if (stage_request_ret != 0) {
        std::cerr << "Request Penguin Stats failed" << std::endl;
        return false;
    }
    std::ifstream ifs(TempFile, std::ios::in);
    if (!ifs.is_open()) {
        std::cout << "open stages.json failed" << std::endl;
        return false;
    }
    std::stringstream iss;
    iss << ifs.rdbuf();
    ifs.close();
    auto parse_ret = json::parse(iss.str());
    if (!parse_ret) {
        std::cout << "parse stages.json failed" << std::endl;
        return false;
    }

    auto& stage_json = parse_ret.value();

    std::ofstream ofs(output_dir / "stages.json", std::ios::out);
    ofs << stage_json.format();
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

        const std::vector<std::string> BlackList = {
            "[style]",
            "bskill_dorm",
            "bskill_train",
            "bskill_ws"
        };

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
