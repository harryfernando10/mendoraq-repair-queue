#include "CircularTicketQueue.h"
#include "RepairWorkshop.h"

#include <functional>
#include <iostream>
#include <string>

namespace {
int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "  FAIL: " << message << '\n';
    }
}

RepairTicket makeTicket(int number) {
    const std::string id = "T" + std::to_string(number);
    return {id, "Customer " + std::to_string(number), "Device", "Issue"};
}

void run(const std::string& name, const std::function<void()>& test) {
    const int before = failures;
    test();
    std::cout << (failures == before ? "[PASS] " : "[FAIL] ") << name << '\n';
}
}

int main() {
    run("empty queue detection and dequeue", [] {
        CircularTicketQueue queue;
        RepairTicket result = makeTicket(99);
        check(queue.isEmpty(), "new queue should be empty");
        check(!queue.dequeue(result), "dequeue from empty queue should fail");
        check(queue.size() == 0, "failed dequeue must preserve size");
        check(result.id == "T99", "failed dequeue must preserve output argument");
    });

    run("single enqueue and dequeue", [] {
        CircularTicketQueue queue;
        RepairTicket result;
        check(queue.enqueue(makeTicket(1)), "enqueue should succeed");
        check(queue.dequeue(result), "dequeue should succeed");
        check(result.id == "T1", "dequeued ticket should match inserted ticket");
        check(queue.isEmpty(), "queue should be empty after dequeue");
    });

    run("FIFO order with multiple tickets", [] {
        CircularTicketQueue queue;
        for (int i = 1; i <= 3; ++i) check(queue.enqueue(makeTicket(i)), "enqueue should succeed");
        const auto tickets = queue.snapshot();
        check(tickets.size() == 3, "snapshot should contain three tickets");
        if (tickets.size() == 3) {
            check(tickets[0].id == "T1" && tickets[1].id == "T2" && tickets[2].id == "T3",
                  "snapshot should be FIFO");
        }
    });

    run("capacity, rejected ninth insertion, and unchanged snapshot", [] {
        CircularTicketQueue queue;
        for (int i = 1; i <= 8; ++i) check(queue.enqueue(makeTicket(i)), "one of eight inserts failed");
        check(queue.isFull() && queue.size() == 8, "queue should be full at eight");
        const auto before = queue.snapshot();
        check(!queue.enqueue(makeTicket(9)), "ninth insert should fail");
        check(queue.snapshot().size() == before.size(), "failed insert changed queue size");
        const auto after = queue.snapshot();
        bool unchanged = after.size() == before.size();
        for (std::size_t i = 0; unchanged && i < before.size(); ++i) {
            unchanged = after[i].id == before[i].id;
        }
        check(unchanged, "failed insert changed FIFO contents");
    });

    run("wraparound order and repeatable snapshots", [] {
        CircularTicketQueue queue;
        for (int i = 1; i <= 8; ++i) queue.enqueue(makeTicket(i));
        RepairTicket result;
        for (int i = 1; i <= 3; ++i) queue.dequeue(result);
        for (int i = 9; i <= 11; ++i) check(queue.enqueue(makeTicket(i)), "wraparound enqueue failed");
        const auto first = queue.snapshot();
        const auto second = queue.snapshot();
        check(first.size() == 8 && second.size() == 8, "snapshot count should remain eight");
        bool orderCorrect = first.size() == 8;
        bool snapshotsMatch = first.size() == second.size();
        for (std::size_t i = 0; i < first.size(); ++i) {
            const int expected = static_cast<int>(i) + 4;
            orderCorrect = orderCorrect && first[i].id == "T" + std::to_string(expected);
            snapshotsMatch = snapshotsMatch && first[i].id == second[i].id;
        }
        check(orderCorrect, "wrapped snapshot should preserve FIFO order T4 through T11");
        check(snapshotsMatch, "repeated snapshot should preserve all queue contents");
        check(queue.size() == 8, "snapshot should preserve queue size");
    });

    run("workshop IDs and whitespace validation", [] {
        RepairWorkshop workshop;
        std::string id;
        check(workshop.registerTicket(" Alex ", "Phone", "Screen", id) && id == "T001", "first ID incorrect");
        check(!workshop.registerTicket(" \t", "Phone", "Screen", id), "blank customer should be rejected");
        check(!workshop.registerTicket("Al", " \t", "Screen", id), "blank device should be rejected");
        check(!workshop.registerTicket("Al", "Phone", " \t", id), "blank issue should be rejected");
        check(workshop.registerTicket("Bea", "Laptop", "Battery", id) && id == "T002",
              "rejected blank registration should not consume an ID");
    });

    run("full workshop registration does not consume an ID", [] {
        RepairWorkshop workshop;
        std::string id;
        for (int i = 0; i < 8; ++i) {
            check(workshop.registerTicket("Customer", "Device", "Issue", id), "registration failed before capacity");
        }
        check(!workshop.registerTicket("Ninth", "Device", "Issue", id), "ninth registration should fail");
        RepairTicket served;
        check(workshop.serveNextTicket(served), "service should free one slot");
        check(workshop.registerTicket("Tenth", "Device", "Issue", id) && id == "T009",
              "rejected full registration must not consume ID");
    });

    run("ticket IDs grow beyond three digits", [] {
        RepairWorkshop workshop;
        std::string id;
        RepairTicket served;
        for (int i = 0; i < 1000; ++i) {
            check(workshop.registerTicket("Customer", "Device", "Issue", id), "registration failed");
            check(workshop.serveNextTicket(served), "service failed");
        }
        check(workshop.registerTicket("Customer", "Device", "Issue", id), "registration after 1000 failed");
        check(id == "T1001", "ID should retain all digits after the minimum three-digit width");
    });

    run("completed count, empty service, and draining", [] {
        RepairWorkshop workshop;
        RepairTicket served;
        check(!workshop.serveNextTicket(served), "empty service should fail");
        check(workshop.completedJobs() == 0, "empty service incremented completed count");
        std::string id;
        for (int i = 0; i < 3; ++i) check(workshop.registerTicket("Customer", "Device", "Issue", id), "registration failed");
        for (int i = 0; i < 3; ++i) check(workshop.serveNextTicket(served), "service failed while draining");
        check(workshop.waitingTickets().empty(), "queue should be empty after drain");
        check(workshop.completedJobs() == 3, "completed count should equal successful services");
    });

    run("field limits, trimming, punctuation, and control characters", [] {
        RepairWorkshop workshop;
        std::string id = "unchanged";
        check(!validateTicketFields("A", "Phone", "Screen issue").valid(), "short customer should fail");
        check(!validateTicketFields("Customer", "X", "Screen issue").valid(), "short device should fail");
        check(!validateTicketFields("Customer", "Phone", "Bad").valid(), "short issue should fail");
        std::string maxCustomer, maxDevice, maxIssue;
        for (std::size_t i = 0; i < 200; ++i) {
            const char letter = static_cast<char>('A' + (i % 26));
            if (i < 60) maxCustomer.push_back(letter);
            if (i < 40) maxDevice.push_back(letter);
            maxIssue.push_back(letter);
        }
        check(maxCustomer.size() == 60 && maxDevice.size() == 40 && maxIssue.size() == 200,
              "maximum-length fixtures must have exact boundary lengths");
        check(validateTicketFields(maxCustomer, maxDevice, maxIssue).valid(),
              "maximum field lengths should pass");
        check(!validateTicketFields(std::string(61, 'C'), "Phone", "Screen issue").valid(), "overlong customer should fail");
        check(!validateTicketFields("Customer", std::string(41, 'D'), "Screen issue").valid(), "overlong device should fail");
        check(!validateTicketFields("Customer", "Phone", std::string(201, 'I')).valid(), "overlong issue should fail");
        check(!workshop.registerTicket("  ", "Phone", "Screen issue", id), "whitespace-only name should fail");
        check(!workshop.registerTicket("Customer\nName", "Phone", "Screen issue", id), "control characters should fail");
        check(!workshop.registerTicket("Customer\tName", "Phone", "Screen issue", id), "embedded tabs should fail");
        check(workshop.registerTicket("  O'Neil-Smith  ", "  Phone / tablet  ", "  Won't power on!  ", id),
              "legitimate punctuation and spaces should pass");
        const auto tickets = workshop.waitingTickets();
        check(tickets.size() == 1 && tickets[0].customerName == "O'Neil-Smith" &&
              tickets[0].deviceName == "Phone / tablet" && tickets[0].issueDescription == "Won't power on!",
              "workshop should store trimmed values");
        check(id == "T001", "invalid registrations must not consume IDs");
    });

    run("validation and queue state transitions", [] {
        RepairWorkshop workshop;
        std::string id;
        check(workshop.waitingTickets().empty(), "initial state should be empty");
        for (int i = 0; i < 8; ++i) check(workshop.registerTicket("Customer", "Device", "Issue", id), "valid registration failed");
        check(workshop.waitingTickets().size() == 8, "queue should report full capacity");
        const auto before = workshop.waitingTickets();
        check(!workshop.registerTicket("Customer", "Device", "Issue", id), "full queue should reject registration");
        check(workshop.waitingTickets().size() == before.size() && workshop.completedJobs() == 0,
              "failed registration should preserve queue and completed count");
        RepairTicket served;
        check(workshop.serveNextTicket(served) && workshop.completedJobs() == 1, "successful service should increment count");
        check(workshop.registerTicket("Customer", "Device", "Issue", id) && id == "T009", "freed slot should accept next ID");
        for (int i = 0; i < 8; ++i) check(workshop.serveNextTicket(served), "queue should drain");
        check(workshop.waitingTickets().empty() && workshop.completedJobs() == 9, "queue should return to valid empty state");
        check(!workshop.serveNextTicket(served) && workshop.completedJobs() == 9, "failed service must not increment count");
    });

    run("meaningful names, devices, and issue descriptions", [] {
        const auto valid = [](const std::string& name, const std::string& device, const std::string& issue) {
            return validateTicketFields(name, device, issue).valid();
        };
        check(!valid("Harry123", "iPhone 15", "Battery drains too quickly"), "customer digits must fail");
        check(!valid("12345", "iPhone 15", "Battery drains too quickly"), "numeric-only customer must fail");
        check(!valid("---...", "iPhone 15", "Battery drains too quickly"), "punctuation-only customer must fail");
        check(valid("Harry Fernando", "iPhone 15", "Battery drains too quickly"), "spaced name/product/issue should pass");
        check(valid("Mary-Jane Smith", "PS5 Controller", "Screen flickers after startup"), "hyphenated name should pass");
        check(valid("O'Connor", "Samsung TV", "USB port not working"), "apostrophe name should pass");
        check(valid("R. K. Sharma", "4K Monitor", "Battery drains too quickly"), "initials and model digits should pass");
        check(valid("José Álvarez", "HP Printer", "Screen flickers after startup"), "common UTF-8 letters should pass");
        check(!valid("Customer", "12345", "Battery drains too quickly"), "numeric-only device must fail");
        check(!valid("Customer", "---...", "Battery drains too quickly"), "punctuation-only device must fail");
        check(valid("Customer", "iPhone 15 Pro", "Battery drains too quickly"), "device model digits must pass");
        check(!valid("Customer", "iPhone 15", "123456"), "numeric-only issue must fail");
        check(!valid("Customer", "iPhone 15", "....."), "punctuation-only issue must fail");
        check(!valid("Customer", "iPhone 15", "No"), "too-short issue must fail");
        check(!valid("aaaaaaaaaaaaa", "iPhone 15", "Battery drains too quickly"), "repeated customer letters must fail");
        check(!valid("Harry Fernando", "111111", "Battery drains too quickly"), "repeated device digits must fail");
        check(!valid("Harry Fernando", "PS5 Controller", "aaaaaaaaaaaa"), "repeated issue letters must fail");
        check(!valid("Harry Fernando", "PS5 Controller", "a-a-a-a-a-a"),
              "separated repetitions must still be rejected as spam");
        check(valid("Harry Fernando", "iPhone 15", "Battery drains too quickly"),
              "normal mixed text and relevant digits must remain accepted");
        RepairWorkshop workshop;
        std::string id = "keep-this-id";
        check(!workshop.registerTicket("Harry123", "iPhone 15", "Battery drains too quickly", id),
              "workshop API must reject customer digits directly");
        check(id == "keep-this-id" && workshop.waitingTickets().empty(),
              "rejected direct API registration must preserve ID output and queue");
        check(workshop.completedJobs() == 0, "rejected registration must not alter completed-job count");
        check(workshop.registerTicket("Harry Fernando", "iPhone 15", "Battery drains too quickly", id) && id == "T001",
              "valid registration after rejected direct API call must still receive T001");
    });

    run("spam rejection preserves an existing queue, output ID, and completion count", [] {
        RepairWorkshop workshop;
        std::string id;
        check(workshop.registerTicket("Harry Fernando", "PS5 Controller", "USB port not working", id) && id == "T001",
              "valid setup registration should succeed");
        const auto before = workshop.waitingTickets();
        std::string rejectedId = "unchanged";
        check(!workshop.registerTicket("aaaaaaaaaaaaa", "PS5 Controller", "USB port not working", rejectedId),
              "spam customer name should be rejected by workshop API");
        const auto after = workshop.waitingTickets();
        check(rejectedId == "unchanged" && before.size() == after.size() &&
              before.front().id == after.front().id && workshop.completedJobs() == 0,
              "spam rejection must leave queue, ID output, and completion count untouched");
        check(workshop.registerTicket("Mary-Jane Smith", "4K Monitor", "Screen flickers after startup", id) && id == "T002",
              "valid follow-up should not skip an ID after spam rejection");
    });

    run("workshop queue view reports physical FRONT and next-insertion REAR", [] {
        RepairWorkshop workshop;
        auto view = workshop.queueView();
        check(view.slotTicketIds.size() == 8 && view.size == 0 && view.frontIndex == 0 && view.rearIndex == 0,
              "empty physical view should start at slot zero");
        std::string id;
        for (int i = 0; i < 8; ++i) check(workshop.registerTicket("Customer", "Device", "Issue", id), "registration failed");
        RepairTicket served;
        for (int i = 0; i < 3; ++i) check(workshop.serveNextTicket(served), "service failed");
        for (int i = 0; i < 3; ++i) check(workshop.registerTicket("Customer", "Device", "Issue", id), "wrapped registration failed");
        view = workshop.queueView();
        check(view.frontIndex == 3 && view.rearIndex == 3 && view.size == 8,
              "view must expose wrapped front and next insertion slot");
        check(view.slotTicketIds[3] == "T004" && view.slotTicketIds[7] == "T008" && view.slotTicketIds[0] == "T009",
              "physical slots should contain the correct actual tickets");
        const auto fifo = workshop.waitingTickets();
        check(fifo.size() == 8 && fifo.front().id == "T004" && fifo.back().id == "T011",
              "rendering physical view must preserve independent FIFO snapshot");
    });

    if (failures != 0) {
        std::cerr << failures << " check(s) failed.\n";
        return 1;
    }
    std::cout << "All tests passed.\n";
    return 0;
}

