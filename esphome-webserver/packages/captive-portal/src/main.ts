if (document.location.search === "?save") {
  const aside = document.getElementsByTagName("aside")[0];
  if (aside) {
    aside.style.display = "block";
  }
}

type ProductUi = {
  display_name?: string;
  product_url?: string;
  update_url?: string;
  ota_warning?: string;
};

type AccessPoint = {
  ssid: string;
  rssi: number;
  lock: boolean;
};

type Config = {
  name: string;
  mac: string;
  hard_ssid: string;
  soft_ssid: string;
  free_sp: number;
  build_ts: string;
  esph_v: string;
  proj_n: string;
  proj_v: string;
  kauf_ui?: ProductUi;
  aps: AccessPoint[];
};

function wifi(dBm: number): string {
  const q = Math.max(Math.min(2 * (dBm + 100), 100), 0) / 100;
  return `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24"><path d="M12 19.25L.7 4.25c7-5 14-5 22.5 0z" fill="none" stroke="currentColor"/><path d="M12 19.25L.7 4.25c7-5 14-5 22.5 0z" transform="scale(${q} ${q})" transform-origin="12 18"/></svg>`;
}

function lock(show: boolean): string {
  if (!show) {
    return "";
  }
  return `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24"><path d='M12 17a2 2 0 0 0 2-2 2 2 0 0 0-2-2 2 2 0 0 0-2 2 2 2 0 0 0 2 2m6-9a2 2 0 0 1 2 2v10a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V10a2 2 0 0 1 2-2h1V6a5 5 0 0 1 5-5 5 5 0 0 1 5 5v2h1m-6-5a3 3 0 0 0-3 3v2h6V6a3 3 0 0 0-3-3z'/></svg>`;
}

function setText(selector: string, value: string): void {
  const el = document.querySelector(selector);
  if (el) {
    el.textContent = value;
  }
}

function formatBytes(value: number): string {
  return Number(value || 0).toLocaleString("en-US");
}

function renderNetworks(aps: AccessPoint[]): void {
  const net = document.querySelector("#net");
  if (!net) {
    return;
  }
  net.textContent = "";

  aps.forEach((ap) => {
    const row = document.createElement("div");
    row.className = "network";
    row.onclick = () => {
      const ssidInput = document.getElementById("ssid") as HTMLInputElement | null;
      const pskInput = document.getElementById("psk") as HTMLInputElement | null;
      if (ssidInput) {
        ssidInput.value = ap.ssid;
      }
      if (pskInput) {
        pskInput.focus();
      }
    };

    const left = document.createElement("a");
    left.href = "#";
    left.className = "network-left";
    left.innerHTML = wifi(ap.rssi);

    const ssid = document.createElement("span");
    ssid.className = "network-ssid";
    ssid.textContent = ap.ssid;
    left.appendChild(ssid);

    row.appendChild(left);
    if (ap.lock) {
      const lockIcon = document.createElement("span");
      lockIcon.innerHTML = lock(true);
      row.appendChild(lockIcon);
    }
    net.appendChild(row);
  });
}

function renderProductInfo(config: Config): void {
  const product = config.kauf_ui || {};

  setText("#t_product", product.display_name || "");
  setText("#t_projn", config.proj_n || "");
  setText("#t_projv", config.proj_v || "");

  const otaWarning = document.querySelector("#ota_warning");
  if (otaWarning) {
    otaWarning.textContent = product.ota_warning ? `** ${product.ota_warning}` : "";
  }
}

fetch("/config.json").then((response) => {
  response.json().then((config: Config) => {
    document.title = config.name;
    renderNetworks(config.aps || []);
    const favicon = document.querySelector("link[rel~='icon']") as HTMLLinkElement | null;
    if (favicon) {
      favicon.href = `data:image/svg+xml,${wifi(-65)}`;
    }

    setText("#t_host", config.name || "");
    setText("#t_mac", config.mac || "");
    setText("#t_hard", config.hard_ssid || "");
    setText("#t_soft", config.soft_ssid || "");
    setText("#t_free", `${formatBytes(config.free_sp || 0)} bytes`);
    setText("#t_buildts", config.build_ts || "");
    setText("#t_esphv", config.esph_v || "");

    renderProductInfo(config);
  });
});
