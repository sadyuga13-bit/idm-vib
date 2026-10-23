const DEFAULT_PORT = 16800;

async function getConfig() {
  return new Promise((resolve) => {
    chrome.storage.local.get(
      { interceptEnabled: true, ipcPort: DEFAULT_PORT },
      (items) => resolve(items)
    );
  });
}

async function pingIdmVibe(port = DEFAULT_PORT) {
  try {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), 1200);
    const res = await fetch(`http://127.0.0.1:${port}/ping`, {
      method: "GET",
      signal: controller.signal
    });
    clearTimeout(timer);
    return res.ok;
  } catch (_) {
    return false;
  }
}

async function sendToIdmVibe(payload) {
  const cfg = await getConfig();
  const body = {
    url: payload.url || "",
    fileName: payload.fileName || "",
    referer: payload.referer || "",
    userAgent: navigator.userAgent,
    cookies: payload.cookies || ""
  };
  const res = await fetch(`http://127.0.0.1:${cfg.ipcPort}/`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body)
  });
  return res.ok;
}

chrome.runtime.onInstalled.addListener(() => {
  chrome.contextMenus.create({
    id: "idm-vibe-download-link",
    title: "Download Link with IDM Vibe",
    contexts: ["link", "video", "audio"]
  });
  chrome.contextMenus.create({
    id: "idm-vibe-download-page",
    title: "Download YouTube / Page Video with IDM Vibe",
    contexts: ["page"]
  });
});

chrome.contextMenus.onClicked.addListener(async (info, tab) => {
  if (info.menuItemId === "idm-vibe-download-link") {
    const targetUrl = info.linkUrl || info.srcUrl || "";
    if (targetUrl) {
      await sendToIdmVibe({
        url: targetUrl,
        referer: tab && tab.url ? tab.url : ""
      });
    }
  } else if (info.menuItemId === "idm-vibe-download-page" && tab && tab.url) {
    await sendToIdmVibe({
      url: tab.url,
      fileName: tab.title ? `${tab.title}.mp4` : "",
      referer: tab.url
    });
  }
});

chrome.downloads.onCreated.addListener(async (downloadItem) => {
  try {
    if (!downloadItem || !downloadItem.finalUrl && !downloadItem.url) {
      return;
    }
    const targetUrl = downloadItem.finalUrl || downloadItem.url;
    if (!targetUrl.startsWith("http://") && !targetUrl.startsWith("https://")) {
      return;
    }
    if (downloadItem.state && downloadItem.state !== "in_progress") {
      return;
    }

    const cfg = await getConfig();
    if (!cfg.interceptEnabled) {
      return;
    }

    const online = await pingIdmVibe(cfg.ipcPort);
    if (!online) {
      return;
    }

    chrome.downloads.cancel(downloadItem.id, () => {
      chrome.downloads.erase({ id: downloadItem.id });
    });

    const rawName = downloadItem.filename
      ? downloadItem.filename.split(/[\\/]/).pop()
      : "";

    await sendToIdmVibe({
      url: targetUrl,
      fileName: rawName,
      referer: downloadItem.referrer || ""
    });
  } catch (_) {}
});

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  if (msg && msg.type === "IDM_PING") {
    getConfig().then((cfg) =>
      pingIdmVibe(cfg.ipcPort).then((online) =>
        sendResponse({ online, port: cfg.ipcPort, interceptEnabled: cfg.interceptEnabled })
      )
    );
    return true;
  }
  if (msg && msg.type === "IDM_ENQUEUE") {
    sendToIdmVibe(msg.payload || {})
      .then((ok) => sendResponse({ ok }))
      .catch(() => sendResponse({ ok: false }));
    return true;
  }
  return false;
});
