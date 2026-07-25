#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace asst::blackflow::perception
{

struct Edge
{
    int id = -1;
    int node_a = -1;
    int node_b = -1;
    bool connected = false;
    bool cnn_connected = false;
    bool forced_by_connectivity_constraint = false;
    double raw_logit = 0.0;
    double calibrated_probability = 0.0;
    double confidence = 0.0;
    std::string decision_source = "cnn_threshold";
    std::vector<cv::Point2f> path;
};

}
