document.addEventListener("DOMContentLoaded", () => {
  const badge = document.getElementById("statusBadge");
  const chkIntercept = document.getElementById("chkIntercept");
  const btnCurrentTab = document.getElementById("btnCurrentTab");
  const customUrlInput = document.getElementById("customUrlInput");
  const btnSendCustom = document.getElementById("btnSendCustom");
  const statusMsg = document.getElementById("statusMsg");

  chrome.runtime.sendMessage({ type: "IDM_PING" }, (res) => {
    if (res && res.online) {
      badge.textContent = `ONLINE :${res.port}`;
      badge.className = "badge online";
    } else {
      badge.textContent = "APP OFFLINE";
      badge.className = "badge offline";
    }
    if (res && typeof res.interceptEnabled === "boolean") {
      chkIntercept.checked = res.interceptEnabled;
    }
  });

  chkIntercept.addEventListener("change", () => {
    chrome.storage.local.set({ interceptEnabled: chkIntercept.checked });
  });

  btnCurrentTab.addEventListener("click", () => {
    chrome.tabs.query({ active: true, currentWindow: true }, (tabs) => {
      const tab = tabs && tabs[0];
      if (!tab || !tab.url) {
        return;
      }
      const cleanTitle = (tab.title || "").replace(/\s*-\s*YouTube$/i, "").trim();
      chrome.runtime.sendMessage(
        {
          type: "IDM_ENQUEUE",
          payload: {
            url: tab.url,
            fileName: cleanTitle ? `${cleanTitle}.mp4` : "",
            referer: tab.url
          }
        },
        (res) => {
          statusMsg.textContent =
            res && res.ok ? "Sent to IDM Vibe!" : "Failed: Ensure IDM Vibe.exe is running.";
        }
      );
    });
  });

  btnSendCustom.addEventListener("click", () => {
    const u = customUrlInput.value.trim();
    if (!u) {
      return;
    }
    chrome.runtime.sendMessage(
      {
        type: "IDM_ENQUEUE",
        payload: { url: u }
      },
      (res) => {
        statusMsg.textContent =
          res && res.ok ? "URL dispatched to IDM Vibe!" : "Failed: Ensure IDM Vibe.exe is running.";
      }
    );
  });
});
