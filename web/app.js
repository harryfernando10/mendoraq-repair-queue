const form = document.querySelector("#ticket-form");
const feedback = document.querySelector("#feedback");
const fields = ["customer", "device", "issue"];
const errorIds = ["customer-error", "device-error", "issue-error"];
let workshop;
let successfulReset = false;

function vectorToArray(vector) {
  return Array.from({ length: vector.size() }, (_, index) => vector.get(index));
}

function showErrors(result) {
  const errors = [result.customerError, result.deviceError, result.issueError];
  fields.forEach((id, index) => {
    const input = document.getElementById(id);
    document.getElementById(errorIds[index]).textContent = errors[index] || "";
    input.toggleAttribute("aria-invalid", Boolean(errors[index]));
  });
}

function render() {
  const state = workshop.state();
  const slots = vectorToArray(state.slots);
  const waiting = vectorToArray(state.waiting);
  document.querySelector("#waiting-count").innerHTML = `${state.size}<span class="capacity"> / ${state.capacity}</span>`;
  document.querySelector("#completed-count").textContent = state.completedJobs;
  document.querySelector("#queue-pill").textContent = state.size;
  const status = state.full ? "Full" : state.size === 0 ? "Empty" : "Available";
  document.querySelector("#queue-status").textContent = status;
  const dot = document.querySelector("#status-dot");
  dot.className = state.full ? "full" : state.size ? "available" : "";
  document.querySelector("#serve-button").disabled = state.size === 0;

  const slotContainer = document.querySelector("#buffer-slots");
  slotContainer.replaceChildren(...slots.map((id, index) => {
    const slot = document.createElement("div");
    slot.className = `slot ${id ? "occupied" : "empty"}`;
    slot.setAttribute("aria-label", `Slot ${index}: ${id || "empty"}${index === state.frontIndex ? ", front, next to serve" : ""}${index === state.rearIndex ? ", rear, next insertion" : ""}`);
    const slotIndex = document.createElement("span");
    slotIndex.className = "slot-index";
    slotIndex.textContent = String(index).padStart(2, "0");
    const ticketId = document.createElement("span");
    ticketId.className = "slot-id";
    ticketId.textContent = id || "—";
    slot.append(slotIndex, ticketId);
    if (index === state.frontIndex) {
      const pointer = document.createElement("span");
      pointer.className = "slot-pointer front-key";
      pointer.textContent = "FRONT";
      slot.append(pointer);
    }
    if (index === state.rearIndex) {
      const pointer = document.createElement("span");
      pointer.className = "slot-pointer rear-key";
      pointer.textContent = "REAR";
      slot.append(pointer);
    }
    return slot;
  }));

  const rows = document.querySelector("#ticket-rows");
  rows.replaceChildren(...waiting.map(ticket => {
    const row = document.createElement("tr");
    const values = [ticket.id, ticket.customerName, ticket.deviceName, ticket.issueDescription];
    values.forEach((value, index) => {
      const cell = document.createElement("td");
      cell.textContent = value;
      if (index === 0) cell.className = "ticket-id";
      row.append(cell);
    });
    return row;
  }));
  document.querySelector("#empty-state").hidden = waiting.length > 0;
}

function announce(message, isError = false) {
  feedback.textContent = message;
  feedback.classList.toggle("error", isError);
}

form.addEventListener("submit", event => {
  event.preventDefault();
  if (!workshop) return;
  const result = workshop.registerTicket(
    document.querySelector("#customer").value,
    document.querySelector("#device").value,
    document.querySelector("#issue").value
  );
  showErrors(result);
  announce(result.message, !result.success);
  if (result.success) {
    successfulReset = true;
    form.reset();
    showErrors({});
    announce(result.message);
  }
  render();
});

form.addEventListener("reset", () => {
  queueMicrotask(() => {
    showErrors({});
    if (!successfulReset) announce("");
    successfulReset = false;
  });
});

document.querySelector("#serve-button").addEventListener("click", () => {
  if (!workshop) return;
  const result = workshop.serveNext();
  announce(result.message, !result.success);
  render();
});

if (typeof createMendoraQModule === "function") {
  createMendoraQModule().then(module => {
    workshop = new module.Workshop();
    document.querySelector("#module-error").hidden = true;
    render();
  }).catch(() => {
    document.querySelector("#module-error").hidden = false;
  });
}
