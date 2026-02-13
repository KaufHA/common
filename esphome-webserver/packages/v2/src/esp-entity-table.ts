import { html, css, LitElement, nothing } from "lit";
import { keyed } from "lit/directives/keyed.js";
import { customElement, property, state } from "lit/decorators.js";
import cssReset from "./css/reset";
import cssButton from "./css/button";

interface entityConfig {
  unique_id: string;
  domain: string;
  id: string;
  state: string;
  detail: string;
  value: string;
  name: string;
  device?: string;  // Device name for hierarchical URLs (sub-devices only)
  when: string;
  icon?: string;
  option?: string[];
  assumed_state?: boolean;
  brightness?: number;
  color_mode?: string;
  color_temp?: number;
  color?: {
    r?: number;
    g?: number;
    b?: number;
    c?: number;
    w?: number;
  };
  min_mireds?: number;
  max_mireds?: number;
  target_temperature?: number;
  target_temperature_low?: number;
  target_temperature_high?: number;
  min_temp?: number;
  max_temp?: number;
  min_value?: number;
  max_value?: number;
  step?: number;
  min_length?: number;
  max_length?: number;
  pattern?: string;
  current_temperature?: number;
  modes?: number[];
  mode?: number;
  speed_count?: number;
  speed_level?: number;
  speed: string;
  effects?: string[];
  effect?: string;
  has_action?: boolean;
  // Water heater specific
  away?: boolean;
  is_on?: boolean;
  // Infrared specific
  supports_transmitter?: boolean;
  supports_receiver?: boolean;
}

export function getBasePath() {
  let str = window.location.pathname;
  return str.endsWith("/") ? str.slice(0, -1) : str;
}

let basePath = getBasePath();

// ID format detection and parsing helpers
// New format: "domain/entity_name" or "domain/device_name/entity_name"
// Old format: "domain-object_id" (deprecated)

function isNewIdFormat(id: string): boolean {
  return id.includes('/');
}

function parseDomainFromId(id: string): string {
  if (isNewIdFormat(id)) {
    return id.split('/')[0];
  }
  // Old format: domain-object_id
  return id.split('-')[0];
}

function buildEntityActionUrl(entity: entityConfig, action: string): string {
  if (isNewIdFormat(entity.unique_id)) {
    // New format: /{domain}/{device?}/{name}/{action}
    const entityName = encodeURIComponent(entity.name);
    const devicePart = entity.device
      ? `${encodeURIComponent(entity.device)}/`
      : '';
    return `${basePath}/${entity.domain}/${devicePart}${entityName}/${action}`;
  }
  // Old format: /{domain}/{object_id}/{action}
  const objectId = entity.unique_id.split('-').slice(1).join('-');
  return `${basePath}/${entity.domain}/${objectId}/${action}`;
}

interface RestAction {
  restAction(entity?: entityConfig, action?: string): void;
}

@customElement("esp-entity-table")
export class EntityTable extends LitElement implements RestAction {
  @state() entities: entityConfig[] = [];
  @state() has_controls: boolean = false;
  @state() activeMode: "rgb" | "ct" | "" = "";
  @state() uiHoldRgb: boolean = false;
  @state() uiRgb: { r: number; g: number; b: number } = { r: 0, g: 0, b: 0 };
  @property({ type: String, attribute: "featured-name" }) featuredName: string = "";
  @property({ type: String }) mode: "hero" | "table" = "table";

  private _actionRenderer = new ActionRenderer();

  connectedCallback() {
    super.connectedCallback();
    window.source?.addEventListener("state", (e: Event) => {
      const messageEvent = e as MessageEvent;
      const data = JSON.parse(messageEvent.data);
      // Prefer name_id (new format) over id (legacy format) for entity identification
      const entityId = data.name_id || data.id;
      let idx = this.entities.findIndex((x) => x.unique_id === entityId);
      if (idx === -1 && entityId) {
        // Dynamically add discovered entity
        // domain comes from JSON (new format) or parsed from id (old format)
        const domain = data.domain || parseDomainFromId(entityId);
        let entity = {
          ...data,
          domain: domain,
          unique_id: entityId,
        } as entityConfig;
        entity.has_action = this.hasAction(entity);
        if (entity.has_action) {
          this.has_controls = true;
        }
        this.entities.push(entity);
        this.entities.sort((a, b) => (a.name < b.name ? -1 : 1));
        this.requestUpdate();
      } else {
        delete data.id;
        delete data.name_id;
        delete data.domain;
        delete data.unique_id;
        Object.assign(this.entities[idx], data);
        this.requestUpdate();
      }
    });
  }

