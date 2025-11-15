#pragma once

#include "Materials.hpp"

#include <deque>
#include <optional>
#include <string>
#include <string_view>

namespace towerdefense {

class ResourceManager {
public:
    enum class TransactionKind {
        Income,
        Spend,
        Refund,
        PassiveIncome,
        Theft,
        Ability
    };

    struct Transaction {
        TransactionKind kind{TransactionKind::Income};
        Materials delta{};
        std::string description{};
        int wave_index{-1};
    };

    struct WaveIncomeSummary {
        int wave_index{-1};
        Materials income{};
        bool flawless{false};
        bool early_call{false};
    };

    ResourceManager(Materials initial, Materials passive_income, int passive_interval_ticks);

    void tick(int wave_index);
    [[nodiscard]] const Materials& materials() const noexcept { return materials_; }

    bool spend(const Materials& cost, std::string_view reason, int wave_index);
    void income(const Materials& amount, std::string_view source, int wave_index);
    void refund(const Materials& amount, std::string_view source, int wave_index);
    void steal(const Materials& amount, std::string_view source, int wave_index);
    bool spend_for_ability(const Materials& cost, std::string_view ability_name, int wave_index);

    void award_wave_income(int wave_index, bool flawless, bool early_call);
    [[nodiscard]] std::optional<WaveIncomeSummary> last_wave_income() const;

    [[nodiscard]] const std::deque<Transaction>& transactions() const noexcept { return transactions_; }

    void set_upcoming_requirement(Materials requirement, std::string description);
    [[nodiscard]] std::optional<std::pair<Materials, std::string>> upcoming_requirement() const;

    [[nodiscard]] double passive_progress() const noexcept;

private:
    Materials materials_;
    Materials passive_income_;
    int passive_interval_ticks_{};
    int ticks_until_income_{};

    std::deque<Transaction> transactions_{};
    static constexpr std::size_t max_transactions_ = 12;

    std::optional<WaveIncomeSummary> last_wave_income_{};
    std::optional<std::pair<Materials, std::string>> upcoming_requirement_{};

    void push_transaction(const Materials& delta, TransactionKind kind, std::string description, int wave_index);
};

} // namespace towerdefense

