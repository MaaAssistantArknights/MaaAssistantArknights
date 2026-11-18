#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <meojson/json.hpp>

#include "Common/AsstMsg.h"
#include "Common/AsstTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "Task/AbstractTaskPlugin.h"

namespace asst
{

class SPAFarmTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    ~SPAFarmTaskPlugin() override = default;

    // `bi(a, s)` 的 10 个状态 —— service.pretty.mjs L4799-4800
    enum class ScreenState
    {
        Unknown,
        ConfirmReady,
        Strategy,
        Info,
        Squad,
        Result,
        Prep,
        Battle,
        Mode,
        Lobby,
    };

    struct OcrItem
    {
        double cx = 0.0;
        double cy = 0.0;
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        std::string text;
        double score = 0.0;
    };

    // `bi` 的返回对象（只保留业务用得到的字段；`pc` 恒 false 已删）
    struct ScreenInfo
    {
        ScreenState screen = ScreenState::Unknown;
        std::vector<OcrItem> texts;
        cv::Mat png;
        bool quick = false;          // `w`：是否极速模拟
        std::string levelName;       // `i`：极速模拟 / 规程等级N
        bool difficulty = false;     // `difficulty`
        bool personal_selected = false; // `personalSelected`
        bool combat = false;         // `combat`：顶部有 x/y 回合计数
        bool selected = false;       // `selected`：出现「确认选择」
        std::optional<int> wins;     // `wins`：已通过模拟 N 次
        std::optional<bool> won;     // `won`
    };

    struct DictEntry
    {
        std::string name;
        std::string norm;
        int len = 0;
        std::unordered_map<std::string, int> bag;
    };

    struct DictMatch
    {
        std::string name;
        int len = 0;
        int start = 0;
        int end = 0;
        double score = 0.0;
        bool exact = false;
    };

    struct StackGeomRule
    {
        std::string geom;      // "front1", "back1", "sameRow", "around4", "around8", "around12", "inRange"
        std::string need_bond; // required bond name on neighbor, or empty
        bool other_only = false;
        bool unconditional = false;
    };

    struct StackTraitInfo
    {
        std::vector<StackGeomRule> rules;
        bool on_skill = false;
        bool on_deploy = false;
        bool end_of_prep = false;
        bool infects_tile = false;
        bool uses_infected_tile = false;
        int stack_cap = 0;
    };

    struct OperatorTrait
    {
        std::string id;
        std::string text;
        std::vector<std::string> tags;
    };

    struct OperatorInfo
    {
        std::string chess_id;
        std::string char_id;
        std::string name;
        std::string norm_name;
        int tier = 1;
        bool elite = false;
        bool golden = false;
        std::string profession;
        std::string sub_profession_id;
        std::string position; // "MELEE" or "RANGED"
        int atk = 0;
        int def = 0;
        int max_hp = 0;
        int block_cnt = 1;
        int purchase_price = 2;
        int sell_price = 1;
        std::vector<std::string> bond_ids;
        std::vector<std::string> bond_names;
        std::string trait_text;
        std::vector<OperatorTrait> traits;              // `traits[]` 原文（tags 供 Nt 判定）
        std::vector<std::pair<int, int>> range_grid;    // `rangeGrid`（[dr, dc] 列表）
        bool self_heal = false;
        bool heals_allies = false;
        int res = 0;
        double bat = 0.0;
        StackTraitInfo stack_info;
        bool is_summoner = false;
        bool is_healer = false;
        bool can_block_air = false;
        int taunt_level = 1;
        double dps_weight = 1.0;
        bool facing_independent = false;
        std::vector<std::pair<int, int>> base_range; // [dr, dc] under RIGHT facing
    };

    struct BondInfo
    {
        std::string bond_id;
        std::string name;
        std::string norm_name;
        bool is_core = false;
        bool stackable = false;   // `stackable`（本数据里恒缺省 false）
        bool inverse = false;     // `inverse`
        std::string count_mode; // "FIELD_ONLY" or "FIELD_AND_BENCH"
        std::vector<int> thresholds;
        int max_Active = 6;
        std::string desc;
        std::vector<std::string> chess_ids;
        std::vector<std::string> operator_names;
    };

    struct ItemInfo
    {
        std::string id;
        std::string name;
        std::string norm_name;
        int tier = 1;
        bool golden = false;
        int price = 2;
        std::string effect;
    };

    struct StrategyRule
    {
        std::string id;
        std::string name;
        std::string norm_name;
        std::string owner;
        bool available = true;
        int passed = 0;
    };

