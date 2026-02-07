import { LitElement, html, css, PropertyValues, nothing } from "lit";
import { customElement, state, query } from "lit/decorators.js";
import { getBasePath } from "./esp-entity-table";

import "./esp-entity-table";
import "./esp-log";
import "./esp-switch";
import cssReset from "./css/reset";
import cssButton from "./css/button";

window.source = new EventSource(getBasePath() + "/events");
const ENABLE_LEGACY_FALLBACK = import.meta.env.VITE_LEGACY_FALLBACK === "true";

type ProductUi = {
  display_name: string;
  product_url: string;
  update_url: string;
  ota_warning: string;
};

const LEGACY_PRODUCT_UI_BY_PROJECT: Record<string, ProductUi> =
  ENABLE_LEGACY_FALLBACK
    ? {
        "Kauf.PLF10": {
          display_name: "Plug (PLF10)",
          product_url: "https://kaufha.com/plf10",
          update_url: "https://github.com/KaufHA/PLF10/releases",
          ota_warning: "",
        },
        "Kauf.PLF12": {
          display_name: "Plug (PLF12)",
          product_url: "https://kaufha.com/plf12",
          update_url: "https://github.com/KaufHA/PLF12/releases",
          ota_warning: "",
        },
        "Kauf.RGBWW": {
          display_name: "RGBWW Bulb",
          product_url: "https://kaufha.com/blf10",
          update_url: "https://github.com/KaufHA/kauf-rgbww-bulbs/releases",
          ota_warning:
            "DO NOT USE ANY WLED BIN FILE. WLED is not going to work properly on this bulb. Use the included DDP functionality to control this bulb from another WLED instance or xLights.",
        },
        "Kauf.RGBSw": {
          display_name: "RGB Switch",
          product_url: "https://kaufha.com/srf10",
          update_url: "https://github.com/KaufHA/kauf-rgb-switch/releases",
          ota_warning: "",
        },
      }
    : {};

interface Config {
  ota: boolean;
  log: boolean;
  title: string;
  comment: string;
  lang?: string;
  proj_n?: string;
  proj_v?: string;
  esph_v?: string;
  free_sp?: number;
  build_ts?: string;
  hostname?: string;
  mac_addr?: string;
  hard_ssid?: string;
  soft_ssid?: string;
  has_ap?: boolean;
  kauf_ui?: {
    display_name?: string;
    product_url?: string;
    update_url?: string;
    ota_warning?: string;
    factory_warning?: string;
  };
  featured_name?: string;
}

@customElement("esp-app")
export default class EspApp extends LitElement {
  @state() scheme: string = "";
  @state() ping: string = "";
  @state() lastApiCall: string = "";
  @state() eventConnected: boolean = true;
  @state() showReconnectNotice: boolean = false;
  @state() hadDisconnect: boolean = false;
  @state() lastPingMs: number = Date.now();
  @state() hasOpenedOnce: boolean = false;
  @query("#beat")
  beat!: HTMLSpanElement;

  version: string = import.meta.env.PACKAGE_VERSION;
  config: Config = { ota: false, log: true, title: "", comment: "" };

  darkQuery: MediaQueryList = window.matchMedia("(prefers-color-scheme: dark)");

  frames = [
    { color: "inherit" },
    { color: "red", transform: "scale(1.25) translateY(-30%)" },
    { color: "inherit" },
  ];

  constructor() {
    super();
    const conf = document.querySelector('script#config');
    if ( conf ) this.setConfig(JSON.parse(conf.innerText));
  }

  setConfig(config: any) {
    if (!("log" in config)) {
      config.log = this.config.log;
    }
    this.config = config;

    document.title = config.title;
    document.documentElement.lang = config.lang || "en";
  }