  hasAction(entity: entityConfig): boolean {
    return `render_${entity.domain}` in this._actionRenderer;
  }

  control(entity: entityConfig) {
    this._actionRenderer.entity = entity;
    this._actionRenderer.actioner = this;
    return this._actionRenderer.exec(
      `render_${entity.domain}` as ActionRendererMethodKey
    );
  }

  restAction(entity: entityConfig, action: string) {
    const path = buildEntityActionUrl(entity, action);
    const fullUrl = `${window.location.origin}${path}`;
    document.dispatchEvent(
      new CustomEvent("webserver-api-call", {
        detail: { method: "POST", url: fullUrl },
      })
    );
    fetch(path, {
      method: "POST",
      headers:{
        'Content-Type': 'application/x-www-form-urlencoded'
      },
    }).then((r) => {
      console.log(r);
    });
  }

  private isFeatured(entity: entityConfig): boolean {
    return !!this.featuredName && entity.name === this.featuredName;
  }

  private getFeaturedEntity(): entityConfig | undefined {
    return this.entities.find((e) => this.isFeatured(e));
  }

  private setActiveMode(mode: "rgb" | "ct") {
    if (this.activeMode !== mode) {
      this.activeMode = mode;
    }
  }

  private holdUiRgb() {
    this.uiHoldRgb = true;
    window.setTimeout(() => {
      this.uiHoldRgb = false;
    }, 600);
  }

  private getPreferredMode(entity: entityConfig): "rgb" | "ct" {
    const mode = (entity.color_mode || "").toLowerCase();
    if (mode.includes("rgb")) return "rgb";
    if (mode.includes("color_temp")) return "ct";
    if (this.activeMode) return this.activeMode;
    return "rgb";
  }

  private kelvinToRgb(kelvin: number): { r: number; g: number; b: number } {
    let temp = Math.min(40000, Math.max(1000, kelvin)) / 100;
    let r: number;
    let g: number;
    let b: number;
    if (temp <= 66) {
      r = 255;
      g = 99.4708025861 * Math.log(temp) - 161.1195681661;
      b = temp <= 19 ? 0 : 138.5177312231 * Math.log(temp - 10) - 305.0447927307;
    } else {
      r = 329.698727446 * Math.pow(temp - 60, -0.1332047592);
      g = 288.1221695283 * Math.pow(temp - 60, -0.0755148492);
      b = 255;
    }
    return {
      r: Math.min(255, Math.max(0, Math.round(r))),
      g: Math.min(255, Math.max(0, Math.round(g))),
      b: Math.min(255, Math.max(0, Math.round(b))),
    };
  }

  private setFeaturedColor(entity: entityConfig | undefined) {
    const root = document.documentElement;
    if (!entity || entity.domain !== "light") {
      root.style.setProperty("--featured-color", "transparent");
      root.style.setProperty("--featured-color-opacity", "0");
      root.style.setProperty("--featured-on", "0");
      root.style.setProperty("--featured-icon-visible", "0");
      return;
    }
    const isOn = entity.state === "ON";
    const r = entity.color?.r ?? 0;
    const g = entity.color?.g ?? 0;
    const b = entity.color?.b ?? 0;
    let color = `rgb(${r}, ${g}, ${b})`;
    const mode = (entity.color_mode || "").toLowerCase();
    const isCt = mode.includes("color_temp");
    if ((isCt || (!r && !g && !b)) && entity.color_temp) {
      const kelvin = Math.round(1000000 / entity.color_temp);
      const rgb = this.kelvinToRgb(kelvin);
      color = `rgb(${rgb.r}, ${rgb.g}, ${rgb.b})`;
    }
    root.style.setProperty("--featured-color", color);
    root.style.setProperty("--featured-color-opacity", isOn ? "1" : "0");
    root.style.setProperty("--featured-on", isOn ? "1" : "0");
    root.style.setProperty("--featured-icon-visible", "1");
  }

