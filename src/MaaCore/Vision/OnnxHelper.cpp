#include "OnnxHelper.h"

#include "Utils/NoWarningCV.h"

using namespace asst;

std::vector<float> OnnxHelper::image_to_tensor(const cv::Mat& image)
{
    cv::Mat src = image.clone();
    cv::cvtColor(src, src, cv::COLOR_BGR2RGB);

    cv::Mat chw = hwc_to_chw(src);
    cv::Mat chw_32f;
    chw.convertTo(chw_32f, CV_32F, 1.0 / 255.0);

    size_t tensor_size = 1ULL * src.cols * src.rows * src.channels();
    std::vector<float> tensor(tensor_size);
    std::memcpy(tensor.data(), chw_32f.data, tensor_size * sizeof(float));
    return tensor;
}

cv::Mat OnnxHelper::hwc_to_chw(const cv::Mat& src)
{
    std::vector<cv::Mat> rgb_images;
    cv::split(src, rgb_images);

    // Stretch one-channel images to vector
    cv::Mat m_flat_r = rgb_images[0].reshape(1, 1);
    cv::Mat m_flat_g = rgb_images[1].reshape(1, 1);
    cv::Mat m_flat_b = rgb_images[2].reshape(1, 1);

    // Now we can rearrange channels if need
    cv::Mat matArray[] = { m_flat_r, m_flat_g, m_flat_b };

    cv::Mat flat_image;
    // Concatenate three vectors to one
    cv::hconcat(matArray, 3, flat_image);
    return flat_image;
}
