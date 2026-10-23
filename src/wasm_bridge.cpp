#include "RepairWorkshop.h"

#include <emscripten/bind.h>

#include <string>
#include <vector>

namespace {
using emscripten::val;

struct TicketData {
    std::string id;
    std::string customerName;
    std::string deviceName;
    std::string issueDescription;
};

struct QueueState {
    std::vector<std::string> slots;
    std::vector<TicketData> waiting;
    std::size_t frontIndex;
    std::size_t rearIndex;
    std::size_t size;
    std::size_t capacity;
    std::size_t completedJobs;
    bool full;
};

struct RegistrationResult {
    bool success;
    std::string ticketId;
    std::string customerError;
    std::string deviceError;
    std::string issueError;
    std::string message;
};

TicketData toTicket(const RepairTicket& ticket) {
    return {ticket.id, ticket.customerName, ticket.deviceName, ticket.issueDescription};
}

class WorkshopBridge {
public:
    RegistrationResult registerTicket(const std::string& customer,
                                      const std::string& device,
                                      const std::string& issue) {
        const auto validation = validateTicketFields(customer, device, issue);
        if (!validation.valid()) {
            return {false, {}, validation.customerError, validation.deviceError,
                    validation.issueError, "Correct the highlighted fields and try again."};
        }
        std::string id;
        if (!workshop_.registerTicket(customer, device, issue, id)) {
            return {false, {}, {}, {}, {}, "The waiting queue is full. Serve a ticket before registering another."};
        }
        return {true, id, {}, {}, {}, "Ticket " + id + " added to the waiting queue."};
    }

    val serveNext() {
        RepairTicket ticket;
        val result = val::object();
        if (!workshop_.serveNextTicket(ticket)) {
            result.set("success", false);
            result.set("message", "There are no waiting tickets to serve.");
            return result;
        }
        result.set("success", true);
        result.set("ticket", toTicket(ticket));
        result.set("message", "Ticket " + ticket.id + " completed.");
        return result;
    }

    QueueState state() const {
        const auto view = workshop_.queueView();
        std::vector<TicketData> waiting;
        for (const auto& ticket : workshop_.waitingTickets()) waiting.push_back(toTicket(ticket));
        return {view.slotTicketIds, waiting, view.frontIndex, view.rearIndex,
                view.size, CircularTicketQueue::Capacity, workshop_.completedJobs(),
                view.size == CircularTicketQueue::Capacity};
    }

private:
    RepairWorkshop workshop_;
};
}

EMSCRIPTEN_BINDINGS(mendoraq_bridge) {
    using namespace emscripten;
    value_object<TicketData>("Ticket")
        .field("id", &TicketData::id)
        .field("customerName", &TicketData::customerName)
        .field("deviceName", &TicketData::deviceName)
        .field("issueDescription", &TicketData::issueDescription);
    value_object<RegistrationResult>("RegistrationResult")
        .field("success", &RegistrationResult::success)
        .field("ticketId", &RegistrationResult::ticketId)
        .field("customerError", &RegistrationResult::customerError)
        .field("deviceError", &RegistrationResult::deviceError)
        .field("issueError", &RegistrationResult::issueError)
        .field("message", &RegistrationResult::message);
    value_object<QueueState>("QueueState")
        .field("slots", &QueueState::slots)
        .field("waiting", &QueueState::waiting)
        .field("frontIndex", &QueueState::frontIndex)
        .field("rearIndex", &QueueState::rearIndex)
        .field("size", &QueueState::size)
        .field("capacity", &QueueState::capacity)
        .field("completedJobs", &QueueState::completedJobs)
        .field("full", &QueueState::full);
    register_vector<std::string>("StringVector");
    register_vector<TicketData>("TicketVector");
    class_<WorkshopBridge>("Workshop")
        .constructor<>()
        .function("registerTicket", &WorkshopBridge::registerTicket)
        .function("serveNext", &WorkshopBridge::serveNext)
        .function("state", &WorkshopBridge::state);
}