  firstUpdated(changedProperties: PropertyValues) {
    super.firstUpdated(changedProperties);
    document.getElementsByTagName("head")[0].innerHTML +=
      '<meta name=viewport content="width=device-width, initial-scale=1,user-scalable=no">';
    const l = <HTMLLinkElement>document.querySelector("link[rel~='icon']"); // Set favicon to house
    l.href =
      'data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" width="25" height="25"><path d="M1 12.5h2.9v7.8h17v-7.8h2.9l-2.9-2.9V4.5h-1.8v3.3L12.3 1 1 12.5Z"/></svg>';
    this.darkQuery.addEventListener("change", () => {
      this.scheme = this.isDark();
    });
    this.scheme = this.isDark();
    window.source.addEventListener("ping", (e: Event) => {
      const messageEvent = e as MessageEvent;
      const d: string = messageEvent.data;
      if (d.length) {
        this.setConfig(JSON.parse(messageEvent.data));
      }
      this.ping = messageEvent.lastEventId;
      this.eventConnected = true;
      this.lastPingMs = Date.now();
    });
    document.addEventListener("webserver-api-call", (e: Event) => {
      const ev = e as CustomEvent;
      const method = ev.detail?.method || "POST";
      const url = ev.detail?.url || "";
      if (url) {
        this.lastApiCall = `${method} ${url}`;
      }
    });
    window.source.onopen = () => {
      this.eventConnected = true;
      if (this.hasOpenedOnce) {
        this.showReconnectNotice = true;
      }
      this.hasOpenedOnce = true;
    };
    window.source.onerror = (e: Event) => {
      console.dir(e);
      this.eventConnected = false;
      this.hadDisconnect = true;
      //alert("Lost event stream!")
    };

    // Heartbeat watchdog: if we miss pings for too long, flag as disconnected.
    window.setInterval(() => {
      const elapsed = Date.now() - this.lastPingMs;
      if (elapsed > 22500) {
        this.eventConnected = false;
        this.hadDisconnect = true;
      }
    }, 5000);
  }

  isDark() {
    return this.darkQuery.matches ? "dark" : "light";
  }

  updated(changedProperties: Map<string, unknown>) {
    super.updated(changedProperties);
    if (changedProperties.has("scheme")) {
      let el = document.documentElement;
      document.documentElement.style.setProperty("color-scheme", this.scheme);
    }
    if (changedProperties.has("ping")) {
      this.beat.animate(this.frames, 1000);
    }
  }

  private getFallbackMetadata(): ProductUi {
    if (!this.config.proj_n) {
      return { display_name: "", product_url: "", update_url: "", ota_warning: "" };
    }
    return (
      LEGACY_PRODUCT_UI_BY_PROJECT[this.config.proj_n] || {
        display_name: "",
        product_url: "",
        update_url: "",
        ota_warning: "",
      }
    );
  }

  private getProductUi() {
    const fallback = this.getFallbackMetadata();
    const fw = this.config.kauf_ui || {};
    return {
      display_name: fw.display_name || fallback.display_name,
      product_url: fw.product_url || fallback.product_url,
      update_url: fw.update_url || fallback.update_url,
      ota_warning: fw.ota_warning || fallback.ota_warning,
      factory_warning: fw.factory_warning || "",
    };
  }

  renderUpdateLink() {
    const product = this.getProductUi();
    if (product.update_url) {
      return html`<br><a href="${product.update_url}" target="_blank" rel="noopener noreferrer">Check for Updates</a>`;
    }
    return html`<br>Project Name: ${this.config.proj_n || "unknown"}`;
  }

  renderOtaWarning() {
    const warning = this.getProductUi().ota_warning;
    return warning ? html`<p>**** ${warning}</p>` : nothing;
  }

  formatBytes(value?: number) {
    return Number(value || 0).toLocaleString("en-US");
  }