    struct StageInfo
    {
        std::string stage_id;
        std::string name;
        int width = 13;
        int height = 7;
        std::vector<std::string> rows;
        std::vector<std::pair<int, int>> infection_tiles; // `special[]` 里 tileKey=="tile_infection"
    };

    struct BoardRefEntry
    {
        std::string stage_id;
        std::string camera;
        std::unordered_map<std::string, std::vector<double>> cells;
    };

    struct BossInfo
    {
        std::string enemy_id;
        std::string name;
        std::string norm_name;
        std::string description;
    };

    struct EnemyTypeInfo
    {
        std::string code;
        std::string name;
    };

    struct RoundPacing
    {
        int round = 1;
        int target_shop_level = 3;
        int min_deploy = 2;
        bool is_boss_round = false;
    };

    struct CameraCalibData
    {
        std::string camera;
        std::vector<std::array<double, 4>> anchors; // [gx, gy, px, py]
        std::array<double, 2> marker_E = { 0.0, 0.0 };
        std::array<double, 2> marker_S1 = { 0.0, 0.0 };
        std::array<double, 2> marker_S2 = { 0.0, 0.0 };
        int ref_n_E = 900;
        int ref_n_S1 = 950;
        int ref_n_S2 = 950;
        std::array<double, 9> base_H = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    };

    struct SimilarityFit
    {
        bool valid = false;
        int matched_markers = 0;
        double scale = 1.0;
        double cos_a = 1.0;
        double sin_a = 0.0;
        double tx = 0.0;
        double ty = 0.0;
        double err = 999.0;
        double angle_deg = 0.0;
    };

    struct CameraDetectResult
    {
        std::string camera; // "expanded", "collapsed", or ""
        std::array<double, 9> H = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
        SimilarityFit fit;
        std::string detail; // 诊断信息：每个 marker 的像素数与拟合参数
    };

    struct StableBoardState
    {
        std::array<double, 9> H = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
        std::array<double, 9> raw_H = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
        std::string camera;
        SimilarityFit fit;
        std::array<double, 4> pts = { 0.0, 0.0, 0.0, 0.0 };
        cv::Mat img;
    };

    struct ShopCard
    {
        int index = 0;
        int x = 0;
        std::string name;
        std::string kind; // "operator", "item", or ""
        std::string source;
        std::vector<std::string> bonds;
        bool free_card = false;
        int cost = 2;
        int tier = 1;
    };

    struct ShopHeaderState
    {
        std::optional<int> level;
        std::optional<int> gold;
        std::optional<int> upgrade_cost;
        std::optional<std::pair<int, int>> upgrade_btn;
        std::optional<std::pair<int, int>> refresh_btn;
        std::string rawLevel; // `Rt` 的 rawLevel（"strokes:N" 或 OCR 原串）
    };

    struct UnitSlot
    {
        std::string slot;   // "board:r,c" or "bench:k"
        std::string area;   // "board" or "bench"
        int r = 0;
        int c = 0;
        std::string name;
        std::string source; // "panel", "shop", "carried", "guess"
        std::string facing; // "RIGHT", "UP", "LEFT", "DOWN", or ""
        bool elite = false;
        bool verified = false;
        bool confirmed = false;
        bool ghost = false;
        bool auxiliary = false;
        bool item = false;
        std::vector<double> hist;
    };

    // `Vt` 扫出来的一格装备（原文的 `{ r, c, name }`）
    struct EquipCell
    {
        int r = 5;
        int c = 4;
        std::string name;
    };

    // `ot`（`e.drag`）的 `beforeUp` 返回值：下一个落点 + 是否最终
    struct DragHookResult
    {
        std::pair<double, double> to;
        bool final = false;
    };

    // `on` 里 `B` 的元素（`{ slot, name, elite, equipped, r, c }`）
    struct EquipTarget
    {
        std::string slot;
        std::string name;
        bool elite = false;
        int equipped = 0;
        int r = 0;
        int c = 0;
    };

    struct GrazeStats
    {
        double ground = 0.0;
        double air = 0.0;
        double early_ground = 0.0;
        double first_step = 0.0;
    };

    struct SpawnPathInfo
    {
        std::pair<int, int> spawn = { 0, 0 };
        std::unordered_map<std::string, double> ground_flow;
        std::unordered_map<std::string, int> ground_progress;
        std::string first_tile;
        bool is_single_tile = false;
    };

