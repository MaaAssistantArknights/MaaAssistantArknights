#pragma once

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <meojson/json.hpp>
#include <opencv2/core.hpp>

#include "TileDef.hpp"

namespace Map
{
    struct Tile
    {
        int heightType = 0;
        int buildableType = 0;
        std::string tileKey;
    };

    class Level
    {
    public:
        Level(const json::value& data);
        int get_width() const { return width; }
        int get_height() const { return height; }
        Tile get_item(int y, int x) const { return tiles[y][x]; }
        std::vector<cv::Point3d> view;
        LevelKey key;

    private:
        int height = 0;
        int width = 0;
        std::vector<std::vector<Tile>> tiles;
    };

    class TileCalc
    {
    public:
        TileCalc(int width, int height);

        bool run(const Level& level, bool side, std::vector<std::vector<cv::Point2d>>& out_pos,
                 std::vector<std::vector<Tile>>& out_tiles, double shift_x = 0, double shift_y = 0) const;

    private:
        bool adapter(double& x, double& y) const;

        int width = 0;
        int height = 0;
        const double degree = atan(1.0) * 4 / 180;
        std::vector<Level> levels;
        cv::Mat MatrixP = cv::Mat(4, 4, CV_64F);
        cv::Mat MatrixX = cv::Mat(4, 4, CV_64F);
        cv::Mat MatrixY = cv::Mat(4, 4, CV_64F);
    };

    inline void InitMat4x4(cv::Mat& m, double (*num)[4])
    {
        for (int i = 0; i < m.rows; i++)
            for (int j = 0; j < m.cols; j++)
                m.at<double>(i, j) = num[i][j];
    }

    inline Level::Level(const json::value& data)
    {
        key.stageId = data.at("stageId").as_string();
        key.code = data.at("code").as_string();
        key.levelId = data.at("levelId").as_string();
        key.name = data.get("name", "null");
        this->height = data.at("height").as_integer();
        this->width = data.at("width").as_integer();
        for (const json::value& point_data : data.at("view").as_array()) {
            cv::Point3d tmp;
            auto point_array = point_data.as_array();
            tmp.x = point_array[0].as_double();
            tmp.y = point_array[1].as_double();
            tmp.z = point_array[2].as_double();
            this->view.emplace_back(std::move(tmp));
        }
        for (const json::value& row : data.at("tiles").as_array()) {
            std::vector<Tile> tmp;
            tmp.reserve(this->width);
            for (const json::value& tile : row.as_array()) {
                tmp.emplace_back(Tile { tile.at("heightType").as_integer(), tile.at("buildableType").as_integer(),
                                        tile.get("tileKey", std::string()) });
            }
            tiles.emplace_back(std::move(tmp));
        }
    }

    inline TileCalc::TileCalc(int width, int height)
    {
        this->width = width;
        this->height = height;
        double ratio = static_cast<double>(height) / width;
        double matrixP[4][4] { { ratio / tan(20 * degree), 0, 0, 0 },
                               { 0, 1 / tan(20 * degree), 0, 0 },
                               { 0, 0, -(1000 + 0.3) / (1000 - 0.3), -(1000 * 0.3 * 2) / (1000 - 0.3) },
                               { 0, 0, -1, 0 } };
        InitMat4x4(this->MatrixP, matrixP);
        double matrixX[4][4] { { 1, 0, 0, 0 },
                               { 0, cos(30 * degree), -sin(30 * degree), 0 },
                               { 0, -sin(30 * degree), -cos(30 * degree), 0 },
                               { 0, 0, 0, 1 } };
        InitMat4x4(this->MatrixX, matrixX);
        double matrixY[4][4] { { cos(10 * degree), 0, sin(10 * degree), 0 },
                               { 0, 1, 0, 0 },
                               { -sin(10 * degree), 0, cos(10 * degree), 0 },
                               { 0, 0, 0, 1 } };
        InitMat4x4(this->MatrixY, matrixY);
    }

    inline bool TileCalc::run(const Level& level, bool side, std::vector<std::vector<cv::Point2d>>& out_pos,
                              std::vector<std::vector<Tile>>& out_tiles, double shift_x, double shift_y) const
    {
        auto [x, y, z] = level.view[side ? 1 : 0];
        double adapter_y = 0, adapter_z = 0;
        this->adapter(adapter_y, adapter_z);
        double matrix[4][4] {
            { 1, 0, 0, -x }, { 0, 1, 0, -y - adapter_y }, { 0, 0, 1, -z - adapter_z }, { 0, 0, 0, 1 }
        };
        auto raw = cv::Mat(cv::Size(4, 4), CV_64F);
        auto Finall_Matrix = cv::Mat(cv::Size(4, 4), CV_64F);
        InitMat4x4(raw, matrix);
        if (side) {
            Finall_Matrix = this->MatrixP * this->MatrixX * this->MatrixY * raw;
        }
        else {
            Finall_Matrix = this->MatrixP * this->MatrixX * raw;
        }
        int h = level.get_height();
        int w = level.get_width();
        auto map_point = cv::Mat(cv::Size(1, 4), CV_64F);
        map_point.at<double>(3, 0) = 1;
        auto tmp_pos = std::vector<cv::Point2d>(w);
        auto tmp_tiles = std::vector<Tile>(w);
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                tmp_tiles[j] = level.get_item(i, j);
                map_point.at<double>(0, 0) = j - (w - 1) / 2.0 + shift_x;
                map_point.at<double>(1, 0) = (h - 1) / 2.0 - i + shift_y;
                map_point.at<double>(2, 0) = tmp_tiles[j].heightType * -0.4;
                cv::Mat view_point = Finall_Matrix * map_point;
                view_point = view_point / view_point.at<double>(3, 0);
                view_point = (view_point + 1) / 2;
                tmp_pos[j] = cv::Point2d(view_point.at<double>(0, 0) * this->width,
                                         (1 - view_point.at<double>(1, 0)) * this->height);
            }
            out_pos.emplace_back(tmp_pos);
            out_tiles.emplace_back(tmp_tiles);
        }
        return true;
    }

    inline bool TileCalc::adapter(double& x, double& y) const
    {
        const double fromRatio = 9.0 / 16;
        const double toRatio = 3.0 / 4;
        double ratio = static_cast<double>(height) / width;
        if (ratio < fromRatio - 0.00001) {
            x = 0;
            y = 0;
            return false;
        }
        double t = (ratio - fromRatio) / (toRatio - fromRatio);
        x = -1.4 * t;
        y = -2.8 * t;
        return true;
    }
} // namespace Map
