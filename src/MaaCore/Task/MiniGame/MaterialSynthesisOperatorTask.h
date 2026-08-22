#pragma once

#include "Task/Infrast/InfrastProductionTask.h"

#include <optional>
#include <string>
#include <vector>

namespace asst
{
class MaterialSynthesisOperatorTask final : public InfrastProductionTask
{
public:
    MaterialSynthesisOperatorTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
    virtual ~MaterialSynthesisOperatorTask() override = default;

    bool select_operator(const std::string& material_id, int material_level);

    virtual std::string facility_name() const override { return "Processing"; }

protected:
    virtual bool _run() override { return false; }

private:
    bool rebuild_cache();
    std::optional<size_t> scan_current_page(std::vector<infrast::Oper>& seen_operators);
    std::optional<size_t> find_best_operator(const std::string& material_id, int material_level) const;
    bool locate_and_select(const infrast::Oper& target);
    bool review_selection(const infrast::Oper& target);
    bool confirm_selection();
    bool is_operator_list() const;
    void invalidate_cache();

    static bool same_cached_operator(const infrast::Oper& lhs, const infrast::Oper& rhs, int face_hash_threshold);
    static bool avatar_matches(const infrast::Oper& lhs, const infrast::Oper& rhs, int face_hash_threshold);

    std::vector<infrast::Oper> m_operator_cache;
    bool m_cache_valid = false;
};
} // namespace asst