    struct StageTrafficMap
    {
        // ---- `Rr(a, s)` 的原始输出（service.pretty.mjs L5748-5822）----
        // 与下面那批旧字段并存，等 `Nt` 重写完成后旧字段整体删除。
        std::vector<std::vector<double>> ground;             // `ground`：按最短路条数概率分摊
        std::vector<std::vector<double>> ground_progress;    // `groundProgress`：0..1 距离进度
        std::vector<std::vector<double>> air;                // `air`：被医疗/远程覆盖的“空中”计数
        std::vector<std::vector<double>> air_progress;       // `airProgress`
        std::vector<std::vector<std::pair<int, int>>> ground_paths; // `groundPaths`
        std::vector<std::vector<std::pair<int, int>>> air_paths;    // `airPaths`
        std::vector<std::vector<std::vector<double>>> ground_by_start; // `groundByStart`
        std::vector<std::vector<std::optional<std::pair<int, int>>>> graze_from; // `grazeFrom`
        double air_weight = 0.0;

        std::string stage_id;
        int rows = 7;
        int cols = 13;
        std::vector<std::string> grid;
        std::vector<std::pair<int, int>> spawns;
        std::vector<std::pair<int, int>> goals;
        std::vector<SpawnPathInfo> paths;
        std::unordered_map<std::string, double> road_importance;
        std::vector<std::vector<double>> ground_traffic;
        std::vector<std::vector<double>> air_traffic;
        std::vector<std::vector<double>> early_ground_traffic;
        std::vector<std::vector<double>> ground_first_step;
        std::vector<std::vector<int>> ground_dist;
        std::vector<std::vector<int>> air_dist;
        std::set<std::string> chokepoints;
        double max_ground_traffic = 1.0;
        double max_air_traffic = 1.0;
    };

    struct PlacementEvalOptions
    {
        std::unordered_set<std::string> focus_bonds;
        double focus_bond_mul = 2.0;
        double core_bond_mul = 1.5;
        double support_defense_weight = 1.0;
        double support_heal_weight = 1.0;
        bool advance = false;
        bool avoid_right_col = false;
        std::string boss_side;
        std::string strategy = "balance"; // `strategy`："defense"（BOSS 回合）/ "balance" / "stack"
        int exclude_left_cols = 0;  // `excludeLeftCols`
        bool left_half_only = false; // `leftHalfOnly`
        int round = 0;
    };

    struct PlacementEvalResult
    {
        double score = -1e9;
        double defense = 0.0;
        double stacking = 0.0;
        double exposed = 0.0;
        double advance_score = 0.0;
        double gate = 0.0;
        int unguarded_routes = 0;
    };

    struct PlacementAssign
    {
        std::string slot;
        std::string from_area;
        int from_r = 0;
        int from_c = 0;
        std::string name;
        int r = 0;
        int c = 0;
        std::string facing = "LEFT";
        bool elite = false;
    };

    struct PlacementMove
    {
        std::string type; // "deploy", "reposition", "bench", "orient"
        std::string unit;
        std::string from_area;
        int from_r = 0;
        int from_c = 0;
        std::string to_area;
        int to_r = 0;
        int to_c = 0;
        std::string facing = "LEFT";
    };

    struct ShopPlanStep
    {
        std::string action; // "upgrade", "buy", "swap", "refresh", "freeze", "done"
        std::string target;
        std::string kind;
        int x = 0;
        int cost = 0;
        std::string sell_slot;
        std::string sell_name;
        std::string reason;
        bool filler = false; // `E.filler`：前期补位，招到第 9 名前会卖掉
    };

    struct InfoScreenData
    {
        struct BondStatus
        {
            std::string name;
            bool active = false;
            double green = 0.0;
            bool core = false;
        };
        std::vector<BondStatus> bonds;
        std::string boss;
        std::vector<std::string> enemy_types;
    };

    bool verify(AsstMsg msg, const json::value& details) const override;
    void set_mode(const std::string& mode); // `ls`：mode + lastRound(9/14)
    bool set_params(const json::value& params);
    void set_profile(bool low_profile);
    void apply_goal_bonds();  // `en(goal)`：按目标切换核心盟约
    void rebuild_stacker_names(); // `en(goal)` 的 `ht`：medal=固定 12 人，task-N=`la(目标盟约)`
    static bool trait_mentions_stacking(const OperatorInfo& op, const std::vector<std::string>& bonds); // `la`
    bool in_stacker_list(std::string_view name) const;
    void set_target_clears(int target_clears);
    void set_max_runs(int max_runs);
    bool _run() override;

private:
    // --- Low-level Controller & Timing ---
    static Point to_720p(double x_1080, double y_1080);
    cv::Mat shot() const;
    bool sleep_ms(int ms);
    bool sleep_sec(double sec);
    void tap(double x, double y, int wait_ms = 600);
    void swipe(double x1, double y1, double x2, double y2, int duration_ms = 650, int wait_ms = 600);
    void touch_event(int type, double x, double y) const;
    void drag_segments(
        double x1,
        double y1,
        double x2,
        double y2,
        int steps = 6,
        int step_ms = 25,
        int hold_ms = 80);
    cv::Mat drag_board_unit(
        double x1,
        double y1,
        double x2,
        double y2,
        bool capture_before_up = false);