  private renderHero() {
    const entity = this.getFeaturedEntity();
    if (!entity) {
      this.setFeaturedColor(undefined);
      return nothing;
    }

    if (entity.domain !== "light") {
      this.setFeaturedColor(undefined);
      return this.renderEntityTable([entity]);
    }

    this.setFeaturedColor(entity);

    const r = entity.color?.r ?? 0;
    const g = entity.color?.g ?? 0;
    const b = entity.color?.b ?? 0;
    const brightness = entity.brightness ?? 0;
    const defaultMinMireds = Math.round(1000000 / 6600);
    const defaultMaxMireds = Math.round(1000000 / 2800);
    const minMireds =
      entity.min_mireds && entity.min_mireds > 0 ? entity.min_mireds : defaultMinMireds;
    const maxMireds =
      entity.max_mireds && entity.max_mireds > 0 ? entity.max_mireds : defaultMaxMireds;
    const colorTemp = entity.color_temp && entity.color_temp > 0 ? entity.color_temp : maxMireds;
    const kelvinMin = Math.round(1000000 / maxMireds);
    const kelvinMax = Math.round(1000000 / minMireds);
    const kelvinNowRaw = Math.round(1000000 / colorTemp);
    const kelvinNow = Math.min(kelvinMax, Math.max(kelvinMin, kelvinNowRaw));
    const dispBrightness = brightness;
    const dispR = this.uiHoldRgb ? this.uiRgb.r : r;
    const dispG = this.uiHoldRgb ? this.uiRgb.g : g;
    const dispB = this.uiHoldRgb ? this.uiRgb.b : b;
    const dispKelvin = kelvinNow;
    const ctInputVal = kelvinMax + kelvinMin - dispKelvin;
    const preferredMode = this.getPreferredMode(entity);

    const isOn = entity.state === "ON";
    const isRgb = isOn && preferredMode === "rgb";
    const isCt = isOn && preferredMode === "ct";
    const powerClass = isOn && (isRgb || isCt) ? "active" : "";
    const rgbInactiveClass = !isRgb ? "inactive" : "";
    const ctInactiveClass = !isCt ? "inactive" : "";
    return html`
      <div class="hero">
        <div class="hero-grid">
          <div
            class="hero-col ${powerClass}"
            @click="${(e: Event) => {
              const target = e.target as HTMLElement;
              if (target && target.tagName === "INPUT") return;
              const next = entity.state === "ON" ? "turn_off" : "turn_on";
              this.restAction(entity, next);
            }}"
          >
            <div class="hero-col-title">Power</div>
            <div class="hero-row">
              <esp-switch
                color="var(--primary-color,currentColor)"
                .state=${entity.state}
                @state="${(e: CustomEvent) => {
                  let act = "turn_" + e.detail.state;
                  this.restAction(entity, act.toLowerCase());
                }}"
              ></esp-switch>
            </div>
            <div class="hero-slider hero-slider-stacked">
              <div class="hero-col-title">Brightness</div>
              <div class="hero-slider-row">
                <input
                  type="range"
                  min="1"
                  max="255"
                  step="1"
                  .value="${dispBrightness}"
                  @click="${(e: Event) => e.stopPropagation()}"
                  @change="${(e: Event) => {
                    e.stopPropagation();
                    const val = Number((e.target as HTMLInputElement).value);
                    this.restAction(entity, `turn_on?brightness=${val}`);
                  }}"
                />
                <span class="hero-slider-value">${dispBrightness}</span>
              </div>
            </div>
          </div>
          <div
            class="hero-col ${isRgb ? "active" : ""} ${rgbInactiveClass}"
            @click="${(e: Event) => {
              const target = e.target as HTMLElement;
              if (target && target.tagName === "INPUT") return;
              this.setActiveMode("rgb");
              this.restAction(entity, "turn_on?color_mode=rgb");
            }}"
          >
            <div class="hero-col-title">RGB</div>
            <div class="hero-slider">
              <label>R</label>
              <input
                  type="range"
                  min="0"
                  max="255"
                  step="1"
                  .value="${dispR}"
                  @click="${(e: Event) => e.stopPropagation()}"
                  @mousedown="${(e: Event) => {
                    if (!isRgb) {
                      e.preventDefault();
                      e.stopPropagation();
                      this.setActiveMode("rgb");
                      this.restAction(entity, "turn_on?color_mode=rgb");
                    }
                  }}"
                  @touchstart="${(e: Event) => {
                    if (!isRgb) {
                      e.preventDefault();
                      e.stopPropagation();
                      this.setActiveMode("rgb");
                      this.restAction(entity, "turn_on?color_mode=rgb");
                    }
                  }}"
                  @change="${(e: Event) => {
                    e.stopPropagation();
                    if (!isRgb) return;
                    const val = Number((e.target as HTMLInputElement).value);
                    this.uiRgb = { r: val, g: dispG, b: dispB };
                    this.holdUiRgb();
                    this.setActiveMode("rgb");
                    this.restAction(entity, `turn_on?r=${val}&g=${dispG}&b=${dispB}`);
                  }}"
                />
              <span class="hero-slider-value">${isRgb ? dispR : "—"}</span>
            </div>
            <div class="hero-slider">
              <label>G</label>
              <input
                  type="range"
                  min="0"
                  max="255"
                  step="1"
                  .value="${dispG}"
                  @click="${(e: Event) => e.stopPropagation()}"
                  @mousedown="${(e: Event) => {
                    if (!isRgb) {
                      e.preventDefault();
                      e.stopPropagation();
                      this.setActiveMode("rgb");
                      this.restAction(entity, "turn_on?color_mode=rgb");
                    }
                  }}"
                  @touchstart="${(e: Event) => {
                    if (!isRgb) {
                      e.preventDefault();
                      e.stopPropagation();
                      this.setActiveMode("rgb");
                      this.restAction(entity, "turn_on?color_mode=rgb");
                    }
                  }}"
                  @change="${(e: Event) => {
                    e.stopPropagation();
                    if (!isRgb) return;
                    const val = Number((e.target as HTMLInputElement).value);
                    this.uiRgb = { r: dispR, g: val, b: dispB };
                    this.holdUiRgb();
                    this.setActiveMode("rgb");
                    this.restAction(entity, `turn_on?r=${dispR}&g=${val}&b=${dispB}`);
                  }}"
                />
              <span class="hero-slider-value">${isRgb ? dispG : "—"}</span>
            </div>
            <div class="hero-slider">
              <label>B</label>
              <input
                  type="range"
                  min="0"
                  max="255"
                  step="1"
                  .value="${dispB}"
                  @click="${(e: Event) => e.stopPropagation()}"
                  @mousedown="${(e: Event) => {
                    if (!isRgb) {
                      e.preventDefault();
                      e.stopPropagation();
                      this.setActiveMode("rgb");
                      this.restAction(entity, "turn_on?color_mode=rgb");
                    }
                  }}"
                  @touchstart="${(e: Event) => {
                    if (!isRgb) {
                      e.preventDefault();
                      e.stopPropagation();
                      this.setActiveMode("rgb");
                      this.restAction(entity, "turn_on?color_mode=rgb");
                    }
                  }}"
                  @change="${(e: Event) => {
                    e.stopPropagation();
                    if (!isRgb) return;
                    const val = Number((e.target as HTMLInputElement).value);
                    this.uiRgb = { r: dispR, g: dispG, b: val };
                    this.holdUiRgb();
                    this.setActiveMode("rgb");
                    this.restAction(entity, `turn_on?r=${dispR}&g=${dispG}&b=${val}`);
                  }}"
                />
              <span class="hero-slider-value">${isRgb ? dispB : "—"}</span>
            </div>
          </div>
          <div
            class="hero-col ${isCt ? "active" : ""} ${ctInactiveClass}"
            @click="${(e: Event) => {
              const target = e.target as HTMLElement;
              if (target && target.tagName === "INPUT") return;
              this.setActiveMode("ct");
              this.restAction(entity, "turn_on?color_mode=color_temp");
            }}"
          >
            <div class="hero-col-title">Color Temp</div>
            <div class="hero-slider hero-slider-ct">
              <input
                  type="range"
                  min="${kelvinMin}"
                  max="${kelvinMax}"
                  step="1"
                  .value="${ctInputVal}"
                  @click="${(e: Event) => e.stopPropagation()}"
                  @mousedown="${(e: Event) => {
                    if (!isCt) {
                      e.preventDefault();
                      e.stopPropagation();
                      this.setActiveMode("ct");
                      this.restAction(entity, "turn_on?color_mode=color_temp");
                    }
                  }}"
                  @touchstart="${(e: Event) => {
                    if (!isCt) {
                      e.preventDefault();
                      e.stopPropagation();
                      this.setActiveMode("ct");
                      this.restAction(entity, "turn_on?color_mode=color_temp");
                    }
                  }}"
                  @change="${(e: Event) => {
                    e.stopPropagation();
                    if (!isCt) return;
                    const val = Number((e.target as HTMLInputElement).value);
                    const kelvin = kelvinMax + kelvinMin - val;
                    this.setActiveMode("ct");
                    const mired = Math.round(1000000 / kelvin);
                    this.restAction(entity, `turn_on?color_temp=${mired}`);
                  }}"
                />
              <span class="hero-slider-value-k">${isCt ? `${dispKelvin}K` : "—"}</span>
            </div>
          </div>
        </div>
      </div>
    `;
  }

