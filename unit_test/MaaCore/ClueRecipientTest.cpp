#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>

#include "Task/Infrast/ClueRecipient.h"

TEST_CASE("Unreadable target rows cannot count as a complete unsuccessful search", "[clue-recipient]")
{
    using asst::infrast::ClueRecipientPage;
    using RowState = ClueRecipientPage::RowState;
    ClueRecipientPage page;
    page.full_names[1] = "Other#1234";
    page.page_keys[1] = "Other#1234";
    page.states = { RowState::Unreadable, RowState::Readable, RowState::Empty, RowState::Empty };
    REQUIRE_FALSE(page.can_continue_search());
    REQUIRE_FALSE(asst::infrast::find_clue_recipient(page.full_names, "Doctor#5678"));
    asst::infrast::ClueRecipientSearchAttempts attempts;
    for (int retry = 0; retry < 2; ++retry) {
        if (page.can_continue_search()) {
            attempts.record_not_found();
        }
    }
    REQUIRE(attempts.not_found_count() == 0);
}

TEST_CASE("Only reliably different nicknames exclude an incomplete clue recipient", "[clue-recipient]")
{
    using asst::infrast::can_exclude_clue_recipient;
    REQUIRE(can_exclude_clue_recipient("Other#12O", "DoctorLong#5678"));
    REQUIRE_FALSE(can_exclude_clue_recipient("DoctorLong#567", "DoctorLong#5678"));
    REQUIRE_FALSE(can_exclude_clue_recipient("Doctor#5678", "DoctorLong#5678"));
    REQUIRE_FALSE(can_exclude_clue_recipient("DoctorLongk#5678", "DoctorLong#5678"));
    REQUIRE_FALSE(can_exclude_clue_recipient("DoctorLong", "DoctorLong#5678"));
    REQUIRE_FALSE(can_exclude_clue_recipient("#5678", "DoctorLong#5678"));
}

TEST_CASE("Complete discriminators distinguish friends with the same nickname", "[clue-recipient]")
{
    using asst::infrast::can_exclude_clue_recipient;
    REQUIRE(can_exclude_clue_recipient("Doctor#4321", "Doctor#1234"));
    REQUIRE_FALSE(can_exclude_clue_recipient("Doctor#1234", "Doctor#1234"));
    REQUIRE_FALSE(can_exclude_clue_recipient("Doctor#123", "Doctor#1234"));
    REQUIRE_FALSE(can_exclude_clue_recipient("Doctor#12O4", "Doctor#1234"));
    REQUIRE_FALSE(can_exclude_clue_recipient("Doctor#1234", "DoctorLong#1234"));
    REQUIRE_FALSE(can_exclude_clue_recipient("DoctorLongk#1234", "DoctorLong#1234"));
    REQUIRE(can_exclude_clue_recipient("Other#12O", "Doctor#1234"));
}

TEST_CASE("Only stable empty tails allow clue recipient pagination", "[clue-recipient]")
{
    using asst::infrast::ClueRecipientPage;
    using RowState = ClueRecipientPage::RowState;
    ClueRecipientPage page;
    page.page_keys[0] = "Other#12O";
    page.states = { RowState::Readable, RowState::Empty, RowState::Empty, RowState::Empty };
    REQUIRE(page.can_continue_search());
    REQUIRE(page.has_empty_rows());
    REQUIRE(page == ClueRecipientPage(page));
    auto changed = page;
    changed.states[1] = RowState::Unreadable;
    REQUIRE_FALSE(page == changed);
    REQUIRE_FALSE(changed.can_continue_search());
    page.states[2] = RowState::Readable;
    REQUIRE_FALSE(page.can_continue_search());
    REQUIRE_FALSE(ClueRecipientPage().can_continue_search());
    page.states.fill(RowState::Empty);
    REQUIRE_FALSE(page.can_continue_search());
}

TEST_CASE("A complete unique clue recipient can be found despite unrelated unreadable rows", "[clue-recipient]")
{
    asst::infrast::ClueRecipientPage page;
    page.full_names[0] = "Doctor#5678";
    page.states[0] = asst::infrast::ClueRecipientPage::RowState::Readable;
    REQUIRE(asst::infrast::find_clue_recipient(page.full_names, "Doctor#5678") == 0);
    REQUIRE_FALSE(page.can_continue_search());
    page.full_names[2] = "Doctor#5678";
    REQUIRE_FALSE(asst::infrast::find_clue_recipient(page.full_names, "Doctor#5678"));
}

TEST_CASE("Skipping clue gifts survives a Reception retry but not a new task", "[clue-recipient]")
{
    asst::infrast::ClueRecipientRunState state;
    state.begin_run(false);
    REQUIRE(state.can_send());
    state.skip();
    state.begin_run(true);
    REQUIRE_FALSE(state.can_send());
    state.use_default();
    REQUIRE_FALSE(state.can_send());
    REQUIRE_FALSE(state.is_using_default());
    state.begin_run(false);
    REQUIRE(state.can_send());
    REQUIRE_FALSE(state.is_using_default());
}

TEST_CASE("The default clue strategy survives a Reception retry but not a new task", "[clue-recipient]")
{
    asst::infrast::ClueRecipientRunState state;
    state.use_default();
    state.begin_run(true);
    REQUIRE(state.can_send());
    REQUIRE(state.is_using_default());
    state.begin_run(false);
    REQUIRE(state.can_send());
    REQUIRE_FALSE(state.is_using_default());
}

