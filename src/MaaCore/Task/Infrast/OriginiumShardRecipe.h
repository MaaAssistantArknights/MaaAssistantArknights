#pragma once

#include <optional>
#include <string_view>

namespace cv
{
class Mat;
}

namespace asst
{
enum class OriginiumShardRecipe
{
    OriginiumOre,
    Device,
};

// 根据配方页显示的材料数量决定源石碎片配方；不允许使用装置时直接使用固源岩配方。
std::optional<OriginiumShardRecipe> detect_originium_shard_recipe(const cv::Mat& image, bool allow_device);

// 返回选择对应配方并继续完成数量和确认操作的任务名。
std::string_view originium_shard_recipe_task_name(OriginiumShardRecipe recipe) noexcept;
}
