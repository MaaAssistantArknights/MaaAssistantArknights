#pragma once

#include "../AbstractRoguelikeTaskPlugin.h"
#include "RoguelikeMap.h"
#include "Vision/MultiTemplMatcher.h"
#include <opencv2/opencv.hpp>

namespace asst
{
    class RoguelikeRoutingTaskPlugin : public AbstractRoguelikeTaskPlugin {
    public:
        // using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        RoguelikeRoutingTaskPlugin(const AsstCallback& callback,
                                   Assistant* inst,
                                   std::string_view task_chain,
                                   std::shared_ptr<RoguelikeConfig> config);

        virtual ~RoguelikeRoutingTaskPlugin() override = default;
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

    private:
        bool generate_map();
        bool generate_edges(const std::size_t& curr, const cv::Mat& image, const int& curr_x);
        void select_route();
        void refresh_then_update_node(const std::size_t& node, const std::size_t& column);

        RoguelikeMap m_map;
        std::size_t m_pos = 0;
        std::unordered_map<std::string, RoguelikeNodeType> m_templ_type_map;

        bool m_need_generate_map = true;
        Point m_swipe_source = {0, 0};
        Point m_swipe_target = {0, 0};
        int m_origin_x = 0;
        int m_node_width = 0;
        int m_node_height = 0;
        int m_column_offset = 0;
        int m_nameplate_offset = 0;
        int m_roi_margin = 0;
        int m_direction_threshold = 0;
        std::function<bool(const MultiTemplMatcher::Result&, const MultiTemplMatcher::Result&)> m_result_compare =
            [](const MultiTemplMatcher::Result& r1, const MultiTemplMatcher::Result& r2) {
                return r1.rect.y < r2.rect.y;
            };
        std::function<bool(const cv::Point&, const cv::Point&)> m_point_compare_positive =
            [](const cv::Point& p1, const cv::Point& p2) {
            return p1.x < p2.x || p1.y > p2.y;
        };
        std::function<bool(const cv::Point&, const cv::Point&)> m_point_compare_negative =
            [](const cv::Point& p1, const cv::Point& p2) {
            return p1.x < p2.x || p1.y < p2.y;
        };
    };
}
