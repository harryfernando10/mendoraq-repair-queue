#include "RepairWorkshop.h"

#include <iostream>
#include <sstream>
#include <string>

namespace {
bool readNonBlankField(const std::string& prompt, std::string& value) {
    while (true) {
        std::cout << prompt;
        if (!std::getline(std::cin, value)) {
            return false;
        }
        if (value.find_first_not_of(" \t\n\r\f\v") != std::string::npos) {
            return true;
        }
        std::cout << "This field cannot be blank. Please try again.\n";
    }
}

bool parseMenuChoice(const std::string& line, int& choice) {
    std::istringstream input(line);
    if (!(input >> choice)) {
        return false;
    }
    // Reject trailing non-whitespace (for example "2abc") rather than accepting a prefix.
    input >> std::ws;
    return input.eof();
}

void printTicket(const RepairTicket& ticket) {
    std::cout << "Ticket ID: " << ticket.id << "\n"
              << "Customer: " << ticket.customerName << "\n"
              << "Device: " << ticket.deviceName << "\n"
              << "Reported issue: " << ticket.issueDescription << "\n";
}
}

int main() {
    RepairWorkshop workshop;
    std::cout << "=== MendoraQ Repair Workshop Service Queue ===\n";

    while (true) {
        std::cout << "\n1. Register a repair ticket\n"
                  << "2. Serve the next waiting ticket\n"
                  << "3. Display waiting tickets\n"
                  << "4. Display completed jobs\n"
                  << "5. Exit\n"
                  << "Choose an option: ";

        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\nInput ended. Exiting MendoraQ.\n";
            break;
        }

        int choice = 0;
        if (!parseMenuChoice(line, choice) || choice < 1 || choice > 5) {
            std::cout << "Invalid choice. Enter a number from 1 to 5.\n";
            continue;
        }

        if (choice == 1) {
            std::string customer, device, issue, id;
            if (!readNonBlankField("Customer name: ", customer) ||
                !readNonBlankField("Device or item: ", device) ||
                !readNonBlankField("Reported issue: ", issue)) {
                std::cout << "\nInput ended. Exiting MendoraQ.\n";
                break;
            }
            if (workshop.registerTicket(customer, device, issue, id)) {
                std::cout << "Ticket registered successfully. ID: " << id << "\n";
            } else {
                std::cout << "The waiting queue is full (8 tickets). Registration was not completed.\n";
            }
        } else if (choice == 2) {
            RepairTicket served;
            if (workshop.serveNextTicket(served)) {
                std::cout << "Ticket served and marked complete:\n";
                printTicket(served);
            } else {
                std::cout << "No tickets are waiting. The queue is empty.\n";
            }
        } else if (choice == 3) {
            const auto tickets = workshop.waitingTickets();
            if (tickets.empty()) {
                std::cout << "No tickets are currently waiting.\n";
            } else {
                std::cout << "Waiting tickets (FIFO order):\n";
                for (const auto& ticket : tickets) {
                    std::cout << "------------------------------\n";
                    printTicket(ticket);
                }
                std::cout << "------------------------------\n";
            }
        } else if (choice == 4) {
            std::cout << "Completed jobs: " << workshop.completedJobs() << "\n";
        } else {
            std::cout << "Thank you for using MendoraQ. Goodbye!\n";
            break;
        }
    }
    return 0;
}
