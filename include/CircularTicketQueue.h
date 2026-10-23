#pragma once

#include "RepairTicket.h"

#include <array>
#include <cstddef>
#include <vector>
#include <string>

// Fixed-capacity FIFO queue. size_ disambiguates full and empty states;
// while occupied, rear_ is the next slot after the logical last ticket.
class CircularTicketQueue {
public:
    static constexpr std::size_t Capacity = 8;

    // Failed operations return false and leave both queue state and output unchanged.
    bool enqueue(const RepairTicket& ticket);
    bool dequeue(RepairTicket& servedTicket);
    bool peek(RepairTicket& nextTicket) const;
    bool isEmpty() const noexcept;
    bool isFull() const noexcept;
    std::size_t size() const noexcept;
    std::size_t frontIndex() const noexcept;
    std::size_t rearIndex() const noexcept;
    std::array<std::string, Capacity> slotTicketIds() const;

    // Returns tickets from front to back without changing queue state.
    std::vector<RepairTicket> snapshot() const;

private:
    std::array<RepairTicket, Capacity> tickets_{};
    std::size_t front_ = 0;
    std::size_t rear_ = 0;
    std::size_t size_ = 0;
};