    // --- Logging & MAA GUI ExtraInfo Reporting ---
    void log_info(const std::string& msg);
    void report_gui_progress(const std::string& event_type, const std::string& message);
    void report_match_finished(bool won, int round_reached);
    void report_task_summary(const std::string& stop_reason, bool completed_ok);
    int total_available_strategies() const;

    // --- OCR & Text Matching (`rt`, `Jr`, `xr`, `yi`) ---
    std::vector<OcrItem> ocr(
        const cv::Mat& img,
        const std::optional<std::array<int, 4>>& roi_xywh = std::nullopt,
        bool pad_replicate = true) const;
    std::vector<OcrItem> ocr_rec_only(
        const cv::Mat& img,
        const std::array<int, 4>& roi_xywh,
        int upscale = 1,
        int black_border = 0) const;
    static std::optional<OcrItem> find_ocr(
        const std::vector<OcrItem>& items,
        std::initializer_list<std::string_view> keywords);
    static std::optional<OcrItem> find_ocr_exact(
        const std::vector<OcrItem>& items,
        std::initializer_list<std::string_view> exact_keywords);
    static std::optional<OcrItem> find_ocr_in_roi(
        const std::vector<OcrItem>& items,
        std::initializer_list<std::string_view> keywords,
        int x0,
        int y0,
        int x1,
        int y1);
    static bool has_ocr(
        const std::vector<OcrItem>& items,
        std::initializer_list<std::string_view> keywords);
    static bool has_ocr_exact(
        const std::vector<OcrItem>& items,
        std::initializer_list<std::string_view> exact_keywords);
    static std::string norm_text(std::string_view sv);
    static std::unordered_map<std::string, int> char_bag(std::string_view norm_sv);
    static double bag_similarity(
        const std::unordered_map<std::string, int>& a,
        int len_a,
        const std::unordered_map<std::string, int>& b,
        int len_b);
    static std::vector<DictEntry> build_dict(const std::vector<std::string>& names);
    static std::vector<DictMatch> match_dict(
        std::string_view text,
        const std::vector<DictEntry>& dict,
        bool fuzzy = true);
    static double match_coverage(std::string_view text, const std::vector<DictMatch>& matches);

    // --- Data Loading (`oa`) ---
    bool load_all_data();
    void init_builtin_fallbacks();
    static StackTraitInfo parse_stack_trait(std::string_view trait_text);
    static std::vector<std::pair<int, int>> default_range_for_subprof(
        std::string_view sub_prof,
        std::string_view pos);
    static std::pair<int, int> rotate_offset(std::pair<int, int> dr_dc, std::string_view facing);
    const OperatorInfo* find_operator(std::string_view name) const;
    const ItemInfo* find_item(std::string_view name) const;
    const BondInfo* find_bond(std::string_view name) const;
    const StageInfo* find_stage(std::string_view stage_id) const;

    // --- Projective Homography & Camera Calibration (`kr`, `ft`, `os`, `Un`, `pn`, `wn`, `Hn`, `Sr`) ---
    static std::array<double, 9> solve_homography(const std::vector<std::array<double, 4>>& anchors);
    static std::pair<double, double> project_point(const std::array<double, 9>& H, double gx, double gy);
    static std::array<double, 9> invert_homography(const std::array<double, 9>& H);
    static std::array<double, 9> adjust_homography(const std::array<double, 9>& H, const SimilarityFit& fit);
    static std::optional<std::tuple<double, double, int>> detect_marker_centroid(
        const cv::Mat& img,
        const std::array<double, 2>& ref_pt,
        int top_offset,
        std::string_view color_kind);
    static SimilarityFit fit_similarity(
        const std::vector<std::pair<double, double>>& src_pts,
        const std::vector<std::pair<double, double>>& dst_pts);
    CameraDetectResult detect_camera(const cv::Mat& img, bool allow_goal_only = false) const;
    std::optional<CameraDetectResult> detect_goal_only_fallback(const cv::Mat& img) const;
    bool goal_only_allowed() const;
    std::array<double, 9> active_board_homography(const CameraDetectResult& cam) const;
    std::optional<StableBoardState> wait_stable_board(
        int timeout_ms = 5000,
        const std::optional<std::array<double, 4>>& prev_pts = std::nullopt);

