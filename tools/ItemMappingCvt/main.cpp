#include <filesystem>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <meojson/json.hpp>

bool trans_and_save_icon(const std::filesystem::path& input, const std::filesystem::path& output);

int main()
{
    const std::filesystem::path input_dir("Arknights-Bot-Resource");
    int git_ret = 0;
    if (std::filesystem::exists(input_dir)) {
        git_ret = system("git -C Arknights-Bot-Resource pull");
    }
    else {
        git_ret = system("git clone https://github.com/yuanyan3060/Arknights-Bot-Resource.git --depth=1");
    }
    if (git_ret != 0) {
        std::cout << "git cmd failed" << std::endl;
        return -1;
    }

    const auto input_json_path = input_dir / "gamedata" / "excel" / "item_table.json";

    std::ifstream ifs(input_json_path, std::ios::in);
    if (!ifs.is_open()) {
        std::cout << "open json failed" << std::endl;
        return -1;
    }
    std::stringstream iss;
    iss << ifs.rdbuf();
    ifs.close();
    auto parse_ret = json::parse(iss.str());
    if (!parse_ret) {
        std::cout << "parse json failed" << std::endl;
        return -1;
    }

    const auto solution_dir = std::filesystem::current_path().parent_path().parent_path();

    auto& input_json = parse_ret.value();
    json::value output_json;
    for (auto&& [item_id, item_info] : input_json["items"].as_object()) {
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

        static const auto output_icon_path = solution_dir / "resource" / "template" / "items";

        trans_and_save_icon(input_icon_path, output_icon_path / output_filename);
    }
    auto output_json_path = solution_dir / "resource" / "item_index.json";
    std::ofstream ofs(output_json_path, std::ios::out);
    ofs << output_json.format();

    return 0;
}

bool trans_and_save_icon(const std::filesystem::path& input, const std::filesystem::path& output)
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

    cv::imwrite(output.string(), dst_resized);
    return true;
}