TEST_CASE("Clue recipients use the default strategy only after two failed searches", "[clue-recipient]")
{
    asst::infrast::ClueRecipientSearchAttempts attempts;
    using Action = asst::infrast::ClueRecipientSearchAttempts::Action;
    REQUIRE(attempts.record_not_found() == Action::Retry);
    REQUIRE(attempts.not_found_count() == 1);
    REQUIRE(attempts.record_not_found() == Action::UseDefault);
    REQUIRE(attempts.not_found_count() == 2);
    REQUIRE(attempts.record_not_found() == Action::UseDefault);
    REQUIRE(attempts.not_found_count() == 2);
}

TEST_CASE("Finding the clue recipient resets consecutive failed searches", "[clue-recipient]")
{
    asst::infrast::ClueRecipientSearchAttempts attempts;
    using Action = asst::infrast::ClueRecipientSearchAttempts::Action;
    REQUIRE(attempts.record_not_found() == Action::Retry);
    attempts.reset();
    REQUIRE(attempts.not_found_count() == 0);
    REQUIRE(attempts.record_not_found() == Action::Retry);
    REQUIRE(attempts.not_found_count() == 1);
    REQUIRE(attempts.record_not_found() == Action::UseDefault);
}

TEST_CASE("Clue recipient fallback does not carry over to a new search instance", "[clue-recipient]")
{
    asst::infrast::ClueRecipientSearchAttempts previous;
    using Action = asst::infrast::ClueRecipientSearchAttempts::Action;
    REQUIRE(previous.record_not_found() == Action::Retry);
    REQUIRE(previous.record_not_found() == Action::UseDefault);
    asst::infrast::ClueRecipientSearchAttempts next;
    REQUIRE(next.record_not_found() == Action::Retry);
    REQUIRE(next.not_found_count() == 1);
}

TEST_CASE("Only recipient send buttons trigger the pre-send guard", "[clue-recipient]")
{
    using asst::infrast::clue_recipient_send_row;
    for (size_t row = 0; row < 4; ++row) {
        const auto task = "InfrastClueSendToRecipient" + std::to_string(row + 1);
        REQUIRE(clue_recipient_send_row(task) == row);
        REQUIRE_FALSE(clue_recipient_send_row(task + "@LoadingText"));
    }
    REQUIRE_FALSE(clue_recipient_send_row("InfrastClueSendToNamedRecipient"));
    REQUIRE_FALSE(clue_recipient_send_row("InfrastClueSendToRecipient0"));
    REQUIRE_FALSE(clue_recipient_send_row("InfrastClueSendToRecipient5"));
    REQUIRE_FALSE(clue_recipient_send_row("InfrastClueSendToRecipient"));
}

TEST_CASE("Clue recipients require a complete player name", "[clue-recipient]")
{
    using asst::infrast::is_valid_clue_recipient;
    REQUIRE(is_valid_clue_recipient("博士#0123"));
    REQUIRE(is_valid_clue_recipient("Doctor#0000"));
    REQUIRE(is_valid_clue_recipient("Doctor#9999"));
    REQUIRE_FALSE(is_valid_clue_recipient(""));
    REQUIRE_FALSE(is_valid_clue_recipient("Doctor"));
    REQUIRE_FALSE(is_valid_clue_recipient("#1234"));
    REQUIRE_FALSE(is_valid_clue_recipient("Doctor#123"));
    REQUIRE_FALSE(is_valid_clue_recipient("Doctor#12345"));
    REQUIRE_FALSE(is_valid_clue_recipient("Doctor#１２３４"));
    REQUIRE_FALSE(is_valid_clue_recipient("Doctor#12O4"));
    REQUIRE_FALSE(is_valid_clue_recipient("Doctor\n#1234"));
}

TEST_CASE("Clue recipient matching distinguishes names and discriminators", "[clue-recipient]")
{
    const std::array<std::string, 4> names { "Doctor#1234", "Doctor#4321", "MyDoctor#1234", "博士#0123" };
    using asst::infrast::find_clue_recipient;
    REQUIRE(find_clue_recipient(names, "Doctor#1234") == 0);
    REQUIRE(find_clue_recipient(names, "Doctor#4321") == 1);
    REQUIRE(find_clue_recipient(names, "MyDoctor#1234") == 2);
    REQUIRE(find_clue_recipient(names, "博士#0123") == 3);
    REQUIRE_FALSE(find_clue_recipient(names, "Doctor"));
    REQUIRE_FALSE(find_clue_recipient(names, "doctor#1234"));
    REQUIRE_FALSE(find_clue_recipient(names, "Doctor#5678"));
    REQUIRE_FALSE(find_clue_recipient(names, ".*#1234"));
}

TEST_CASE("Ambiguous or unreadable clue recipients never select a row", "[clue-recipient]")
{
    using asst::infrast::find_clue_recipient;
    const std::array<std::string, 4> duplicate { "Doctor#1234", "", "Doctor#1234", "" };
    REQUIRE_FALSE(find_clue_recipient(duplicate, "Doctor#1234"));
    const std::array<std::string, 4> unreadable {};
    REQUIRE_FALSE(find_clue_recipient(unreadable, "Doctor#1234"));
    const std::array<std::string, 4> partial { "", "", "博士#0123", "" };
    REQUIRE(find_clue_recipient(partial, "博士#0123") == 2);
}
