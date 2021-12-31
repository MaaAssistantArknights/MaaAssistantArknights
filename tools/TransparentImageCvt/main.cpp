/******
 * 转换透明图片，若Alpha通道不为255，则直接设置为黑色
 ******/

#include <filesystem>

#include <opencv2/opencv.hpp>

int main()
{
    auto solution_dir = std::filesystem::current_path().parent_path().parent_path();
    auto input_dir = solution_dir / "resource" / "infrast";
    auto output_dir = solution_dir / "resource" / "infrast" / "cvt";

    for (auto&& entry : std::filesystem::directory_iterator(input_dir)) {
        if (entry.path().extension() != ".png") {
            continue;
        }
        cv::Mat image = cv::imread(entry.path().u8string(), -1);
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
        std::string out_file = (output_dir / entry.path().filename()).u8string();
        cv::imwrite(out_file, cvt);
    }

    return 0;
}
