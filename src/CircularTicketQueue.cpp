#include "CircularTicketQueue.h"

bool CircularTicketQueue::enqueue(const RepairTicket& ticket) {
    if (isFull()) {
        return false;
    }

    tickets_[rear_] = ticket;
    // Modulo wraps the insertion index to slot zero, reusing space freed at the front.
    rear_ = (rear_ + 1) % Capacity;
    ++size_;
    return true;
}

bool CircularTicketQueue::dequeue(RepairTicket& servedTicket) {
    if (isEmpty()) {
        return false;
    }

    servedTicket = tickets_[front_];
    // front_ advances independently of rear_; modulo keeps both indices in array bounds.
    front_ = (front_ + 1) % Capacity;
    --size_;
    return true;
}

bool CircularTicketQueue::peek(RepairTicket& nextTicket) const {
    if (isEmpty()) {
        return false;
    }
    nextTicket = tickets_[front_];
    return true;
}

bool CircularTicketQueue::isEmpty() const noexcept { return size_ == 0; }
bool CircularTicketQueue::isFull() const noexcept { return size_ == Capacity; }
std::size_t CircularTicketQueue::size() const noexcept { return size_; }
std::size_t CircularTicketQueue::frontIndex() const noexcept { return front_; }
std::size_t CircularTicketQueue::rearIndex() const noexcept { return rear_; }

std::array<std::string, CircularTicketQueue::Capacity> CircularTicketQueue::slotTicketIds() const {
    std::array<std::string, Capacity> result{};
    for (std::size_t offset = 0; offset < size_; ++offset) {
        const auto index = (front_ + offset) % Capacity;
        result[index] = tickets_[index].id;
    }
    return result;
}

std::vector<RepairTicket> CircularTicketQueue::snapshot() const {
    std::vector<RepairTicket> result;
    result.reserve(size_);
    // Walk only the occupied logical range; wrapping here preserves FIFO order.
    for (std::size_t offset = 0; offset < size_; ++offset) {
        result.push_back(tickets_[(front_ + offset) % Capacity]);
    }
    return result;
}