  private renderTable() {
    const rows = this.entities.filter((e) => !this.isFeatured(e));
    return this.renderEntityTable(rows);
  }

  private renderEntityTable(rows: entityConfig[]) {
    const showControls = rows.some((e) => e.has_action);
    return html`
      <table>
        <thead>
          <tr>
            <th>Name</th>
            <th>State</th>
            ${showControls ? html`<th>Actions</th>` : html``}
          </tr>
        </thead>
        <tbody>
          ${rows.map(
            (component) => html`
              <tr>
                <td>${component.device ? `[${component.device}] ` : ""}${component.name}</td>
                <td>${component.state}</td>
                ${showControls
                  ? html`<td>
                      ${component.has_action ? this.control(component) : html``}
                    </td>`
                  : html``}
              </tr>
            `
          )}
        </tbody>
      </table>
    `;
  }

  render() {
    if (this.mode === "hero") {
      return this.renderHero();
    }
    return this.renderTable();
  }

  static get styles() {
    return [
      cssReset,
      cssButton,
      css`
        table {
          border-spacing: 0;
          border-collapse: collapse;
          width: 100%;
          border: 1px solid currentColor;
          background-color: var(--c-bg);
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
          width: 100%;
          border-radius: 4px;
        }
        input[type="range"], input[type="text"] {
          width: calc(100% - 8rem);
          min-width: 5rem;
          height: 0.75rem;
        }
        .range, .text {
          text-align: center;
        }
        .hero {
          padding: 0.5rem 0;
        }
        .hero.empty {
          color: rgba(127, 127, 127, 0.8);
        }
        .hero-grid {
          display: grid;
          grid-template-columns: repeat(3, minmax(0, 1fr));
          gap: 0.5rem;
          width: 100%;
          margin: 0;
        }
        .hero-col {
          border: 1px solid rgba(127, 127, 127, 0.5);
          border-radius: 4px;
          padding: 0.5rem;
          background: rgba(127, 127, 127, 0.1);
        }
        .hero-col.inactive {
          opacity: 0.55;
          filter: grayscale(0.2);
        }
        .hero-col.inactive input[type="range"] {
          accent-color: rgba(127, 127, 127, 0.7);
          pointer-events: none;
        }
        .hero-col.inactive input[type="range"]:hover {
          accent-color: rgba(127, 127, 127, 0.7);
        }
        .hero-col.active {
          border-color: var(--primary-color, currentColor);
          background: rgba(3, 169, 244, 0.15);
        }
        .hero-col-title {
          font-weight: 600;
          margin-bottom: 0.25rem;
          user-select: none;
        }
        .hero-row {
          display: flex;
          align-items: center;
          justify-content: space-between;
          gap: 0.5rem;
        }
        .hero-slider {
          display: grid;
          grid-template-columns: 2ch minmax(0, 1fr) 3ch;
          align-items: center;
          gap: 0.5rem;
          margin-top: 0.25rem;
        }
        .hero-slider-ct {
          grid-template-columns: minmax(0, 1fr) 5ch;
        }
        .hero-slider input[type="range"] {
          width: 100%;
          height: 0.75rem;
        }
        .hero-slider label {
          text-align: right;
          display: inline-block;
          width: 2ch;
        }
        .hero-slider span {
          display: inline-block;
          text-align: right;
        }
        .hero-slider-value {
          width: 3ch;
        }
        .hero-slider-value-k {
          width: 5ch;
        }
        .hero-slider-stacked {
          display: block;
        }
        .hero-slider-row {
          display: grid;
          grid-template-columns: minmax(0, 1fr) 3ch;
          align-items: center;
          gap: 0.5rem;
        }
        .hero-actions {
          display: flex;
          gap: 0.25rem;
          align-items: center;
        }
        @media (max-width: 800px) {
          .hero-grid {
            grid-template-columns: 1fr;
          }
        }
      `,
    ];
  }
}

