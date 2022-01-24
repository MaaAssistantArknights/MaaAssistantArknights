#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <fstream>
#include <iostream>

#include <opencv2/core.hpp>
#include <meojson/json.hpp>

namespace Map
{
    struct Tile
    {
        int heightType = 0;
        int buildableType = 0;
    };

    class Level
    {
    public:
        Level(const json::value& data);
        int get_width() const { return width; }
        int get_height() const { return height; }
        Tile get_item(int y, int x) const { return tiles[y][x]; }
        int view = 0;
        std::string stageId;
        std::string	code;
        std::string	levelId;
        std::string	name;
    private:
        int height = 0;
        int width = 0;
        std::vector<std::vector<Tile>> tiles;
    };
    class TileCalc
    {
    public:
        TileCalc(int width, int height, const std::string& dir);
        bool run(const std::string& code_or_name, bool side, std::vector<std::vector<cv::Point2d>>& out_pos, std::vector<std::vector<Tile>>& out_tiles) const;
    private:
        int width = 0;
        int height = 0;
        const double degree = atan(1.0) * 4 / 180;
        std::vector<Level> levels;
        cv::Mat MatrixP = cv::Mat(4, 4, CV_64F);
        cv::Mat MatrixX = cv::Mat(4, 4, CV_64F);
        cv::Mat MatrixY = cv::Mat(4, 4, CV_64F);
        bool adapter(double& x, double& y) const;
    };

    inline void InitMat4x4(cv::Mat& m, double(*num)[4])
    {
        for (int i = 0; i < m.rows; i++)
            for (int j = 0; j < m.cols; j++)
                m.at<double>(i, j) = num[i][j];
    }

    Level::Level(const json::value& data)
    {
        Level::stageId = data.at("stageId").as_string();
        Level::code = data.at("code").as_string();
        Level::levelId = data.at("levelId").as_string();
        Level::name = data.get("name", "null");
        Level::height = data.at("height").as_integer();
        Level::width = data.at("width").as_integer();
        Level::view = data.at("view").as_integer();
        for (const json::value& row : data.at("tiles").as_array()) {
            std::vector<Tile> tmp(Level::width);
            for (const json::value& tile : row.as_array()) {
                tmp.emplace_back(Tile{ tile.at("heightType").as_integer(), tile.at("buildableType").as_integer() });
            }
            tiles.emplace_back(std::move(tmp));
        }
    }

    TileCalc::TileCalc(int width, int height, const std::string& dir)
    {
        TileCalc::width = width;
        TileCalc::height = height;
        double ratio = static_cast<double>(height) / width;
        double matrixP[4][4]{
            { ratio / tan(20 * degree), 0, 0, 0},
            { 0, 1 / tan(20 * degree), 0, 0},
            { 0, 0, -(1000 + 0.3) / (1000 - 0.3), -(1000 * 0.3 * 2) / (1000 - 0.3)},
            { 0, 0, -1, 0 }
        };
        InitMat4x4(TileCalc::MatrixP, matrixP);
        double matrixX[4][4]{
                    { 1, 0, 0, 0},
                    { 0, cos(30 * degree), -sin(30 * degree), 0},
                    { 0, -sin(30 * degree), -cos(30 * degree), 0},
                    { 0, 0, 0, 1}
        };
        InitMat4x4(TileCalc::MatrixX, matrixX);
        double matrixY[4][4]{
                    { cos(10 * degree), 0, sin(10 * degree), 0},
                    { 0, 1, 0, 0},
                    { -sin(10 * degree), 0, cos(10 * degree), 0},
                    { 0, 0, 0, 1}
        };
        InitMat4x4(TileCalc::MatrixY, matrixY);
        std::ifstream ifs(dir, std::ios::in);
        if (!ifs.is_open()) {
            std::cerr << "Read resource failed" << std::endl;
            throw "Read resource failed";
        }
        std::stringstream iss;
        iss << ifs.rdbuf();
        ifs.close();
        std::string content = iss.str();
        auto ret = json::parse(content);
        if (!ret) {
            std::cerr << "Parsing failed" << std::endl;
            throw "Parsing failed";
        }
        for (const json::value& item : ret.value().as_array()) {
            TileCalc::levels.emplace_back(item);
        }
    }

    bool TileCalc::adapter(double& x, double& y) const
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

    bool TileCalc::run(const std::string& code_or_name, bool side, std::vector<std::vector<cv::Point2d>>& out_pos, std::vector<std::vector<Tile>>& out_tiles) const
    {
        bool runned = false;
        double x = 0, y = 0, z = 0;
        for (const Map::Level& level : TileCalc::levels) {
            if (level.code == code_or_name || level.name == code_or_name) {
                switch (level.view) {
                case 0:
                    x = 0;
                    y = -4.81;
                    z = -7.76;
                    if (side) {
                        x += 0.5975104570388794;
                        y -= 0.5;
                        z -= 0.882108688354492;
                    }
                    break;
                case 1:
                    x = 0;
                    y = -5.60;
                    z = -8.92;
                    if (side) {
                        x += 0.7989424467086792;
                        y -= 0.5;
                        z -= 0.86448486328125;
                    }
                    break;
                case 2:
                    x = 0;
                    y = -5.08;
                    z = -8.04;
                    if (side) {
                        x += 0.6461319923400879;
                        y -= 0.5;
                        z -= 0.877854309082031;
                    }
                    break;
                default:
                    x = 0;
                    y = -6.1;
                    z = -9.78;
                    if (side) {
                        x += 0.948279857635498;
                        y -= 0.5;
                        z -= 0.85141918182373;
                    }
                    break;
                }
                double adapter_y = 0, adapter_z = 0;
                TileCalc::adapter(adapter_y, adapter_z);
                double matrix[4][4]{
                    { 1, 0, 0, -x},
                    { 0, 1, 0, -y - adapter_y},
                    { 0, 0, 1, -z - adapter_z},
                    { 0, 0, 0, 1}
                };
                auto raw = cv::Mat(cv::Size(4, 4), CV_64F);
                auto Finall_Matrix = cv::Mat(cv::Size(4, 4), CV_64F);
                InitMat4x4(raw, matrix);
                if (side) {
                    Finall_Matrix = TileCalc::MatrixP * TileCalc::MatrixX * TileCalc::MatrixY * raw;
                }
                else {
                    Finall_Matrix = TileCalc::MatrixP * TileCalc::MatrixX * raw;
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
                        map_point.at<double>(0, 0) = j - (w - 1) / 2.0;
                        map_point.at<double>(1, 0) = (h - 1) / 2.0 - i;
                        map_point.at<double>(2, 0) = tmp_tiles[j].heightType * -0.4;
                        cv::Mat view_point = Finall_Matrix * map_point;
                        view_point = view_point / view_point.at<double>(3, 0);
                        view_point = (view_point + 1) / 2;
                        tmp_pos[j] = cv::Point2d(view_point.at<double>(0, 0) * TileCalc::width, (1 - view_point.at<double>(1, 0)) * TileCalc::height);
                    }
                    out_pos.emplace_back(tmp_pos);
                    out_tiles.emplace_back(tmp_tiles);
                }
                runned = true;
                break;
            }
        }
        return runned;
    }
}
