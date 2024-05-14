import { LitElement, html, css, PropertyValues, nothing } from "lit";
import { customElement, state, query } from "lit/decorators.js";
import { getBasePath } from "./esp-entity-table";

import "./esp-entity-table";
import "./esp-log";
import "./esp-switch";
import cssReset from "./css/reset";
import cssButton from "./css/button";

window.source = new EventSource(getBasePath() + "/events");

interface Config {
  ota: boolean;
  log: boolean;
  title: string;
  comment: string;
}

@customElement("esp-app")
export default class EspApp extends LitElement {
  @state() scheme: string = "";
  @state() ping: string = "";
  @query("#beat")
  beat!: HTMLSpanElement;

  version: String = import.meta.env.PACKAGE_VERSION;
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
    document.documentElement.lang = config.lang;
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
      const d: String = messageEvent.data;
      if (d.length) {
        this.setConfig(JSON.parse(messageEvent.data));
      }
      this.ping = messageEvent.lastEventId;
    });
    window.source.onerror = function (e: Event) {
      console.dir(e);
      //alert("Lost event stream!")
    };
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

  kauf_p_name() {
    if ( this.config.proj_n == "Kauf.PLF10")
      return "Plug";
    else if ( this.config.proj_n == "Kauf.PLF12")
      return "Plug";
    else if ( this.config.proj_n == "Kauf.RGBWW")
      return "RGBWW Bulb";
    else if ( this.config.proj_n == "Kauf.RGBSw")
      return "RGB Switch";
    else
      return "";
  }

  kauf_p_url() {
    if ( this.config.proj_n == "Kauf.PLF10")
      return "plf10";
    else if ( this.config.proj_n == "Kauf.PLF12")
      return "plf12";
    else if ( this.config.proj_n == "Kauf.RGBWW")
      return "blf10";
    else if ( this.config.proj_n == "Kauf.RGBSw")
      return "srf10";
    else
      return "";
  }

  kauf_p_up() {
    if ( this.config.proj_n == "Kauf.PLF10")
      return html`<br><a href="https://github.com/KaufHA/PLF10/releases" target="_blank" rel="noopener noreferrer">Check for Updates</a>`;
    else if ( this.config.proj_n == "Kauf.PLF12")
      return html`<br><a href="https://github.com/KaufHA/PLF12/releases" target="_blank" rel="noopener noreferrer">Check for Updates</a>`;
    else if ( this.config.proj_n == "Kauf.RGBWW")
      return html`<br><a href="https://github.com/KaufHA/kauf-rgbww-bulbs/releases" target="_blank" rel="noopener noreferrer">Check for Updates</a>`;
    else if ( this.config.proj_n == "Kauf.RGBSw")
      return html`<br><a href="https://github.com/KaufHA/kauf-rgb-switch/releases" target="_blank" rel="noopener noreferrer">Check for Updates</a>`;
    else
      return html`<br>Project Name: ${this.config.proj_n}`;
  }

  kauf_ota_extra() {
    if ( this.config.proj_n == "Kauf.RGBWW")
      return html`<p>**** DO NOT USE ANY <b>WLED</b> BIN file.<br>**** WLED is not going to work properly on this bulb.<br>**** Use the included DDP functionality to control this bulb from another WLED instance or xLights.</p>`;
    else
      return "";
  }

  ota() {
    if (this.config.ota) {
      let basePath = getBasePath();
      return html`<h2>OTA Update</h2>
        <p>Either .bin or .bin.gz files will work, but the file size needs to be under ${this.config.free_sp} bytes.  For KAUF update files, the .bin.gz file is recommended, and the .bin files are typically too large to work anyway.</p>
        <p>**** DO NOT USE <b>TASMOTA-MINIMAL</b>.BIN or .BIN.GZ<br>**** Use tasmota.bin.gz or tasmota-lite.bin.gz</p>
        ${this.kauf_ota_extra()}
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
    if ( this.config.proj_l == "f" )
      return html `<p><b> For factory images, with version suffix (f)</b>, /reset will place the firmware into factory test mode.  Factory test mode can typically be cleared easily by pressing a button on the device after a few seconds.  For bulbs, factory test mode will automatically stop after 10 minutes or can be cleared through the web interface by pressing the "Stop Factory Routine" button once you get the bulb connected back to Wi-Fi.  You may need to refresh the page to see this button.</p>`
  }

  clear() {
    if ( this.config.has_ap )
      return html `<p><a href="/clear" target="_blank">/clear</a> - Writes new software-configured Wi-Fi credentials (SSID: initial_ap, password: asdfasdfasdfasdf).  The device will reboot and try to connect to a network with those credentials.  If those credentials cannot be connected to, then this device will put up a Wi-Fi AP allowing new software-configured Wi-Fi credential to be entered.</p>`
  }

  render() {
    return html`
      <main class="flex-grid-half">
        <section class="col">
          <h1>
            ${this.config.title}
            <span id="beat" title="${this.version}">❤</span>
          </h1>
          <p><b>KAUF ${this.kauf_p_name()}</b> by <a href="https://kaufha.com/${this.kauf_p_url()}" target="_blank" rel="noopener noreferrer">Kaufman Home Automation</a>
          <br>Firmware version ${this.config.proj_v} made using <a href="https://esphome.io" target="_blank" rel="noopener noreferrer">ESPHome</a> version ${this.config.esph_v}. ${this.kauf_p_up()}</p>
          <p>See <a href="https://esphome.io/web-api/" target="_blank" rel="noopener noreferrer">ESPHome Web Server API</a> for REST API documentation.</p>
          ${this.renderComment()}
          <h2>Entity Table</h2>
          <esp-entity-table></esp-entity-table>
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
              <td>${this.config.title}</td>
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
              <td>${this.config.free_sp} bytes</td>
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