type ActionRendererNonCallable = "entity" | "actioner" | "exec";
type ActionRendererMethodKey = keyof Omit<
  ActionRenderer,
  ActionRendererNonCallable
>;

class ActionRenderer {
  public entity?: entityConfig;
  public actioner?: RestAction;

  exec(method: ActionRendererMethodKey) {
    if (!this[method] || typeof this[method] !== "function") {
      console.log(`ActionRenderer.${method} is not callable`);
      return;
    }
    return this[method]();
  }

  private _actionButton(entity: entityConfig, label: string, action: string) {
    if (!entity) return;
    let a = action || label.toLowerCase();
    return html`<button
      class="rnd"
      @click=${() => this.actioner?.restAction(entity, a)}
    >
      ${label}
    </button>`;
  }

  private _switch(entity: entityConfig) {
    return html`<esp-switch
      color="var(--primary-color,currentColor)"
      .state=${entity.state}
      @state="${(e: CustomEvent) => {
        let act = "turn_" + e.detail.state;
        this.actioner?.restAction(entity, act.toLowerCase());
      }}"
    ></esp-switch>`;
  }

  private _select(
    entity: entityConfig,
    action: string,
    opt: string,
    options: string[] | number[],
    val: string | number | undefined
  ) {
    return html`<select
      @change="${(e: Event) => {
        let val = e.target?.value;
        this.actioner?.restAction(
          entity,
          `${action}?${opt}=${encodeURIComponent(val)}`
        );
      }}"
    >
      ${options.map(
        (option) =>
          html`
            <option value="${option}" ?selected="${option == val}">
              ${option}
            </option>
          `
      )}
    </select>`;
  }

