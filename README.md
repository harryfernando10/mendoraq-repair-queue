# MendoraQ — Repair Workshop Manager

MendoraQ is a C++17 desktop application for organizing a small repair workshop's waiting requests. It demonstrates a manually implemented circular queue and separates its business rules from its Windows graphical interface.

## Purpose and SDG 8 relevance

Customers register a name, device or item, and reported issue. The workshop serves the oldest waiting request first. In this educational model, serving a ticket marks the job complete; there is no separate in-progress repair state. An orderly service queue is a small example of a tool that can help a local business organize its work, with a modest connection to **UN Sustainable Development Goal 8: Decent Work and Economic Growth**. The simulator does not measure or claim broader economic outcomes.

## Desktop interface

The initial dashboard is titled **MendoraQ — Repair Workshop Manager** and presents waiting capacity (`n / 8`), completed jobs, and Empty / Available / Full status. Its eight physical slots show occupied ticket IDs and the actual FRONT and next-insertion REAR indices; the FIFO table remains in logical service order. The balanced registration form validates each field and shows corrections inline. The Serve Next action names the served ticket and updates the completion count. The window resizes and maximizes with a minimum track size so the form and queue remain available. The executable manifest enables Per-Monitor DPI Awareness V2; `WM_DPICHANGED` updates child control fonts and recomputes geometry for the new DPI. GDI text uses Segoe UI with ClearType-quality fonts and is recreated during paint at the target window's DPI. The app uses native Windows controls and GDI drawing, with no third-party GUI dependency.

## Queue and workshop rules

`CircularTicketQueue` is a manually implemented fixed array of eight tickets. It maintains `front`, `rear` (the next insertion slot), and `size`; size distinguishes empty from full. Enqueue and dequeue advance indices with modulo arithmetic, and snapshots walk the logical occupied range in FIFO order without mutating it. The implementation does not use `std::queue`.

`RepairWorkshop` owns registration, validation, ID assignment, and completion counting. Shared field validation trims leading/trailing ASCII whitespace, counts UTF-8 characters, and applies these limits:

| Field | Allowed length |
| --- | ---: |
| Customer name | 2–60 characters |
| Device/item | 2–40 characters |
| Reported issue | 5–200 characters |

Customer names require 2–60 characters and at least two letters; letters, spaces, periods, apostrophes, and hyphens are allowed. Device/item names require 2–40 characters and at least two letters, with digits and common product notation allowed. Issue descriptions require 5–200 characters and at least three letters. Values are trimmed before validation; whitespace-only values and control characters are rejected. Common Latin, Greek, Cyrillic, Arabic, Indic, CJK, and Hangul letter ranges are recognized. This practical UTF-8 handling is not a complete Unicode character database. Repeated-character spam is rejected when it contains a run of six identical letters/digits or at least six meaningful characters with one character making up 80% or more. IDs start at `T001` for each run and use three digits as a minimum width. Rejected registration does not consume an ID. The completed-job count increases only after a successful FIFO service. All data is in memory and resets on exit.

## Structure

```text
include/RepairTicket.h             Ticket data model
include/CircularTicketQueue.h      Circular queue interface and physical slot snapshot
include/RepairWorkshop.h           Workshop API and validation result
src/CircularTicketQueue.cpp        Circular-array queue
src/RepairWorkshop.cpp             Validation, IDs, service counts, UI-safe queue view
src/gui_win32.cpp                  Resizable, per-monitor DPI-aware Windows dashboard
src/mendoraq.manifest              Per-Monitor V2 DPI awareness declaration
src/mendoraq.rc                    Embedded Windows application manifest resource
src/main.cpp                       Earlier console demonstration (not built)
tests/queue_tests.cpp              Dependency-free queue/workshop tests
CMakeLists.txt                     Application, core library, and CTest targets
sample_output.txt                  GUI behavior and demonstration notes
```

## Browser version (C++ WebAssembly)

The `web/` interface uses the same `mendoraq_core` C++ sources as the Windows application. `src/wasm_bridge.cpp` exposes a small Embind API around `RepairWorkshop`; JavaScript handles form events and renders the C++ FIFO snapshot and eight physical slots. No browser-side queue or persistence service is used. A built `web/mendoraq.js` and `web/mendoraq.wasm` are generated artifacts and are intentionally not committed.

### Build the WebAssembly module on Windows

Install Emscripten once using its official SDK in PowerShell. This does not change Windows security settings:

```powershell
git clone https://github.com/emscripten-core/emsdk.git "$env:USERPROFILE\emsdk"
Set-Location "$env:USERPROFILE\emsdk"
.\emsdk.bat install latest
.\emsdk.bat activate latest
```

After activation, open a new PowerShell window so the SDK paths are available. Return to the MendoraQ project directory and confirm `emcc -v` works before configuring/building:

```powershell
Set-Location "C:\path\to\mendoraq-repair-queue"
emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm --target mendoraq_web
```

This writes `web/mendoraq.js` and `web/mendoraq.wasm`. The generated files are ignored by Git. Requirements are Git for the one-time SDK checkout, the Emscripten SDK, and CMake 3.16 or newer. On Windows, activating the SDK updates the environment for new terminals; the official SDK also provides `emsdk_env.bat` for Command Prompt sessions.

### Serve and open

From the project root, start Python's static server:

```powershell
python -m http.server 8000 --directory web
```

Open <http://localhost:8000/> in a browser. Use HTTP rather than opening `index.html` as a `file://` URL so the browser can load the WebAssembly module. The browser session is in memory and resets when the page is reloaded. Before the module has been built, the page displays a setup notice.

To verify the C++ integration, build the module, open the page, register a ticket, and check that the ticket ID and physical slot appear. Register up to eight tickets, serve one, and register another: the physical slots wrap while the waiting table remains in FIFO order. The status and completed count are read from the C++ workshop state.

### Native tests

Keep using the native CMake build and tests described below; the optional Emscripten target does not replace them. There is no browser automation configured in this repository.

## Windows build and launch

Requirements: Windows, CMake 3.16 or newer, and a C++17 toolchain. For Visual Studio Build Tools, use a Developer PowerShell or Developer Command Prompt. From the project root:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Launch the desktop app with:

```powershell
& .\build\Release\mendoraq.exe
```

For a single-configuration generator such as Ninja or MinGW Makefiles, configure with the installed compiler and run `build\mendoraq.exe` directly. The project does not hardcode a toolchain path. The same CMake project can build the core and tests on non-Windows systems, but the GUI target is Windows-only.

## Tests

Configure and build as above, then run:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

The test target covers FIFO order, capacity and ninth-item rejection, wraparound, snapshots, validation limits and trimming, repeated-character spam rejection with normal-input regressions, ID sequencing, failed-registration non-mutation, completion counts, and empty/full transitions. Tests exercise the core without opening a window. No automated GUI smoke test is included; visually inspect form errors, add/serve feedback, queue rows, and disabled actions during a desktop demonstration.

## Complexity

- Enqueue, dequeue, peek, full/empty checks, and size: **O(1)**.
- FIFO snapshot: **O(n)** for `n` waiting tickets.
- Queue storage: **O(8)** ticket slots.

## Suggested four-person presentation split

1. Explain the circular array, modulo wraparound, and `front` / `rear` / `size` invariants.
2. Demonstrate workshop validation, ticket IDs, and safe failure behavior.
3. Walk through dashboard states, registration, FIFO display, and serving a job.
4. Present CMake/CTest, test coverage, project structure, and limitations such as in-memory storage.