    // --- Stage Identification & Info Screen (`Ri`, `ji`, `Ni`, `ki`, `lr`, `F`) ---
    std::unordered_map<std::string, std::vector<double>> sample_board_cells(
        const cv::Mat& img,
        const std::array<double, 9>& H) const;
    void update_stage_from_board(
        const cv::Mat& img,
        const std::array<double, 9>& H,
        std::string_view camera,
        const std::set<std::string>& occupied_cells);
    std::optional<InfoScreenData> read_info_screen(const cv::Mat& img) const;
    std::pair<std::optional<int>, bool> read_round_and_prep(
        const cv::Mat& img,
        const std::vector<OcrItem>& full_items) const;
    std::optional<int> read_remaining_deploy(const cv::Mat& img, const std::vector<OcrItem>& items) const;
    bool is_personal_selected(const cv::Mat& img, const std::vector<OcrItem>& items) const; // `wi`
    std::vector<int> detect_strategy_rows(const cv::Mat& img, bool selected) const;        // `Ft`

    // --- Shop Reader (`Kr`, `Kn`, `Rt`, `Bi`, `Ci`, `Di`, `Mt`) ---
    std::optional<int> read_shop_digits_4x(const cv::Mat& img, const std::array<int, 4>& roi_xywh) const;
    static std::optional<int> recognize_shop_level_roman_cc(const cv::Mat& img, int up_x);
    ShopHeaderState read_shop_header(const cv::Mat& img, const std::vector<OcrItem>& full_items) const;
    struct CardGridFit
    {
        double p = 0.0;
        int first = 0;
        int votes = 0;
    };
    static std::optional<CardGridFit> detect_card_gaps(const cv::Mat& img);
    std::vector<ShopCard> read_shop_cards(const cv::Mat& img);
    void detect_free_cards(const cv::Mat& img, std::vector<ShopCard>& cards) const;

    // --- Board Unit Detection, Sprite Histogram Tracking & Detail Panel (`he`, `yn`, `ta`, `Oi`, `Gt`, `Ce`, `Pt`, `zr`) ---
    static cv::Rect unit_sprite_box(const std::array<double, 9>& H, int r, int c, std::string_view camera);
    static double green_hp_bar_ratio(const cv::Mat& img, double cx, double top_y, double box_h);
    static std::vector<double> compute_hsv_hist(const cv::Mat& img, const cv::Rect& roi, const cv::Mat* bg_ref = nullptr);
    static double hist_intersection(const std::vector<double>& a, const std::vector<double>& b);
    std::vector<std::pair<double, double>> detect_unit_badges(
        const cv::Mat& img,
        const std::array<double, 9>& H,
        std::string_view camera,
        const std::vector<std::string>& rows) const;
    std::vector<UnitSlot> map_badges_to_units(
        const std::vector<std::pair<double, double>>& badges,
        const std::array<double, 9>& H,
        std::string_view camera,
        const std::vector<std::string>& rows) const;
    bool is_detail_panel_open(const cv::Mat& img) const;
    bool close_detail_panel();
    std::pair<std::string, std::string> read_unit_detail_panel(const cv::Mat& img) const;
    void refresh_board_snapshot(const cv::Mat& img, const CameraDetectResult& cam, bool force_shop = true);
    void inspect_unknown_units(const CameraDetectResult& cam, bool skip_bench_unknown = false, bool force_all = false);
    void check_triple_merge(const std::string& op_name, bool force = false); // `mt`/`resolveMerge`

    // --- Traffic Analysis & Simulated Annealing Placement Solver (`Rr`, `Nt`, `Zr`, `is`, `ss`, `cr`, `ws`) ---
    StageTrafficMap build_stage_traffic(std::string_view stage_id, bool bottom_only = false) const;
    // `Rr` 的内部原语
    static std::vector<std::pair<int, int>> cells_of(const std::vector<std::string>& rows, char ch);
    static std::vector<std::pair<int, int>> deploy_cells(const std::vector<std::string>& rows); // `Pr`
    struct BfsResult
    {
        std::vector<std::vector<int>> dist;
        std::vector<std::vector<int>> ways;
    };
    static BfsResult bfs_from(const std::vector<std::string>& rows, const std::vector<std::pair<int, int>>& starts);
    static std::vector<std::pair<int, int>> build_path(
        const std::vector<std::string>& rows,
        std::pair<int, int> from,
        std::pair<int, int> to); // `Hi`
    GrazeStats compute_cell_graze(
        const StageTrafficMap& traffic,
        const OperatorInfo& op,
        int r,
        int c,
        std::string_view facing) const;
    PlacementEvalOptions default_eval_options(bool advance_mode) const;
    PlacementEvalResult evaluate_placement_nt(
        const std::vector<PlacementAssign>& placement,
        const std::vector<UnitSlot>& bench_units,
        const StageTrafficMap& traffic,
        const PlacementEvalOptions& opts) const; // `Nt`（忠实移植）
    double evaluate_placement(
        const std::vector<PlacementAssign>& placement,
        const std::vector<UnitSlot>& bench_units,
        const StageTrafficMap& traffic,
        bool advance_mode = false) const;
    std::pair<std::vector<PlacementAssign>, std::vector<PlacementMove>> solve_placement(
        const StageTrafficMap& traffic,
        int max_deploy,
        bool advance_mode = false,
        const PlacementEvalOptions* extra = nullptr) const;
    static std::optional<std::pair<double, double>> detect_facing_white_box(const cv::Mat& img);
    bool execute_placement_move(const PlacementMove& mv, const CameraDetectResult& cam);
    int run_auto_place(const CameraDetectResult& cam, bool advance_mode = false, bool retry = false);

