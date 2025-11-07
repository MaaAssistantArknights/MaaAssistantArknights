#pragma once

#include "Common/AsstBattleDef.h"
#include "Task/AbstractTask.h"

namespace asst
{
class SupportList : public AbstractTask
{
public:
    using Role = battle::Role;
    using OperModule = battle::OperModule;
    using Friendship = battle::Friendship;
    using SupportUnit = battle::SupportUnit;

    using AbstractTask::AbstractTask;
    virtual ~SupportList() override = default;

    virtual bool _run() override { return true; };

    /// <summary>
    /// 选择助战列表职业。
    /// </summary>
    /// <param name="role">目标职业。</param>
    /// <returns>
    /// 若操作成功，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    /// <remarks>
    /// 同时更新 <c>m_selected_role</c>，并重置除 <c>m_selected_role</c> 外已记录的助战列表信息。
    /// </remarks>
    bool select_role(Role role);

    /// <summary>
    /// 从左往右依次获取助战列表页截图并以此更新助战列表信息记录。
    /// </summary>
    /// <returns>
    /// 若操作成功，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    bool update();

    /// <summary>
    /// 选择助战干员以展开其详情界面。
    /// </summary>
    /// <param name="index">目标助战干员所在助战栏位的索引。</param>
    /// <returns>
    /// 若操作成功，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    bool select_support_unit(size_t index);

    /// <summary>
    /// 招募当前详情界面所属的助战干员。
    /// </summary>
    /// <returns>
    /// 若操作成功，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    bool confirm_to_use_support_unit();

    /// <summary>
    /// 退出助战干员详情界面。
    /// </summary>
    /// <returns>
    /// 若操作成功，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    bool leave_support_unit_detail_panel();

    /// <summary>
    /// 助战干员详情界面识别 <c>skill</c> 技能的等级，若不小于 <c>minimum_skill_level</c> 则选择该技能。
    /// </summary>
    /// <param name="skill">目标技能，要求为 1–3 的整数。</param>
    /// <param name="minimum_skill_level">技能等级下限。</param>
    /// <returns>
    /// 若技能等级达标且成功选择技能，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    bool select_skill(int skill, int minimum_skill_level = 0);

    /// <summary>
    /// 助战干员详情界面识别 <c>module</c> 模组的等级，若不小于 <c>minimum_module_level</c> 则选择该模组。
    /// </summary>
    /// <param name="module">目标模组；不可为 <c>OperModule::Unspecified</c>。</param>
    /// <param name="minimum_module_level">模组等级下限；仅当 <c>module != OperModule::Original</c> 时有效。</param>
    /// <returns>
    /// 若模组等级达标且成功选择模组，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    /// <remarks>
    /// 若当前助战干员未解锁模组系统，即 <c>support_unit.module_enabled == false</c>，
    /// 则即便只选择 <c>OperModule::Original</c> 也不会成功。
    /// </remarks>
    bool select_module(OperModule module, int minimum_module_level = 0);

    /// <summary>
    /// 刷新助战列表。
    /// </summary>
    /// <returns>
    /// 若操作成功，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    /// <remarks>
    /// 同时重置除 <c>m_selected_role</c> 外已记录的助战列表信息。
    /// </remarks>
    bool refresh_list();

    [[nodiscard]] const std::vector<SupportUnit>& get_list() const { return m_list; };

    [[nodiscard]] size_t size() const { return m_list.size(); };

    [[nodiscard]] const SupportUnit& operator[](const size_t index) const { return m_list.at(index); };

    [[nodiscard]] const SupportUnit& at(const size_t index) const { return m_list.at(index); };

private:
    struct ModuleItem
    {
        Rect rect;
        OperModule module = OperModule::Unspecified;
        int level = 0;
    };

    /// <summary>
    /// 识别 <c>image</c> 中显示的助战列表当前所选的职业，并更新 <c>m_selected_role</c>。
    /// </summary>
    /// <param name="image">助战列表截图。</param>
    /// <returns>
    /// 若成功识别当前所选职业，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    bool update_selected_role(const cv::Mat& image);

    /// <summary>
    /// 获取当前助战列表页截图并以此更新助战列表信息记录。
    /// </summary>
    /// <returns>
    /// 若操作成功，则返回新增助战栏位数量，反之则返回 <c>std::nullopt</c>。
    /// </returns>
    std::optional<size_t> update_page();

    /// <summary>
    /// 重置已记录的助战列表信息。
    /// </summary>
    void reset_list_and_view_data();

    void report_support_list();

    /// <summary>
    /// 根据 support_units_in_view 更新当前助战列表视图，若 <c>support_units_in_view == std::nullopt</c>
    /// 则使用 <c>image</c> 或主动截图，生成助战栏位序列后再更新。
    /// </summary>
    /// <param name="support_units_in_view">当前助战列表视图内的助战栏位序列，仅需要 <c>rect</c> 与
    /// <c>name</c>。</param> <param name="image">当前助战列表视图的截图。</param>
    void update_view(
        const std::optional<std::vector<SupportUnit>>& support_units_in_view = std::nullopt,
        const cv::Mat& image = cv::Mat());

