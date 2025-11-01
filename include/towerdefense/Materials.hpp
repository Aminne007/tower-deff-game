#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

namespace towerdefense {

enum class MaterialType {
    Wood = 0,
    Stone = 1,
    Crystal = 2
};

class Materials {
public:
    Materials() = default;
    Materials(int wood, int stone, int crystal)
        : storage_{wood, stone, crystal} {}

    [[nodiscard]] int wood() const noexcept { return storage_[0]; }
    [[nodiscard]] int stone() const noexcept { return storage_[1]; }
    [[nodiscard]] int crystal() const noexcept { return storage_[2]; }

    void add(MaterialType type, int amount);
    bool consume_if_possible(const Materials& cost);
    void add(const Materials& other);

    [[nodiscard]] std::string to_string() const;

    [[nodiscard]] const std::array<int, 3>& data() const noexcept { return storage_; }

private:
    std::array<int, 3> storage_{0, 0, 0};
};

[[nodiscard]] std::string_view to_string(MaterialType type) noexcept;

} // namespace towerdefense

