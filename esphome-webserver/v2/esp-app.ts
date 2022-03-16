import { LitElement, html, css } from "lit";
import { customElement, state, query } from "lit/decorators.js";

import "./esp-entity-table";
import "./esp-log";
import "./esp-switch";
import cssReset from "./css/reset";
import cssButton from "./css/button";

window.source = new EventSource("/events");

@customElement("esp-app")
export default class EspApp extends LitElement {
  @state({ type: String }) scheme = "";
  @state({ type: String }) ping = "";
  @query("#beat")
  beat!: HTMLSpanElement;

  version: String = import.meta.env.PACKAGE_VERSION;
  config: Object = { ota: false, title: "" };

  darkQuery: MediaQueryList = window.matchMedia("(prefers-color-scheme: dark)");

  frames = [
    { color: "inherit" },
    { color: "red", transform: "scale(1.25) translateY(-30%)" },
    { color: "inherit" },
  ];

  constructor() {
    super();
  }

  firstUpdated(changedProperties) {
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
        const config = JSON.parse(messageEvent.data);
        this.config = config;

        document.title = config.title;
        document.documentElement.lang = config.lang;
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
    if ( this.config.proj_n == "kauf.plf10")
      return "Plug";
    else if ( this.config.proj_n == "kauf.rgbww")
      return "RGBWW Bulb";
    else
      return "";
  }

  kauf_p_url() {
    if ( this.config.proj_n == "kauf.plf10")
      return "plf10";
    else if ( this.config.proj_n == "kauf.rgbww")
      return "blf10";
    else
      return "";
  }

  kauf_p_up() {
    if ( this.config.proj_n == "kauf.plf10")
      return html`<br><a href="https://github.com/KaufHA/PLF10/releases" target="_blank" rel="noopener noreferrer">Check for Updates</a>`;
    else if ( this.config.proj_n == "kauf.rgbww")
      return html`<br><a href="https://github.com/KaufHA/kauf-rgbww-bulbs/releases" target="_blank" rel="noopener noreferrer">Check for Updates</a>`;
    else
      return html`<br>Project Name: ${this.config.proj_n}`;
  }

  kauf_ota_extra() {
    if ( this.config.proj_n == "kauf.rgbww")
      return html`<p>**** DO NOT USE ANY <b>WLED</b> BIN file.<br>**** WLED is not going to work properly on this bulb.<br>**** Use the included DDP functionality to control this bulb from another WLED instance or xLights.</p>`;
    else
      return "";

  }

  ota() {
    if (this.config.ota)
      return html`<h2>OTA Update</h2>
        <p>**** DO NOT USE <b>TASMOTA-MINIMAL</b>.BIN or .BIN.GZ<br>**** Use tasmota.bin.gz or tasmota-lite.bin.gz</p>
        ${this.kauf_ota_extra()}
        <form method="POST" action="/update" enctype="multipart/form-data">
          <input class="btn" type="file" name="update" />
          <input class="btn" type="submit" value="Update" />
        </form>`;
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
            >
            </esp-switch>
            Color Scheme
          </h2>
          ${this.ota()}
        </section>
        <section class="col">
          <esp-log rows="50"></esp-log>
        </section>
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
          float:right
        }
      `,
    ];
  }
}