    /// <summary>
    /// 重置视图。
    /// </summary>
    void reset_view();

    void move_to_support_unit(size_t index);
    void move_to_list_head();
    void move_forward();
    void move_backward();

    std::vector<ModuleItem> analyze_module_page();

    /// <summary>
    /// 获取模版名 <c>s</c> 的后缀数字。
    /// </summary>
    /// <param name="s">模版名。</param>
    /// <param name="delimiter">模版名后缀数字前的分隔符。</param>
    /// <returns>
    /// 若成功识别到后缀数字，则返回识别结果，反之则返回 <c>std::nullopt</c>。
    /// </returns>
    /// <example>
    /// <code>
    /// get_suffix_num("SupportList-DetailPanel-SkillLevel-10.png", '-') == 10
    /// </code>
    /// </example>
    [[nodiscard]] static std::optional<int> get_suffix_num(const std::string& s, char delimiter = '-');

    /// <summary>
    /// 获取模版名 <c>s</c> 的后缀子串。
    /// </summary>
    /// <param name="s">模版名。</param>
    /// <param name="delimiter">模版名后缀子串前的分隔符。</param>
    /// <returns>
    /// 后缀子串。
    /// </returns>
    /// <example>
    /// <code>
    /// get_suffix_num("SupportList-DetailPanel-Module-Chi_1.png", '-') == "Chi"
    /// </code>
    /// </example>
    [[nodiscard]] static std::string get_suffix_str(const std::string& s, char delimiter = '-');

    /// <summary>
    /// 将 <c>image</c> 保存于 debug/supportList 目录下，并在输出 "Save {description} to {文件路径}" 的日志。
    /// </summary>
    /// <param name="image">要保存的图像。</param>
    /// <param name="description">填入日志信息中的描述图像的字符串。</param>
    /// <returns>
    /// 若文件写入成功，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    static bool save_img(const cv::Mat& image, std::string_view description = "image");

    /// <summary>
    /// 当前助战列表所选职业。
    /// </summary>
    Role m_selected_role = Role::Unknown;

    /// <summary>
    /// 当前助战列表，一般情况下为完整的 1–9 号助战栏位。
    /// </summary>
    std::vector<SupportUnit> m_list;

    /// <summary>
    /// 干员名称到 <c>m_list</c> 索引的映射。
    /// </summary>
    /// <remarks>
    /// 基于“每位干员在当前助战列表中最多出现一次”的假设。
    /// </remarks>
    std::unordered_map<std::string, size_t> m_name2index;

    /// <summary>
    /// 当前助战列表视图起始位置。视图内助战栏位的索引范围为 <c>m_view_begin</c>–<c>m_view_end - 1</c>。
    /// </summary>
    /// <remarks>
    /// 一般情况下助战列表视图为 1–8 或 2–9 号助战栏位 (索引范围为 0–7 或 1–8)，
    /// 仅视图内的助战栏位的 <c>rect</c> 为 up-to-date。
    /// </remarks>
    size_t m_view_begin = 0;

    /// <summary>
    /// 当前助战列表视图结束位置。视图内助战栏位的索引范围为 <c>m_view_begin</c>–<c>m_view_end - 1</c>。
    /// </summary>
    /// <remarks>
    /// 一般情况下助战列表视图为 1–8 或 2–9 号助战栏位 (索引范围为 0–7 或 1–8)，
    /// 仅视图内的助战栏位的 <c>rect</c> 为 up-to-date。
    /// </remarks>
    size_t m_view_end = 0;

    /// <summary>
    /// 当前是否位于助战干员详情界面。
    /// </summary>
    bool m_in_support_unit_detail_panel = false;

    /// <summary>
    /// 助战列表的总页数。
    /// </summary>
    /// <remarks>
    /// 助战列表共有 9 个栏位，一页即一屏，屏幕上最多只能同时完整显示 8 名助战干员，因而总页数为 2。
    /// 注意助战列表页之间的内容重叠。
    /// </remarks>
    static constexpr size_t MAX_NUM_PAGES = 2;

    /// <summary>
    /// 助战干员可选职业。
    /// </summary>
    static constexpr std::array<Role, 8> SUPPORT_UNIT_ROLES = {
        Role::Pioneer, Role::Warrior, Role::Tank, Role::Sniper, Role::Caster, Role::Medic, Role::Support, Role::Special,
    };

    /// <summary>
    /// 助战干员详情界面中模组列表的总页数。
    /// </summary>
    static constexpr size_t MAX_NUM_MODULE_PAGES = 2;

    /// <summary>
    /// 助战干员可选模组。
    /// </summary>
    static constexpr std::array<OperModule, 8> SUPPORT_UNIT_MODULES = {
        OperModule::Original, OperModule::Chi,   OperModule::Upsilon,
        OperModule::Delta,    OperModule::Alpha, OperModule::Beta,
    };
};
} // namespace asst