  private _range(
    entity: entityConfig,
    action: string,
    opt: string,
    value: string | number,
    min: number | undefined,
    max: number | undefined,
    step: number | undefined
  ) {
    return html`<div class="range">
      <label>${min || 0}</label>
      <input
        type="${entity.mode == 1 ? "number" : "range"}"
        name="${entity.unique_id}"
        id="${entity.unique_id}"
        step="${step || 1}"
        min="${min || Math.min(0, value as number)}"
        max="${max || Math.max(10, value as number)}"
        .value="${value!}"
        @change="${(e: Event) => {
          let val = e.target?.value;
          this.actioner?.restAction(entity, `${action}?${opt}=${val}`);
        }}"
      />
      <label>${max || 100}</label>
    </div>`;
  }

  private _datetime(
    entity: entityConfig,
    type: string,
    action: string,
    opt: string,
    value: string,
  ) {
    return html`
      <input 
        type="${type}" 
        name="${entity.unique_id}"
        id="${entity.unique_id}"
        .value="${value}"
        @change="${(e: Event) => {
          const val = (<HTMLTextAreaElement>e.target)?.value;
          this.actioner?.restAction(
            entity,
            `${action}?${opt}=${val.replace('T', ' ')}`
          );
        }}"
      />
    `;
  }

  private _textinput(
    entity: entityConfig,
    action: string,
    opt: string,
    value: string | number,
    min: number | undefined,
    max: number | undefined,
    pattern: string | undefined
  ) {
    return html`<div class="text">
      <input
        type="${entity.mode == 1 ? "password" : "text"}"
        name="${entity.unique_id}"
        id="${entity.unique_id}"
        minlength="${min || Math.min(0, value as number)}"
        maxlength="${max || Math.max(255, value as number)}"
        pattern="${pattern || ''}"
        .value="${value!}"
        @change="${(e: Event) => {
          let val = e.target?.value;
          this.actioner?.restAction(entity, `${action}?${opt}=${encodeURIComponent(val)}`);
        }}"
      />
    </div>`;
  }

  render_switch() {
    if (!this.entity) return;
    if (this.entity.assumed_state)
      return html`${this._actionButton(this.entity, "❌", "turn_off")}
      ${this._actionButton(this.entity, "✔️", "turn_on")}`;
    else return this._switch(this.entity);
  }

  render_fan() {
    if (!this.entity) return;
    return [
      this.entity.speed,
      " ",
      this.entity.speed_level,
      this._switch(this.entity),
      this.entity.speed_count
        ? this._range(
            this.entity,
            `turn_${this.entity.state.toLowerCase()}`,
            "speed_level",
            this.entity.speed_level ? this.entity.speed_level : 0,
            0,
            this.entity.speed_count,
            1
          )
        : "",
    ];
  }

  render_light() {
    if (!this.entity) return;
    return [
      this._switch(this.entity),
      this.entity.brightness
        ? this._range(
            this.entity,
            "turn_on",
            "brightness",
            this.entity.brightness,
            0,
            255,
            1
          )
        : "",
      this.entity.effects?.filter((v) => v != "None").length
        ? this._select(
            this.entity,
            "turn_on",
            "effect",
            this.entity.effects || [],
            this.entity.effect
          )
        : "",
    ];
  }

  render_lock() {
    if (!this.entity) return;
    return html`${this._actionButton(this.entity, "🔐", "lock")}
    ${this._actionButton(this.entity, "🔓", "unlock")}
    ${this._actionButton(this.entity, "↑", "open")} `;
  }

  render_cover() {
    if (!this.entity) return;
    return html`${this._actionButton(this.entity, "↑", "open")}
    ${this._actionButton(this.entity, "☐", "stop")}
    ${this._actionButton(this.entity, "↓", "close")}`;
  }

