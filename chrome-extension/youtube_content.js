(function () {
  const BTN_ID = "idm-vibe-yt-download-btn";

  function isWatchOrShortsPage() {
    const p = window.location.pathname;
    return p.startsWith("/watch") || p.startsWith("/shorts/");
  }

  function ensureFloatingDownloadButton() {
    let existing = document.getElementById(BTN_ID);
    if (!isWatchOrShortsPage()) {
      if (existing) {
        existing.remove();
      }
      return;
    }
    if (existing) {
      return;
    }

    const btn = document.createElement("button");
    btn.id = BTN_ID;
    btn.type = "button";
    btn.innerHTML = `
      <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="#FFFFFF" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round" style="margin-right:6px;vertical-align:middle;">
        <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/>
        <polyline points="7 10 12 15 17 10"/>
        <line x1="12" y1="15" x2="12" y2="3"/>
      </svg>
      <span id="idm-vibe-yt-btn-label" style="vertical-align:middle;">Download Video (IDM Vibe)</span>
    `;
    Object.assign(btn.style, {
      position: "fixed",
      bottom: "22px",
      right: "22px",
      zIndex: "2147483647",
      display: "inline-flex",
      alignItems: "center",
      backgroundColor: "#2563EB",
      color: "#FFFFFF",
      border: "1px solid #3B82F6",
      borderRadius: "6px",
      padding: "9px 14px",
      fontFamily: "Segoe UI, Inter, Arial, sans-serif",
      fontSize: "12.5px",
      fontWeight: "600",
      cursor: "pointer",
      boxShadow: "0 6px 20px rgba(0, 0, 0, 0.45)",
      transition: "background-color 0.15s ease"
    });

    btn.addEventListener("mouseenter", () => {
      btn.style.backgroundColor = "#1D4ED8";
    });
    btn.addEventListener("mouseleave", () => {
      btn.style.backgroundColor = "#2563EB";
    });

    btn.addEventListener("click", () => {
      const labelSpan = document.getElementById("idm-vibe-yt-btn-label");
      const rawTitle = document.title.replace(/\s*-\s*YouTube$/i, "").trim();
      if (labelSpan) {
        labelSpan.textContent = "Sending to IDM Vibe...";
      }
      chrome.runtime.sendMessage(
        {
          type: "IDM_ENQUEUE",
          payload: {
            url: window.location.href,
            fileName: rawTitle ? `${rawTitle}.mp4` : "YouTube_Video.mp4",
            referer: window.location.href
          }
        },
        (res) => {
          if (labelSpan) {
            labelSpan.textContent =
              res && res.ok
                ? "Opened in IDM Vibe!"
                : "IDM Vibe Not Running (Start App)";
            setTimeout(() => {
              labelSpan.textContent = "Download Video (IDM Vibe)";
            }, 2500);
          }
        }
      );
    });

    document.body.appendChild(btn);
  }

  ensureFloatingDownloadButton();
  window.addEventListener("yt-navigate-finish", ensureFloatingDownloadButton);
  setInterval(ensureFloatingDownloadButton, 2000);
})();
