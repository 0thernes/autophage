/* SPDX-License-Identifier: Apache-2.0 */
const q = document.getElementById("filter-q");
const modSel = document.getElementById("filter-mod");
const rows = Array.from(document.querySelectorAll("#thm-body tr"));
const countEl = document.getElementById("thm-count");
const nodes = Array.from(document.querySelectorAll(".dag-node"));

if (q && modSel && countEl) {
  function applyFilter() {
    const needle = (q.value || "").trim().toLowerCase();
    const mod = modSel.value;
    let shown = 0;
    for (const tr of rows) {
      const okMod = mod === "ALL" || tr.dataset.mod === mod;
      const okText = !needle || tr.textContent.toLowerCase().includes(needle);
      const vis = okMod && okText;
      tr.hidden = !vis;
      if (vis) shown++;
    }
    countEl.textContent = `Showing ${shown} / ${rows.length} audited theorems`;
    for (const n of nodes) n.classList.toggle("active", n.dataset.mod === mod && mod !== "ALL");
  }

  q.addEventListener("input", applyFilter);
  modSel.addEventListener("change", applyFilter);
  for (const n of nodes) {
    const pick = () => {
      modSel.value = n.dataset.mod || "ALL";
      applyFilter();
    };
    n.addEventListener("click", pick);
    n.addEventListener("keydown", (e) => {
      if (e.key === "Enter" || e.key === " ") { e.preventDefault(); pick(); }
    });
  }
  applyFilter();
}
