#include "towerdefense/ResourceManager.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace towerdefense {

ResourceManager::ResourceManager(Materials initial, Materials passive_income, int passive_interval_ticks)
    : materials_(std::move(initial))
    , passive_income_(std::move(passive_income))
    , passive_interval_ticks_(std::max(1, passive_interval_ticks))
    , ticks_until_income_(passive_interval_ticks_) {}

void ResourceManager::tick(int wave_index) {
    if (--ticks_until_income_ <= 0) {
        ticks_until_income_ = passive_interval_ticks_;
        materials_.add(passive_income_);
        push_transaction(passive_income_, TransactionKind::PassiveIncome, "Passive income", wave_index);
    }
}

bool ResourceManager::spend(const Materials& cost, std::string_view reason, int wave_index) {
    if (!materials_.consume_if_possible(cost)) {
        return false;
    }
    push_transaction(cost, TransactionKind::Spend, std::string(reason), wave_index);
    return true;
}

bool ResourceManager::spend_for_ability(const Materials& cost, std::string_view ability_name, int wave_index) {
    if (!materials_.consume_if_possible(cost)) {
        return false;
    }
    push_transaction(cost, TransactionKind::Ability, std::string(ability_name), wave_index);
    return true;
}

void ResourceManager::income(const Materials& amount, std::string_view source, int wave_index) {
    materials_.add(amount);
    push_transaction(amount, TransactionKind::Income, std::string(source), wave_index);
}

void ResourceManager::refund(const Materials& amount, std::string_view source, int wave_index) {
    materials_.add(amount);
    push_transaction(amount, TransactionKind::Refund, std::string(source), wave_index);
}

void ResourceManager::steal(const Materials& amount, std::string_view source, int wave_index) {
    Materials actual_loss{
        std::min(materials_.wood(), amount.wood()),
        std::min(materials_.stone(), amount.stone()),
        std::min(materials_.crystal(), amount.crystal())};
    if (actual_loss.wood() > 0 || actual_loss.stone() > 0 || actual_loss.crystal() > 0) {
        materials_.consume_if_possible(actual_loss);
        push_transaction(actual_loss, TransactionKind::Theft, std::string(source), wave_index);
    }
}

void ResourceManager::award_wave_income(int wave_index, bool flawless, bool early_call) {
    Materials base_income{2, 1, 1};
    Materials reward = base_income;
    if (flawless) {
        reward.add(Materials{1, 1, 1});
    }
    if (early_call) {
        reward.add(Materials{1, 0, 1});
    }
    income(reward, "Wave income", wave_index);
    last_wave_income_ = WaveIncomeSummary{wave_index, reward, flawless, early_call};
}

std::optional<ResourceManager::WaveIncomeSummary> ResourceManager::last_wave_income() const {
    return last_wave_income_;
}

void ResourceManager::set_upcoming_requirement(Materials requirement, std::string description) {
    if ((requirement.wood() == 0 && requirement.stone() == 0 && requirement.crystal() == 0) || description.empty()) {
        upcoming_requirement_.reset();
        return;
    }
    upcoming_requirement_ = std::make_pair(std::move(requirement), std::move(description));
}

std::optional<std::pair<Materials, std::string>> ResourceManager::upcoming_requirement() const {
    return upcoming_requirement_;
}

void ResourceManager::push_transaction(const Materials& delta, TransactionKind kind, std::string description, int wave_index) {
    transactions_.push_front(Transaction{kind, delta, std::move(description), wave_index});
    if (transactions_.size() > max_transactions_) {
        transactions_.pop_back();
    }
}

double ResourceManager::passive_progress() const noexcept {
    const double progress = 1.0 - static_cast<double>(ticks_until_income_) / static_cast<double>(passive_interval_ticks_);
    return std::clamp(progress, 0.0, 1.0);
}

} // namespace towerdefense