    // --- Shop Multi-Step Action Planner & Execution (`_r`, `gs`, `ps`, `Lr`, `ha`, `ur`, `ct`, `oe`) ---
    RoundPacing get_round_pacing(int round) const;
    double score_operator_for_plan(
        const OperatorInfo& op,
        const std::unordered_map<std::string, int>& owned_counts,
        const std::unordered_set<std::string>& active_bonds) const;
    // `ps` / `ha` 的模拟状态：只带决策需要的字段
    struct ShopSimUnit
    {
        std::string name;
        std::string area; // "board" / "bench"
        bool elite = false;
        bool auxiliary = false;
        bool item = false;
    };
    struct ShopSimState
    {
        std::vector<ShopSimUnit> units;
        std::vector<ShopCard> shop;
        int level = 1;
        int gold = 0;
        std::optional<int> upgrade_cost;
        int refreshes = 0;
    };
    struct ScoredShopCard
    {
        ShopCard card;
        double score = -1.0;
        int price = 0;
    };

    // `_r(a, s)` 的候选（tier 1/2/3，tier2 可能带出售目标）
    struct ShopCandidate
    {
        ShopCard card;
        std::string unit_name;
        int unit_tier = 0;
        int unit_price = 0;
        int tier = 0;
        std::string sell;
        bool has_sell = false;
    };

    // `e.std` —— 标准模式下的一次整备统计（`gs` 的第四参数）
    struct PrepTrackState
    {
        int bought = 0;
        int upgrades = 0;
        bool front_plan = false;
        int start_gold = 0;
        bool filler_bought = false;
        bool item_bought = false;
        bool use_items = false;
    };

    ShopSimState make_shop_sim(const ShopHeaderState& header) const;
    std::vector<ScoredShopCard> score_shop_cards(const ShopSimState& sim) const; // `Lr`
    ShopPlanStep ps_step(ShopSimState& sim) const;                              // `ps`
    ShopPlanStep gs_step(ShopSimState& sim, PrepTrackState& prep) const;         // `gs`
    std::vector<ShopCandidate> rank_shop_candidates(const ShopSimState& sim) const; // `_r`
    std::optional<std::string> pick_freeze_target(const ShopSimState& sim) const;  // `bs`
    static int stack_potential(const OperatorInfo& op);                            // `tr`
    bool is_stacker(const OperatorInfo& op) const;                                 // `Bn`
    std::optional<std::string> active_focus_bond(const ShopSimState& sim, bool include_bench) const; // `fa`
    int bond_need(const std::string& bond) const;                               // `xn.get(bond) ?? 3`
    std::vector<ShopPlanStep> plan_shop_Steps(
        const ShopHeaderState& header,
        const std::vector<ShopCard>& shop);
    bool buy_shop_card(int card_x, const std::string& expected_name, int upgrade_x_hint);
    bool upgrade_shop(const ShopHeaderState& header);
    bool sell_unit_slot(const std::string& slot, int gold_baseline); // `oe`
    bool close_equip_full_dialog(const std::string& unit_name); // `br`
    bool is_transition_unit(std::string_view name) const;                              // `lr`
    const UnitSlot* pick_sell_bench_unit(std::string_view except_name) const;          // `F`

