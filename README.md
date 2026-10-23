# MendoraQ — Repair Workshop Manager

**A C++17 repair workshop simulator demonstrating a manually implemented circular queue, FIFO service, robust input validation, and a native Windows desktop interface.**

[**🌐 Open Live Demo**](https://harryfernando10.github.io/mendoraq-repair-queue/) · [**📦 Source Code**](https://github.com/harryfernando10/mendoraq-repair-queue)

---

## Overview

MendoraQ helps a small repair workshop organize incoming repair requests in the order they arrive. Customers submit a name, device or item, and issue description. The workshop serves the oldest waiting ticket first, following the **First In, First Out (FIFO)** principle.

The project demonstrates how a fundamental data structure can solve a practical scheduling problem. It includes a manually implemented circular queue with a fixed capacity of eight tickets, a separate workshop-management layer, and two interfaces:

- **Windows desktop application:** A native, resizable graphical dashboard built with Win32 and GDI.
- **Browser application:** An HTML/CSS/JavaScript interface connected to the same C++ workshop logic compiled to WebAssembly.

All workshop data is stored in memory and resets when the application session ends.

## Try MendoraQ

### Live browser demo

**[Launch MendoraQ →](https://harryfernando10.github.io/mendoraq-repair-queue/)**

The browser version lets you register repair requests, view the waiting queue, inspect the circular buffer, and serve tickets without installing a C++ compiler.

### How to use it

1. Enter the customer's name, device or item, and reported issue.
2. Register the ticket and review the generated ticket ID.
3. Inspect the waiting queue and the eight physical buffer slots.
4. Select **Serve Next** to complete the oldest waiting ticket.
5. Observe the queue, buffer indices, queue status, and completed-job count update.

The queue accepts at most eight waiting tickets. A ninth ticket is rejected until space becomes available.

> **Note:** The browser demo uses WebAssembly to execute the C++ workshop logic. The desktop application is a separate native Windows build.

## Features

| Feature | Description |
|---|---|
| Fixed-capacity circular queue | Manually implemented array-based queue with eight slots |
| FIFO service | Tickets are served in their order of arrival |
| Ticket identification | Sequential IDs such as `T001`, `T002`, and `T003` |
| Input validation | Field-specific length, character, and spam checks |
| Safe registration | Invalid requests do not consume ticket IDs |
| Queue status | Displays waiting capacity and Empty, Available, or Full status |
| Physical buffer visualization | Shows all eight slots and the FRONT and REAR indices |
| Completion tracking | Increments only after a successful service operation |
| Native desktop interface | Resizable Windows dashboard with DPI-aware rendering |
| Browser interface | WebAssembly-powered frontend using the existing C++ core |
| Automated core tests | Tests queue behavior, validation, IDs, and state transitions |

## Understanding the circular queue

MendoraQ uses a **circular array**, rather than `std::queue`, to implement its waiting queue.

The buffer contains eight physical slots, indexed from `0` to `7`.

### Queue state

- **`front`** — Index of the oldest waiting ticket and the next ticket to serve.
- **`rear`** — Index of the next slot where a new ticket will be inserted.
- **`size`** — Number of tickets currently waiting.

The `size` variable distinguishes an empty queue from a full queue, even when the index positions wrap around.

### Circular movement

Both insertion and removal advance their indices using modulo arithmetic:

`next_index = (current_index + 1) % 8`

When an index reaches slot `7`, its next position becomes slot `0`. This allows the queue to reuse vacated slots without shifting all remaining elements.

### FIFO behavior

- **Enqueue:** Insert a valid ticket at `rear`, advance the index, and update the size.
- **Dequeue:** Remove the ticket at `front`, advance the index, and update the size.
- **Snapshot:** Traverse the occupied slots in logical FIFO order without modifying the queue.
- **Full queue:** Reject additional tickets when all eight slots are occupied.
- **Empty queue:** Prevent service when no tickets are waiting.

The physical circular buffer and the logical waiting list are different views of the same queue. The dashboard shows the physical slots, while the service table displays tickets in FIFO order.

## Validation and business rules

The `RepairWorkshop` layer owns input validation, registration, ticket-ID assignment, and completion counting. These rules are enforced in C++, rather than relying solely on frontend checks.

| Field | Length limit | Validation requirements |
|---|---:|---|
| Customer name | 2–60 characters | At least two letters; supports spaces, periods, apostrophes, and hyphens |
| Device or item | 2–40 characters | At least two letters; digits and common product notation are permitted |
| Reported issue | 5–200 characters | At least three letters; ordinary punctuation and digits are permitted |

Additional validation rules:

- Leading and trailing ASCII whitespace is trimmed before validation.
- Blank values and control characters are rejected.
- Repeated-character spam is rejected when a run of six identical letters/digits occurs, or when at least six meaningful characters contain one character making up 80% or more.
- Common Latin, Greek, Cyrillic, Arabic, Indic, CJK, and Hangul letter ranges are recognized.
- UTF-8 character counting and letter recognition are practical implementations, not a complete Unicode character database.

### Ticket and completion rules

- Ticket IDs begin at `T001` for each application run, with three digits as the minimum width.
- Failed registration does not consume a ticket ID.
- A ticket is completed when it is successfully served from the front of the queue.
- Serving an empty queue does not increment the completed count.
- There is no separate in-progress repair state, database, or persistent storage.

## Architecture and technology stack

The project separates the queue implementation, workshop rules, and presentation layers.

### Native Windows application

- **Language:** C++17
- **GUI:** Win32 API and GDI
- **Rendering:** Segoe UI fonts with ClearType-quality text rendering
- **DPI support:** Per-Monitor DPI Awareness V2 and `WM_DPICHANGED` handling
- **Build system:** CMake
- **Testing:** CTest with a dependency-free C++ test executable

The desktop application uses native Windows APIs without a third-party GUI framework. Its window can be resized or maximized while maintaining a minimum usable layout.

### Browser application

- **Core logic:** Existing C++ workshop and circular queue implementation
- **WebAssembly:** Emscripten and Embind
- **Frontend:** HTML, CSS, and JavaScript
- **Hosting and deployment:** GitHub Pages with GitHub Actions

The browser frontend handles form interactions and presentation. Queue operations, validation, ticket IDs, and completion counting remain in C++. Embind exposes the required C++ interface to JavaScript.

The browser version does not implement a separate JavaScript queue or use a persistence service. Its state exists only for the current page session.

## Project structure

```text
mendoraq-repair-queue/
├── .github/
│   └── workflows/
│       └── deploy-pages.yml
├── include/
│   ├── RepairTicket.h
│   ├── CircularTicketQueue.h
│   └── RepairWorkshop.h
├── src/
│   ├── CircularTicketQueue.cpp
│   ├── RepairWorkshop.cpp
│   ├── gui_win32.cpp
│   ├── wasm_bridge.cpp
│   ├── main.cpp
│   ├── mendoraq.manifest
│   └── mendoraq.rc
├── tests/
│   └── queue_tests.cpp
├── web/
│   ├── index.html
│   ├── styles.css
│   └── app.js
├── CMakeLists.txt
├── README.md
├── .gitignore
└── sample_output.txt
```

| File | Responsibility |
|---|---|
| `RepairTicket.h` | Ticket data model |
| `CircularTicketQueue.h/.cpp` | Fixed-array circular queue and physical slot snapshots |
| `RepairWorkshop.h/.cpp` | Validation, registration, IDs, service counts, and workshop state |
| `gui_win32.cpp` | Native Windows desktop interface |
| `wasm_bridge.cpp` | Embind interface exposing the C++ core to the browser |
| `main.cpp` | Earlier console demonstration; not built as the current GUI application |
| `mendoraq.manifest` and `mendoraq.rc` | Windows application manifest and embedded resource |
| `queue_tests.cpp` | Queue and workshop unit tests |
| `web/` | Browser interface and frontend assets |
| `deploy-pages.yml` | Automated WebAssembly build and GitHub Pages deployment |

Generated WebAssembly files, including `web/mendoraq.js` and `web/mendoraq.wasm`, are build artifacts and are intentionally excluded from source control.

## Build and run on Windows

### Prerequisites

- Windows
- CMake 3.16 or newer
- A C++17 compiler toolchain, such as Visual Studio Build Tools or an appropriately configured GCC toolchain

Open a Developer PowerShell or Developer Command Prompt when using Visual Studio Build Tools.

From the project root, configure and build:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Launch the native application:

```powershell
& .\build\Release\mendoraq.exe
```

These commands use a Visual Studio generator. For a single-configuration generator such as Ninja or MinGW Makefiles, configure with the installed toolchain and launch the executable from the corresponding build directory instead.

## Build the WebAssembly version locally

The live browser demo is built automatically by GitHub Actions. To build the WebAssembly module on your own Windows machine, install the official Emscripten SDK.

### 1. Install and activate Emscripten

In PowerShell:

```powershell
git clone https://github.com/emscripten-core/emsdk.git "$env:USERPROFILE\emsdk"
Set-Location "$env:USERPROFILE\emsdk"

.\emsdk.bat install latest
.\emsdk.bat activate latest
```

Open an Emscripten-configured terminal and confirm that the compiler is available:

```powershell
emcc -v
```

### 2. Configure and compile

Return to the MendoraQ project directory:

```powershell
Set-Location "C:\path\to\mendoraq-repair-queue"

emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm --target mendoraq_web
```

Replace the example path with your actual project location.

The build should generate:

```text
web/mendoraq.js
web/mendoraq.wasm
```

### 3. Run the local browser version

From the project root, start a local HTTP server:

```powershell
python -m http.server 8000 --directory web
```

Open [http://localhost:8000/](http://localhost:8000/) in your browser.

Use HTTP instead of opening `index.html` directly as a `file://` URL so the browser can load the WebAssembly module correctly.

Stop the server with `Ctrl + C` when finished.

## Automated deployment

The repository includes a GitHub Actions workflow that builds the existing C++ WebAssembly target and publishes the contents of `web/` to GitHub Pages.

On pushes to `main`, the workflow:

1. Checks out the repository.
2. Configures the Emscripten build environment.
3. Compiles the `mendoraq_web` target.
4. Verifies that the generated JavaScript and WebAssembly module exist.
5. Uploads the website artifact and deploys it to GitHub Pages.

The deployed site is available at:

**https://harryfernando10.github.io/mendoraq-repair-queue/**

To publish future updates, commit and push the relevant source changes to `main`. The deployment workflow will run automatically.

## Tests and verification

The native test suite exercises the queue and workshop core without opening a graphical window.

After configuring and building the project, run:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Coverage includes:

- FIFO insertion and removal
- Queue capacity and ninth-ticket rejection
- Circular wraparound
- Queue snapshots and physical slot state
- Empty/full transitions
- Input trimming, validation limits, and repeated-character spam rejection
- Normal-input regression cases
- Ticket-ID sequencing
- Failed-registration state preservation
- Completion counting

The browser JavaScript syntax can also be checked with:

```powershell
node --check web/app.js
```

**Testing limitation:** The repository does not include automated browser end-to-end tests or a native GUI smoke-test suite. The browser interface and Windows dashboard should therefore be tested manually in addition to running the core tests.

## Complexity analysis

Let \(n\) be the number of waiting tickets, where \(0 \le n \le 8\).

| Operation | Time complexity |
|---|---:|
| Enqueue | \(O(1)\) |
| Dequeue | \(O(1)\) |
| Peek at next ticket | \(O(1)\) |
| Check empty/full status | \(O(1)\) |
| Read queue size | \(O(1)\) |
| Create FIFO snapshot | \(O(n)\) |

**Queue storage:** \(O(8)\), which is constant space because the capacity is fixed.

## SDG 8 relevance

MendoraQ has a modest educational connection to **United Nations Sustainable Development Goal 8: Decent Work and Economic Growth**.

An orderly service queue illustrates how a small workshop can structure incoming requests, maintain service order, and track completed jobs. The project demonstrates a basic organizational tool; it does not measure productivity, employment, income, or broader economic impact.

## Limitations and possible extensions

Current limitations:

- Fixed capacity of eight waiting tickets
- In-memory data only; state resets when the application or browser session ends
- No database, authentication, networking, or persistent history
- Serving a ticket marks it complete immediately; no repair-progress workflow is modeled
- No automated browser end-to-end tests
- Unicode letter recognition covers common ranges but is not a complete Unicode implementation

Possible future improvements include persistent storage, repair-status tracking, service history, configurable queue capacity, and automated browser tests. These are potential extensions, not existing features.

---

**MendoraQ** demonstrates how a manually implemented circular queue can support a practical workflow while keeping data-structure logic, business rules, and user-interface code separate.

[Open the live demo](https://harryfernando10.github.io/mendoraq-repair-queue/) · [Explore the source code](https://github.com/harryfernando10/mendoraq-repair-queue)
