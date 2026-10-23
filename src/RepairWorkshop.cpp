#include "RepairWorkshop.h"

#include <iomanip>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <map>
#include <cctype>
#include <vector>

namespace {
std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\n\r\f\v");
    return value.substr(first, last - first + 1);
}

bool decodeUtf8(const std::string& value, std::vector<std::uint32_t>& points) {
    points.clear();
    for (std::size_t i = 0; i < value.size();) {
        const auto first = static_cast<unsigned char>(value[i]);
        std::uint32_t cp = 0; std::size_t width = 0;
        if (first < 0x80) { cp = first; width = 1; }
        else if ((first & 0xE0) == 0xC0) { cp = first & 0x1F; width = 2; }
        else if ((first & 0xF0) == 0xE0) { cp = first & 0x0F; width = 3; }
        else if ((first & 0xF8) == 0xF0) { cp = first & 0x07; width = 4; }
        else return false;
        if (i + width > value.size()) return false;
        for (std::size_t j = 1; j < width; ++j) {
            const auto next = static_cast<unsigned char>(value[i + j]);
            if ((next & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (next & 0x3F);
        }
        if ((width == 2 && cp < 0x80) || (width == 3 && cp < 0x800) ||
            (width == 4 && cp < 0x10000) || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
        points.push_back(cp); i += width;
    }
    return true;
}

bool isLetter(std::uint32_t cp) {
    if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) return true;
    // Practical Unicode letter ranges for common Latin, Greek, Cyrillic, Arabic,
    // Indic, CJK, and Hangul names. This is intentionally not a full Unicode database.
    if ((cp >= 0x00C0 && cp <= 0x02AF && cp != 0x00D7 && cp != 0x00F7) ||
        (cp >= 0x0370 && cp <= 0x052F) || (cp >= 0x0531 && cp <= 0x0588) ||
        (cp >= 0x05D0 && cp <= 0x05EA) || (cp >= 0x0620 && cp <= 0x064A) ||
        (cp >= 0x066E && cp <= 0x06D3) || (cp >= 0x0904 && cp <= 0x0939) ||
        (cp >= 0x0958 && cp <= 0x0961) || (cp >= 0x0985 && cp <= 0x09B9) ||
        (cp >= 0x0A05 && cp <= 0x0A39) || (cp >= 0x0A85 && cp <= 0x0AB9) ||
        (cp >= 0x0B05 && cp <= 0x0B39) || (cp >= 0x0B85 && cp <= 0x0BB9) ||
        (cp >= 0x0C05 && cp <= 0x0C39) || (cp >= 0x0C85 && cp <= 0x0CB9) ||
        (cp >= 0x0D05 && cp <= 0x0D3A) || (cp >= 0x0E01 && cp <= 0x0E30) ||
        (cp >= 0x0E81 && cp <= 0x0EAE) || (cp >= 0x10A0 && cp <= 0x10FF) ||
        (cp >= 0x3041 && cp <= 0x30FF) || (cp >= 0x3400 && cp <= 0x9FFF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) || (cp >= 0xF900 && cp <= 0xFAFF)) return true;
    return false;
}

bool isDigit(std::uint32_t cp) {
    return (cp >= '0' && cp <= '9') || (cp >= 0x0660 && cp <= 0x0669) ||
           (cp >= 0x06F0 && cp <= 0x06F9) || (cp >= 0x0966 && cp <= 0x096F);
}

std::string validateField(const std::string& value, std::size_t minimum, std::size_t maximum,
                          const char* label, int minimumLetters, bool customer, bool device) {
    std::vector<std::uint32_t> points;
    if (!decodeUtf8(value, points)) return std::string(label) + " must use valid text encoding.";
    for (auto cp : points) if (cp < 0x20 || (cp >= 0x7F && cp <= 0x9F))
        return std::string(label) + " cannot contain control characters.";
    if (points.size() < minimum) return std::string(label) + " must be at least " + std::to_string(minimum) + " characters.";
    if (points.size() > maximum) return std::string(label) + " must be no more than " + std::to_string(maximum) + " characters.";
    std::size_t letters = 0;
    std::size_t meaningfulCount = 0, highestRepeat = 0;
    std::map<std::uint32_t, std::size_t> meaningfulCounts;
    std::uint32_t previous = 0;
    std::size_t runLength = 0;
    for (auto cp : points) {
        const bool alpha = isLetter(cp);
        letters += alpha ? 1U : 0U;
        if (alpha || isDigit(cp)) {
            ++meaningfulCount;
            if (cp < 0x80) cp = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(cp)));
            const auto frequency = ++meaningfulCounts[cp];
            highestRepeat = std::max(highestRepeat, frequency);
            if (cp == previous) ++runLength;
            else { previous = cp; runLength = 1; }
            if (runLength >= 6) return std::string(label) + " cannot contain a run of six repeated letters or digits.";
        } else {
            previous = 0; runLength = 0;
        }
        if (customer && !alpha && cp != ' ' && cp != '.' && cp != '\'' && cp != '-')
            return "Use letters, spaces, periods, apostrophes, or hyphens.";
        if (device && !alpha && !isDigit(cp) && cp != ' ' && cp != '-' && cp != '_' && cp != '.' &&
            cp != '/' && cp != '+' && cp != '#' && cp != '(' && cp != ')' && cp != '&')
            return "Use letters, digits, spaces, and model punctuation.";
    }
    if (letters < static_cast<std::size_t>(minimumLetters))
        return std::string(label) + " must contain at least " + std::to_string(minimumLetters) + " alphabetic characters.";
    if (meaningfulCount >= 6 && highestRepeat * 100 >= meaningfulCount * 80)
        return std::string(label) + " cannot be mostly repetitions of one letter or digit.";
    return {};
}

}