    // --- Equipment Row `Q`, Classification (`Es`/`Fs`/`ma`/`vs`/`An`/`ga`), Jessica Pager, Mob Elite & Summons ---
    static std::string classify_item_category(std::string_view name, std::string_view effect);
    int equip_cap(const std::string& name) const;                                        // `Ct`
    std::vector<EquipTarget> build_equip_targets() const;                                // `on` 的 `B`
    const EquipTarget* pick_equipment_target(
        std::string_view category,
        const std::vector<EquipTarget>& targets,
        const EquipTarget* exclude = nullptr); // `ga(x, B, e.chess, ht, st)`
    static std::vector<uint8_t> capture_item_patch(
        const cv::Mat& img,
        const std::array<double, 9>& H,
        int r,
        int c);                                                                    // `Pt`
    static double patch_diff(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b); // `zr`
    std::vector<EquipCell> detect_item_slots(
        const std::vector<std::string>& rows,
        const std::array<double, 9>& H,
        bool force);                                                                    // `Vt`
    static std::optional<std::array<double, 4>> detect_drop_popup(const cv::Mat& before, const cv::Mat& after); // `sn`
    void drag_with_hook(
        double x1,
        double y1,
        double x2,
        double y2,
        int steps,
        int step_ms,
        int hold_ms,
        int press_ms,
        const std::function<std::optional<DragHookResult>(const std::pair<double, double>&)>& before_up); // `ot`
    bool equip_drag_verified(
        const std::string& tag,
        const std::array<double, 9>& H,
        const EquipCell& src,
        const EquipTarget& tgt,
        const std::string& expected_name); // `Cr`
    void handle_overflow_units(const CameraDetectResult& cam);
    void deploy_summons(const StageTrafficMap& traffic); // `Dt`
    void scan_and_distribute_equipment(const cv::Mat& img, const CameraDetectResult& cam);
    bool try_use_jessica_pager(const CameraDetectResult& cam);          // `Ur`（含收起/还原）
    bool jessica_pager_body(const CameraDetectResult& cam);             // `Dr`
    std::optional<UnitSlot> pick_pager_target() const;                  // `fr`
    std::optional<std::string> inspect_cell_name(const std::array<double, 9>& H, int r, int c); // `inspectCell`
    bool try_use_mob_skill(const CameraDetectResult& cam, int gold);
    void run_prep_phase(const cv::Mat& img, const CameraDetectResult& cam);

    // --- Screen State Machine & Strategy/Squad/Popup Handlers (`bi`, `Ot`, `K`, `ce`, `ln`) ---
    ScreenInfo classify_screen(const cv::Mat& img) const; // `bi`
    static cv::Mat extract_strategy_icon(const cv::Mat& img, int gx, int gy);
    static double icon_diff(const cv::Mat& a, const cv::Mat& b);
    static bool has_strategy_selected_border(const cv::Mat& img, int gx, int gy);
    bool has_strategy_badge(const cv::Mat& img, int gx, int gy, int size = 190) const; // `Vn`
    bool handle_strategy_screen(const cv::Mat& img);
    std::string strategy_from_panel(const ScreenInfo& info) const; // `mt`
    // `ys`（极速模拟）/ `xs`（标准模拟，规程等级选择）
    bool handle_lobby_quick(ScreenInfo info);
    bool handle_lobby_standard(ScreenInfo info, const std::string& level_name = "规程等级1");
    std::optional<ScreenInfo> wait_for(
        const std::function<bool(const ScreenInfo&)>& pred,
        int timeout_ms = 10000);
    void handle_squad_screen(const cv::Mat& img, const std::vector<OcrItem>& items);
    void handle_info_screen(const cv::Mat& img, const std::vector<OcrItem>& items);
    void handle_three_choice_screen(const cv::Mat& img, const std::vector<OcrItem>& items);
    void handle_decision_screen(const cv::Mat& img, const std::vector<OcrItem>& items);
    void switch_core_bond_group(const ScreenInfo& info); // `ln`

    // --- Configuration & Runtime State ---
    std::string m_profile = "full";
    std::string m_goal = "medal";
    int m_target = 3;
    std::string m_mode = "quick"; // `_t.mode`
    int m_last_round = 9;         // `_t.lastRound`（quick 9 / standard 14）
    int m_runs = 20;
    int m_attempts = 0;
    double m_step_scale = 1.0;

