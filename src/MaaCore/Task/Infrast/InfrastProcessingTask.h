#pragma once
#include "InfrastProductionTask.h"

#include <optional>
#include <string>
#include <vector>

namespace asst
{
class InfrastProcessingTask final : public InfrastProductionTask
{
public:
    using InfrastProductionTask::InfrastProductionTask;
    virtual ~InfrastProcessingTask() override = default;

    // 材料合成使用独立实例调用此入口，不读写常规基建排班配置和缓存。
    bool select_operator(const std::string& material_id, int material_level);

protected:
    virtual bool _run() override;

private:
    struct MaterialSynthesisScanResult
    {
        size_t skilled_operators = 0;
        size_t new_candidates = 0;
    };

    bool rebuild_material_synthesis_cache();
    std::optional<MaterialSynthesisScanResult> scan_material_synthesis_page();
    std::optional<size_t>
        find_best_material_synthesis_operator(const std::string& material_id, int material_level) const;
    bool locate_and_select_material_synthesis_operator(const infrast::Oper& target);
    bool review_material_synthesis_selection(const infrast::Oper& target);
    bool confirm_material_synthesis_selection();
    bool is_material_synthesis_operator_list() const;
    void invalidate_material_synthesis_cache();

    static bool
        same_material_synthesis_operator(const infrast::Oper& lhs, const infrast::Oper& rhs, int face_hash_threshold);
    static bool
        material_synthesis_avatar_matches(const infrast::Oper& lhs, const infrast::Oper& rhs, int face_hash_threshold);

    std::vector<infrast::Oper> m_material_synthesis_operator_cache;
    bool m_material_synthesis_cache_valid = false;
};
}