bool TicketValidation::valid() const noexcept {
    return customerError.empty() && deviceError.empty() && issueError.empty();
}

TicketValidation validateTicketFields(const std::string& customerName,
                                      const std::string& deviceName,
                                      const std::string& issueDescription) {
    TicketValidation result;
    result.customerName = trim(customerName);
    result.deviceName = trim(deviceName);
    result.issueDescription = trim(issueDescription);
    result.customerError = validateField(result.customerName, 2, 60, "Customer name", 2, true, false);
    result.deviceError = validateField(result.deviceName, 2, 40, "Device/item", 2, false, true);
    result.issueError = validateField(result.issueDescription, 5, 200, "Issue description", 3, false, false);
    return result;
}

std::string RepairWorkshop::nextTicketId() const {
    std::ostringstream id;
    // setw is a minimum width, so IDs naturally grow to T1000 rather than truncating.
    id << 'T' << std::setw(3) << std::setfill('0') << nextId_;
    return id.str();
}

bool RepairWorkshop::registerTicket(const std::string& customerName,
                                    const std::string& deviceName,
                                    const std::string& issueDescription,
                                    std::string& assignedTicketId) {
    const auto fields = validateTicketFields(customerName, deviceName, issueDescription);
    if (!fields.valid() || queue_.isFull()) {
        return false;
    }

    RepairTicket ticket{nextTicketId(), fields.customerName, fields.deviceName, fields.issueDescription};
    if (!queue_.enqueue(ticket)) {
        return false;
    }

    // Advance the sequence only after the ticket is safely in the waiting queue.
    assignedTicketId = ticket.id;
    ++nextId_;
    return true;
}

bool RepairWorkshop::serveNextTicket(RepairTicket& servedTicket) {
    if (!queue_.dequeue(servedTicket)) {
        return false;
    }
    // Serving is defined as completion in this simulator; failed dequeues never count.
    ++completedJobs_;
    return true;
}

std::vector<RepairTicket> RepairWorkshop::waitingTickets() const { return queue_.snapshot(); }
TicketQueueView RepairWorkshop::queueView() const {
    const auto ids = queue_.slotTicketIds();
    return {std::vector<std::string>(ids.begin(), ids.end()), queue_.frontIndex(), queue_.rearIndex(), queue_.size()};
}
std::size_t RepairWorkshop::completedJobs() const noexcept { return completedJobs_; }
