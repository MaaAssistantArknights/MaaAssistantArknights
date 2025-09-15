#pragma once

#include "Common/AsstBattleDef.h"
#include "Task/AbstractTask.h"

namespace asst
{
class SupportList
{
public:
    using Role = battle::Role;
    using SupportUnit = battle::SupportUnit;

    SupportList(Assistant* inst, AbstractTask& task);
    virtual ~SupportList() = default;

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
    bool use_support_unit();

    /// <summary>
    /// 退出助战干员详情界面。
    /// </summary>
    /// <returns>
    /// 若操作成功，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    bool leave_support_unit_detail_panel();

    /// <summary>
    /// 点选助战干员，判断技能是否为专三，并使用其 skill 技能；
    /// 若 <c>skill == 0</c> 则忽视 <c>max_spec_lvl</c>，并使用 support_unit 的默认技能。
    /// </summary>
    /// <param name="index">目标助战干员所在助战栏位的索引。</param>
    /// <param name="skill">目标技能。</param>
    /// <param name="max_spec_lvl">是否要求技能专三。</param>
    /// <returns>
    /// 若操作成功，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    /// <remarks>
    /// 默认目标技能存在且已解锁。
    /// </remarks>
    bool use_support_unit(size_t index, int skill = 0, bool max_spec_lvl = true);

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

    /// <summary>
    /// 重置已记录的助战列表信息。
    /// </summary>
    /// <param name="reset_selected_role">是否重置 <c>m_selected_role</c>。</param>
    void clear(bool reset_selected_role = false);

    [[nodiscard]] const std::vector<SupportUnit>& get_list() const { return m_list; };

    [[nodiscard]] size_t size() const { return m_list.size(); };

    [[nodiscard]] const SupportUnit& operator[](const size_t index) const { return m_list.at(index); };

    [[nodiscard]] const SupportUnit& at(const size_t index) const { return m_list.at(index); };

private:
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
    std::optional<unsigned> update_page();

    void move_to_list_head();
    void move_forward(unsigned num_pages = 1);
    void move_backward(unsigned num_pages = 1);
    void move_to_support_unit(size_t index);

    InstHelper m_inst_helper;
    AbstractTask& m_task;

    /// <summary>
    /// 当前助战列表所选职业。
    /// </summary>
    Role m_selected_role = Role::Unknown;

    /// <summary>
    /// 当前助战列表，一般情况下为完整的 1–9 号助战栏位。
    /// </summary>
    std::vector<SupportUnit> m_list;

    /// <summary>
    /// 干员名到 <c>m_list</c> 索引的映射。
    /// </summary>
    /// <remarks>
    /// 基于“每位干员在当前助战列表中最多出现一次”的假设。
    /// </remarks>
    std::unordered_map<std::string, size_t> m_name2index;

    /// <summary>
    /// 当前助战列表视图起始位置。视图内助战栏位的索引范围为 <c>m_view_begin</c>–<c>m_view_end - 1</c>。
    /// </summary>
    /// <remarks>
    /// 一般情况下助战列表视图为 1–8 或 2–9 号助战栏位 (索引范围为 0–7 或 1–8)，仅视图内的助战栏位的 <c>rect</c> 为 up-to-date。
    /// </remarks>
    size_t m_view_begin = 0;

    /// <summary>
    /// 当前助战列表视图结束位置。视图内助战栏位的索引范围为 <c>m_view_begin</c>–<c>m_view_end - 1</c>。
    /// </summary>
    /// <remarks>
    /// 一般情况下助战列表视图为 1–8 或 2–9 号助战栏位 (索引范围为 0–7 或 1–8)，仅视图内的助战栏位的 <c>rect</c> 为 up-to-date。
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
    static constexpr unsigned MAX_NUM_PAGES = 2;

    /// <summary>
    /// 助战干员可选职业。
    /// </summary>
    static constexpr std::array<Role, 8> SUPPORT_UNIT_ROLES = { Role::Pioneer, Role::Warrior, Role::Tank,
                                                                Role::Sniper,  Role::Caster,  Role::Medic,
                                                                Role::Support, Role::Special };
};
} // namespace asst