  render_button() {
    if (!this.entity) return;
    return html`${this._actionButton(this.entity, "☐", "press ")}`;
  }

  render_select() {
    if (!this.entity) return;
    return this._select(
      this.entity,
      "set",
      "option",
      this.entity.option || [],
      this.entity.value
    );
  }

  render_number() {
    if (!this.entity) return;
    return this._range(
      this.entity,
      "set",
      "value",
      this.entity.value,
      this.entity.min_value,
      this.entity.max_value,
      this.entity.step
    );
  }

  render_date() {
    if (!this.entity) return;
    return html`
      ${this._datetime(
        this.entity,
        "date",
        "set",
        "value",
        this.entity.value,
      )}
    `;
  }

  render_time() {
    if (!this.entity) return;
    return html`
      ${this._datetime(
        this.entity,
        "time",
        "set",
        "value",
        this.entity.value,
      )}
    `;
  }

  render_datetime() {
    if (!this.entity) return;
    return html`
      ${this._datetime(
        this.entity,
        "datetime-local",
        "set",
        "value",
        this.entity.value,
      )}
    `;
  }

  render_text() {
    if (!this.entity) return;
    return this._textinput(
      this.entity,
      "set",
      "value",
      this.entity.value,
      this.entity.min_length,
      this.entity.max_length,
      this.entity.pattern,
    );
  }

  render_climate() {
    if (!this.entity) return;
    let target_temp_slider, target_temp_label;
    if (
      this.entity.target_temperature_low !== undefined &&
      this.entity.target_temperature_high !== undefined
    ) {
      target_temp_label = html`${this.entity
        .target_temperature_low}&nbsp;..&nbsp;${this.entity
        .target_temperature_high}`;
      target_temp_slider = html`
        ${this._range(
          this.entity,
          "set",
          "target_temperature_low",
          this.entity.target_temperature_low,
          this.entity.min_temp,
          this.entity.max_temp,
          this.entity.step
        )}
        ${this._range(
          this.entity,
          "set",
          "target_temperature_high",
          this.entity.target_temperature_high,
          this.entity.min_temp,
          this.entity.max_temp,
          this.entity.step
        )}
      `;
    } else {
      target_temp_label = html`${this.entity.target_temperature}`;
      target_temp_slider = html`
        ${this._range(
          this.entity,
          "set",
          "target_temperature",
          this.entity.target_temperature!!,
          this.entity.min_temp,
          this.entity.max_temp,
          this.entity.step
        )}
      `;
    }
    let modes = html``;
    if ((this.entity.modes ? this.entity.modes.length : 0) > 0) {
      modes = html`Mode:<br />
        ${this._select(
          this.entity,
          "set",
          "mode",
          this.entity.modes || [],
          this.entity.mode || ""
        )}`;
    }
    return html`
      <label
        >Current:&nbsp;${this.entity.current_temperature},
        Target:&nbsp;${target_temp_label}</label
      >
      ${target_temp_slider} ${modes}
    `;
  }
  render_valve() {
    if (!this.entity) return;
    return html`${this._actionButton(this.entity, "| |", "open")}
    ${this._actionButton(this.entity, "☐", "stop")}
    ${this._actionButton(this.entity, "|-|", "close")}`;
  }
  render_water_heater() {
    if (!this.entity) return;
    let target_temp_slider = html``;
    let target_temp_label = html``;
    let has_target = false;
    if (
      this.entity.target_temperature_low !== undefined &&
      this.entity.target_temperature_high !== undefined
    ) {
      has_target = true;
      target_temp_label = html`Target:&nbsp;${this.entity
        .target_temperature_low}&nbsp;..&nbsp;${this.entity
        .target_temperature_high}`;
      target_temp_slider = html`
        ${this._range(
          this.entity,
          "set",
          "target_temperature_low",
          this.entity.target_temperature_low,
          this.entity.min_temp,
          this.entity.max_temp,
          this.entity.step
        )}
        ${this._range(
          this.entity,
          "set",
          "target_temperature_high",
          this.entity.target_temperature_high,
          this.entity.min_temp,
          this.entity.max_temp,
          this.entity.step
        )}
      `;
    } else if (this.entity.target_temperature !== undefined) {
      has_target = true;
      target_temp_label = html`Target:&nbsp;${this.entity.target_temperature}`;
      target_temp_slider = html`
        ${this._range(
          this.entity,
          "set",
          "target_temperature",
          this.entity.target_temperature,
          this.entity.min_temp,
          this.entity.max_temp,
          this.entity.step
        )}
      `;
    }
    let modes = html``;
    if ((this.entity.modes ? this.entity.modes.length : 0) > 0) {
      modes = html`Mode:<br />
        ${this._select(
          this.entity,
          "set",
          "mode",
          this.entity.modes || [],
          this.entity.state || ""
        )}`;
    }
    // Away mode toggle (if supported)
    let away = this.entity.away !== undefined
      ? html`Away:&nbsp;${this._actionButton(
          this.entity,
          this.entity.away ? "ON" : "OFF",
          `set?away=${!this.entity.away}`
        )}<br />`
      : html``;
    // On/Off toggle (if supported)
    let on_off = this.entity.is_on !== undefined
      ? html`Power:&nbsp;${this._actionButton(
          this.entity,
          this.entity.is_on ? "ON" : "OFF",
          `set?is_on=${!this.entity.is_on}`
        )}<br />`
      : html``;
    const has_current = this.entity.current_temperature !== undefined;
    let current_temp = has_current
      ? html`Current:&nbsp;${this.entity.current_temperature}`
      : html``;
    return html`
      <label>${current_temp}${has_current && has_target ? ', ' : ''}${target_temp_label}</label>
      ${target_temp_slider} ${modes} ${away} ${on_off}
    `;
  }