  ota() {
    if (this.config.ota) {
      let basePath = getBasePath();
      return html`<h2>OTA Update</h2>
        <p>Either .bin or .bin.gz files will work, but the file size needs to be under ${this.formatBytes(this.config.free_sp)} bytes.  For KAUF update files, the .bin.gz file is recommended, and the .bin files are typically too large to work anyway.</p>
        <p>**** DO NOT USE <b>TASMOTA-MINIMAL</b>.BIN or .BIN.GZ<br>**** Use tasmota.bin.gz or tasmota-lite.bin.gz</p>
        ${this.renderOtaWarning()}
        <form
          method="POST"
          action="${basePath}/update"
          enctype="multipart/form-data"
        >
          <input class="btn" type="file" name="update" />
          <input class="btn" type="submit" value="Update" />
        </form>`;
    }
  }

  renderComment() {
    return this.config.comment
      ? html`<h3>${this.config.comment}</h3>`
      : nothing;
  }

  renderLog() {
    return this.config.log
      ? html`<section class="col"><esp-log rows="50"></esp-log></section>`
      : nothing;
  }

  factory_reset() {
    const warning = this.getProductUi().factory_warning;
    return warning ? html`<p><b>${warning}</b></p>` : nothing;
  }

  clear() {
    if ( this.config.has_ap )
      return html `<p><a href="/clear" target="_blank">/clear</a> - Writes new software-configured Wi-Fi credentials (SSID: initial_ap, password: asdfasdfasdfasdf).  The device will reboot and try to connect to a network with those credentials.  If those credentials cannot be connected to, then this device will put up a Wi-Fi AP allowing new software-configured Wi-Fi credential to be entered.</p>`
  }

  render() {
    const product = this.getProductUi();
    const displayName = product.display_name ? ` ${product.display_name}` : "";
    const productUrl = product.product_url || "https://kaufha.com";
    const featuredName = this.config.featured_name || "";

    return html`
      <main class="flex-grid-half">
        <section class="col">
          <h1>
            ${this.config.title}
            <span id="beat" title="${this.version}">❤</span>
          </h1>
          <p><b>KAUF${displayName}</b> by <a href="${productUrl}" target="_blank" rel="noopener noreferrer">Kaufman Home Automation</a>
          <br>Firmware version ${this.config.proj_v} made using <a href="https://esphome.io" target="_blank" rel="noopener noreferrer">ESPHome</a> version ${this.config.esph_v}. ${this.renderUpdateLink()}</p>
          <p>See <a href="https://esphome.io/web-api/" target="_blank" rel="noopener noreferrer">ESPHome Web Server API</a> for REST API documentation.</p>
          ${this.renderComment()}
          ${featuredName
            ? html`<h2>Main Control Entity</h2>
                <esp-entity-table mode="hero" featured-name="${featuredName}"></esp-entity-table>`
            : html``}
          ${this.eventConnected
            ? nothing
            : html`<p class="event-warning"><b>Warning:</b> Live updates are disconnected. Refresh this page to see current status.</p>`}
          ${this.showReconnectNotice
            ? html`<p class="event-notice">
                Live updates reconnected. Refresh recommended if you recently updated the device.
                <button
                  class="event-notice-close"
                  @click=${() => (this.showReconnectNotice = false)}
                  aria-label="Dismiss notice"
                  title="Dismiss"
                >
                  ×
                </button>
              </p>`
            : nothing}
          <h2>Additional Entities</h2>
          <esp-entity-table mode="table" featured-name="${featuredName}"></esp-entity-table>
          <h2>
            <esp-switch
              color="var(--primary-color,currentColor)"
              class="right"
              .state="${this.scheme}"
              @state="${(e: CustomEvent) => (this.scheme = e.detail.state)}"
              labelOn="🌒"
              labelOff="☀️"
              stateOn="dark"
              stateOff="light"
              optimistic
            >
            </esp-switch>
            Color Scheme
          </h2>
          ${this.ota()}
          <h2>Web-Based Utilities</h2>
          <p><a href="/reset" target="_blank">/reset</a> - Erases all software-stored data, including the current states of all modifiable entities.  Firmware defaults will be restored.  If this device has a relay, the relay may toggle.  Any software-configured Wi-Fi credentials will be erased, but hard-coded credentials will not be erased.</p>
          ${this.factory_reset()}
          ${this.clear()}
          <h2>Device Parameters</h2>
          <table><tbody>
            <tr>
              <td>Hostname</td>
              <td>${this.config.hostname}</td>
            </tr>
            <tr>
              <td>MAC Address</td>
              <td>${this.config.mac_addr}</td>
            </tr>
            <tr>
              <td>Hard-Coded SSID</td>
              <td>${this.config.hard_ssid}</td>
            </tr>
            <tr>
              <td>Software-Configured SSID</td>
              <td>${this.config.soft_ssid}</td>
            </tr>
            <tr>
              <td>Firmware has AP?</td>
              <td>${this.config.has_ap}</td>
            </tr>
            <tr>
              <td>Free Space for OTA</td>
              <td>${this.formatBytes(this.config.free_sp)} bytes</td>
            </tr>
            ${this.config.build_ts
              ? html`<tr>
                  <td>Build Timestamp</td>
                  <td>${this.config.build_ts}</td>
                </tr>`
              : nothing}
            <tr>
              <td>Last API Call</td>
              <td>${this.lastApiCall || "-"}</td>
            </tr>
          </tbody></table>
        </section>
        ${this.renderLog()}
      </main>
    `;
  }

