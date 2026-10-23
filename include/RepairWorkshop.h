#pragma once

#include "CircularTicketQueue.h"

#include <cstddef>
#include <string>
#include <vector>

struct TicketValidation {
    std::string customerName;
    std::string deviceName;
    std::string issueDescription;
    std::string customerError;
    std::string deviceError;
    std::string issueError;
    bool valid() const noexcept;
};

struct TicketQueueView {
    std::vector<std::string> slotTicketIds; // Eight entries; empty string means vacant.
    std::size_t frontIndex = 0;
    std::size_t rearIndex = 0; // Next insertion slot, which may be occupied when full.
    std::size_t size = 0;
};

// Shared policy used by both the GUI and workshop API. Successful output is trimmed.
TicketValidation validateTicketFields(const std::string& customerName,
                                      const std::string& deviceName,
                                      const std::string& issueDescription);

class RepairWorkshop {
public:
    // On success, writes the assigned ID. Invalid fields or a full queue fail
    // without changing the output ID, queue, or ID sequence.
    bool registerTicket(const std::string& customerName,
                        const std::string& deviceName,
                        const std::string& issueDescription,
                        std::string& assignedTicketId);
    // A successful service removes one FIFO ticket and increments completedJobs().
    bool serveNextTicket(RepairTicket& servedTicket);
    std::vector<RepairTicket> waitingTickets() const;
    TicketQueueView queueView() const;
    std::size_t completedJobs() const noexcept;

private:
    std::string nextTicketId() const;

    CircularTicketQueue queue_;
    std::size_t nextId_ = 1;
    std::size_t completedJobs_ = 0;
};