  render_infrared() {
    if (!this.entity) return;

    // Only show transmit UI if entity supports transmitter
    if (this.entity.supports_transmitter !== true) {
      return nothing;
    }

    const entity = this.entity;

    // Helper to encode timings array to base64url
    const encodeTimings = (timingsStr: string): string => {
      const timings = timingsStr.split(',').map(s => parseInt(s.trim(), 10)).filter(n => !isNaN(n));
      const buffer = new ArrayBuffer(timings.length * 4);
      const view = new DataView(buffer);
      timings.forEach((val, i) => view.setInt32(i * 4, val, true)); // little-endian
      const bytes = new Uint8Array(buffer);
      let binary = '';
      bytes.forEach(b => binary += String.fromCharCode(b));
      // Convert to base64url: replace + with -, / with _, remove padding =
      return btoa(binary).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
    };

    const handleTransmit = (e: Event) => {
      const button = e.currentTarget as HTMLElement;
      const container = button.parentElement;
      if (!container) {
        console.error('Infrared: Could not find container');
        return;
      }

      const carrierInput = container.querySelector('input[data-field="carrier"]') as HTMLInputElement;
      const repeatInput = container.querySelector('input[data-field="repeat"]') as HTMLInputElement;
      const timingsInput = container.querySelector('input[data-field="timings"]') as HTMLInputElement;

      if (!carrierInput || !repeatInput || !timingsInput) {
        console.error('Infrared: Could not find input elements', { carrierInput, repeatInput, timingsInput });
        return;
      }

      const carrier = carrierInput.value || '38000';
      const repeat = repeatInput.value || '1';
      const timingsRaw = timingsInput.value || '';

      if (!timingsRaw.trim()) {
        console.warn('Infrared: No timings provided');
        return;
      }

      const timingsEncoded = encodeTimings(timingsRaw);
      console.log('Infrared: Transmitting', { carrier, repeat, timingsRaw, timingsEncoded });

      // Build URL for transmit action (without query params - data goes in body)
      const url = buildEntityActionUrl(entity, 'transmit');

      // Send data in POST body to avoid URI Too Long error
      const body = new URLSearchParams();
      body.append('carrier_frequency', carrier);
      body.append('repeat_count', repeat);
      body.append('data', timingsEncoded);

      fetch(url, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded'
        },
        body: body.toString()
      }).then(r => {
        console.log('Infrared: Transmit response', r);
      }).catch(err => {
        console.error('Infrared: Transmit error', err);
      });
    };

    return html`
      <div class="infrared-wrap">
        <label>Carrier (Hz):&nbsp;</label>
        <input
          type="number"
          data-field="carrier"
          value="38000"
          min="1000"
          max="100000"
          style="width: 80px"
        /><br />
        <label>Repeat:&nbsp;</label>
        <input
          type="number"
          data-field="repeat"
          value="1"
          min="1"
          max="100"
          style="width: 50px"
        /><br />
        <label>Timings:&nbsp;</label>
        <input
          type="text"
          data-field="timings"
          placeholder="e.g. 9000,-4500,560,-560,..."
          style="width: calc(100% - 8rem); min-width: 10rem"
        /><br />
        <button class="rnd" @click=${handleTransmit}>TX</button>
      </div>
    `;
  }
}