  static get styles() {
    return [
      cssReset,
      cssButton,
      css`
        .flex-grid {
          display: flex;
        }
        .flex-grid .col {
          flex: 2;
        }
        .flex-grid-half {
          display: flex;
          justify-content: space-evenly;
        }
        .col {
          width: 48%;
        }

        @media (max-width: 600px) {
          .flex-grid,
          .flex-grid-half {
            display: block;
          }
          .col {
            width: 100%;
            margin: 0 0 10px 0;
          }
        }

        * {
          box-sizing: border-box;
        }
        .flex-grid {
          margin: 0 0 20px 0;
        }
        h1 {
          text-align: center;
          width: 100%;
          line-height: 4rem;
        }
        h1,
        h2 {
          border-bottom: 1px solid #eaecef;
          margin-bottom: 0.25rem;
        }
        h3 {
          text-align: center;
          margin: 0.5rem 0;
        }
        .event-warning {
          color: #b00020;
          font-weight: 700;
          margin: 0.25rem 0 0.5rem;
        }
        .event-notice {
          position: relative;
          color: #666;
          font-weight: 600;
          margin: 0.25rem 0 0.5rem;
          padding-right: 1.5rem;
        }
        .event-notice-close {
          position: absolute;
          right: 0.25rem;
          top: 0;
          border: none;
          background: transparent;
          color: #666;
          font-size: 1.1rem;
          line-height: 1;
          cursor: pointer;
        }
        #beat {
          float: right;
          height: 1rem;
        }
        a.logo {
          height: 4rem;
          float: left;
          color: inherit;
        }
        .right {
          float: right;
        }
        table {
          border-spacing: 0;
          border-collapse: collapse;
          width: 100%;
          border: 1px solid currentColor;
        }

        th {
          font-weight: 600;
          text-align: left;
        }
        th,
        td {
          padding: 0.25rem 0.5rem;
          border: 1px solid currentColor;
        }
        td:nth-child(2),
        th:nth-child(2) {
          text-align: center;
        }
        tr th,
        tr:nth-child(2n) {
          background-color: rgba(127, 127, 127, 0.3);
        }
        select {
          background-color: inherit;
          color: inherit;
          width: 100%;
          border-radius: 4px;
        }
        option {
          color: currentColor;
          background-color: var(--primary-color, currentColor);
        }
        input[type="range"] {
          width: calc(100% - 4rem);
        }

      `,
    ];
  }
}
