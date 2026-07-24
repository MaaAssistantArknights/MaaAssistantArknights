#include "BlackFlowMovementRecognition.h"

#include <array>
#include <string_view>

#include "Vision/Matcher.h"

namespace asst::blackflow
{
std::optional<MovementKind> recognize_loaded_movement(const cv::Mat& image)
{
    struct LoadedTask
    {
        MovementKind movement;
        std::string_view task;
    };

    static constexpr std::array LoadedTasks = {
        LoadedTask { MovementKind::Walk, "BlackFlow@Roguelike@MovementLoaded-Walk" },
        LoadedTask { MovementKind::M01, "BlackFlow@Roguelike@MovementLoaded-M01" },
        LoadedTask { MovementKind::M02, "BlackFlow@Roguelike@MovementLoaded-M02" },
        LoadedTask { MovementKind::M03, "BlackFlow@Roguelike@MovementLoaded-M03" },
        LoadedTask { MovementKind::M05, "BlackFlow@Roguelike@MovementLoaded-M05" },
        LoadedTask { MovementKind::M06, "BlackFlow@Roguelike@MovementLoaded-M06" },
        LoadedTask { MovementKind::M07, "BlackFlow@Roguelike@MovementLoaded-M07" },
        LoadedTask { MovementKind::M08, "BlackFlow@Roguelike@MovementLoaded-M08" },
        LoadedTask { MovementKind::M09, "BlackFlow@Roguelike@MovementLoaded-M09" },
        LoadedTask { MovementKind::M10, "BlackFlow@Roguelike@MovementLoaded-M10" },
        LoadedTask { MovementKind::M11, "BlackFlow@Roguelike@MovementLoaded-M11" },
    };

    std::optional<MovementKind> best;
    double best_score = 0.0;
    for (const LoadedTask& loaded : LoadedTasks) {
        Matcher matcher(image);
        matcher.set_task_info(std::string(loaded.task));
        const auto result = matcher.analyze();
        if (result.has_value() && result->score > best_score) {
            best = loaded.movement;
            best_score = result->score;
        }
    }
    return best;
}
} // namespace asst::blackflow