    bool m_data_loaded = false;
    std::vector<OperatorInfo> m_operators;
    std::unordered_map<std::string, size_t> m_op_by_norm;
    std::vector<BondInfo> m_bonds;
    std::unordered_map<std::string, size_t> m_bond_by_norm;
    std::vector<ItemInfo> m_items;
    std::unordered_map<std::string, size_t> m_item_by_norm;
    std::vector<StrategyRule> m_strategies;
    std::vector<StageInfo> m_stages;
    std::vector<BoardRefEntry> m_board_refs;
    std::vector<BossInfo> m_bosses;
    std::vector<EnemyTypeInfo> m_enemy_types;
    // `he` 的 6 个 TemplateMatch 模板（badge_l1..l6.png，绿底为掩码）
    std::vector<cv::Mat> m_badge_templates;
    CameraCalibData m_calib_expanded;
    CameraCalibData m_calib_collapsed;
    std::array<double, 9> m_collapsed_board_H = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };

    std::vector<DictEntry> m_chess_dict;
    std::vector<DictEntry> m_item_dict;
    std::vector<DictEntry> m_bond_dict;
    std::vector<DictEntry> m_boss_dict;
    std::vector<DictEntry> m_strategy_dict;

    // Active goal bonds & operator sets
    std::vector<std::string> m_active_core_bonds = { "精准", "奥术" };
    // `ht`：叠层手名单（medal 固定 12 人；task-N 由 `la` 从 traits 推导）
    std::vector<std::string> m_stacker_list;
    std::unordered_set<std::string> m_stacker_names;
    // 本局已激活盟约（`info.bonds.filter(active)`，对应 JS 的 `l`）。
    // 与 `m_active_core_bonds`（= 目标聚焦盟约 `st`，goal 决定，运行期不变）是两件事。
    std::vector<std::string> m_active_bond_names;
    int m_shop_refreshes = 0; // 本回合刷新次数（`e.refreshes`）

    // Match & Board Tracking State
    bool m_match_info_read = false;
    std::string m_stage_id = "act3autochess_m01";
    std::string m_stage_source = "default";
    std::pair<std::string, int> m_stage_vote = { "", 0 };
    int m_current_round = 0;
    int m_last_prep_round = -1;
    int m_current_shop_level = 1;
    int m_current_gold = 0;
    int m_deploy_cap = 8;
    std::optional<int> m_upgrade_x_Ref;

    std::vector<UnitSlot> m_units;
    std::vector<ShopCard> m_shop;
    std::unordered_map<std::string, UnitSlot> m_remembered_units;
    std::unordered_map<std::string, double> m_missing_since; // `missingSince`（slot → 秒）
    double m_last_merge_check = 0.0;                        // `Dr` 时间戳（5s 节流）
    std::unordered_map<std::string, std::string> m_panel_verified_slots;
    std::unordered_map<std::string, std::vector<std::vector<double>>> m_op_hist_bank;
    std::unordered_map<std::string, cv::Mat> m_empty_cell_refs;
    std::vector<std::string> m_recent_bought_names;
    std::unordered_set<std::string> m_bought_items;
    std::string m_current_boss;
    std::unordered_map<int, int> m_shop_upgrade_costs; // `t.shop[].initialUpgradePrice`（按商店等级）
    bool m_verify_upgrade = false;                     // `X.verifyUpgrade`（生产默认 false）
    std::unordered_set<std::string> m_elite_units;      // `Et`：已确认精锐（合成/标记过）
    std::unordered_set<std::string> m_pending_merge;    // `p`：本回合待做三合一的干员名
    std::string m_last_bought_name;                     // `Gt`：最近一次买到的干员名
    std::unordered_set<std::string> m_filler_units;     // `It`：临时补位（招第 9 名前会卖掉）
    std::unordered_set<std::string> m_bought_operators; // `Nr` / `ke`：本局买过的干员
    PrepTrackState m_prep_track;                        // `e.std`
    int m_prep_track_round = -1;                        // `Ce` 在回合一变就重置
    std::unordered_map<std::string, std::vector<uint8_t>> m_equip_patch_cache; // `xt`
    struct EquipNameCache
    {
        std::vector<uint8_t> patch;
        std::string name;
    };
    std::unordered_map<std::string, EquipNameCache> m_equip_name_cache; // `cr`
    std::vector<EquipCell> m_overflow_equip_units;                       // `wr`
    bool m_equip_drag_missed = false;                                    // `Ar`
    std::unordered_map<std::string, int> m_unit_equipped_count;
    bool m_mob_used_this_round = false;
    int m_pager_used_round = -1;
    int m_pager_uses = 0;        // `vt`：本局剩余可用寻呼次数
    int m_pager_round = -1;      // `Kt`：上一次发放寻呼次数的回合
    bool m_pager_started = false; // `dr`：是否已过首次引导
    std::optional<std::pair<int, int>> m_pager_target_cell; // 调试用：上次寻呼目标格

    // Strategy & Task Completion Progress
    std::string m_current_strategy;
    std::string m_current_strategy_id;
    std::set<std::string> m_done_strategies;
    std::map<std::string, int> m_strategy_clears;
    struct SeenStrategyIcon
    {
        std::string name;
        cv::Mat icon;
    };
    std::vector<SeenStrategyIcon> m_seen_strategy_icons;
    int m_runs_completed = 0;
    int m_attempts_done = 0;
    int m_medals_earned = 0;

    void reset_match_state();
};

} // namespace asst