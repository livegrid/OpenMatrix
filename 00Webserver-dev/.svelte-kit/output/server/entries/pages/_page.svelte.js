import { c as create_ssr_component, b as createEventDispatcher, d as add_attribute, e as escape, a as subscribe, v as validate_component, m as missing_component, f as each } from "../../chunks/index2.js";
import "theme-change";
import { colord, extend } from "colord";
import a11yPlugin from "colord/plugins/a11y";
import { B as BROWSER, d as derived, w as writable } from "../../chunks/index.js";
const browser = BROWSER;
const app = "";
const styles = "";
const Toggle = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  createEventDispatcher();
  let value = false;
  let { command = "interaction" } = $$props;
  let { param = "null" } = $$props;
  if ($$props.command === void 0 && $$bindings.command && command !== void 0)
    $$bindings.command(command);
  if ($$props.param === void 0 && $$bindings.param && param !== void 0)
    $$bindings.param(param);
  return `<input type="checkbox"${add_attribute("command", command, 0)}${add_attribute("param", param, 0)} class="toggle toggle-sm"${add_attribute("checked", value, 1)}>`;
});
const Slider$1 = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  createEventDispatcher();
  let value = 50;
  let { command = "interaction" } = $$props;
  let { face = "null" } = $$props;
  let { param = "null" } = $$props;
  let { minValue = 0 } = $$props;
  let { maxValue = 100 } = $$props;
  let { classSlider = "range range-primary" } = $$props;
  if ($$props.command === void 0 && $$bindings.command && command !== void 0)
    $$bindings.command(command);
  if ($$props.face === void 0 && $$bindings.face && face !== void 0)
    $$bindings.face(face);
  if ($$props.param === void 0 && $$bindings.param && param !== void 0)
    $$bindings.param(param);
  if ($$props.minValue === void 0 && $$bindings.minValue && minValue !== void 0)
    $$bindings.minValue(minValue);
  if ($$props.maxValue === void 0 && $$bindings.maxValue && maxValue !== void 0)
    $$bindings.maxValue(maxValue);
  if ($$props.classSlider === void 0 && $$bindings.classSlider && classSlider !== void 0)
    $$bindings.classSlider(classSlider);
  return `<div>${escape(param)}
  <input type="range"${add_attribute("command", command, 0)}${add_attribute("class", classSlider, 0)}${add_attribute("face", face, 0)}${add_attribute("param", param, 0)}${add_attribute("min", minValue, 0)}${add_attribute("max", maxValue, 0)}${add_attribute("value", value, 0)}></div>`;
});
const keyPressed = writable({
  ArrowLeft: 0,
  ArrowUp: 0,
  ArrowRight: 0,
  ArrowDown: 0
});
const keyPressedCustom = derived(keyPressed, ($keyPressed) => {
  return {
    ...$keyPressed,
    ArrowV: $keyPressed.ArrowUp + $keyPressed.ArrowDown,
    ArrowH: $keyPressed.ArrowLeft + $keyPressed.ArrowRight,
    ArrowVH: $keyPressed.ArrowUp + $keyPressed.ArrowDown + $keyPressed.ArrowLeft + $keyPressed.ArrowRight
  };
});
const Picker_svelte_svelte_type_style_lang = "";
const css$c = {
  code: ".picker.svelte-uiwgvv{position:relative;width:100%;height:100%;background:linear-gradient(#ffffff00, #000000ff),\n			linear-gradient(0.25turn, #ffffffff, #00000000), var(--color-bg);outline:none;user-select:none}",
  map: null
};
const Picker = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let $$unsubscribe_keyPressed;
  let $$unsubscribe_keyPressedCustom;
  $$unsubscribe_keyPressed = subscribe(keyPressed, (value) => value);
  $$unsubscribe_keyPressedCustom = subscribe(keyPressedCustom, (value) => value);
  let { components } = $$props;
  let { h } = $$props;
  let { s } = $$props;
  let { v } = $$props;
  let { isOpen } = $$props;
  let { toRight } = $$props;
  let { isDark } = $$props;
  let picker;
  let focused = false;
  let colorBg;
  let pos = { x: 100, y: 0 };
  if ($$props.components === void 0 && $$bindings.components && components !== void 0)
    $$bindings.components(components);
  if ($$props.h === void 0 && $$bindings.h && h !== void 0)
    $$bindings.h(h);
  if ($$props.s === void 0 && $$bindings.s && s !== void 0)
    $$bindings.s(s);
  if ($$props.v === void 0 && $$bindings.v && v !== void 0)
    $$bindings.v(v);
  if ($$props.isOpen === void 0 && $$bindings.isOpen && isOpen !== void 0)
    $$bindings.isOpen(isOpen);
  if ($$props.toRight === void 0 && $$bindings.toRight && toRight !== void 0)
    $$bindings.toRight(toRight);
  if ($$props.isDark === void 0 && $$bindings.isDark && isDark !== void 0)
    $$bindings.isDark(isDark);
  $$result.css.add(css$c);
  {
    if (typeof h === "number")
      colorBg = colord({ h, s: 100, v: 100, a: 1 }).toHex();
  }
  {
    if (typeof s === "number" && typeof v === "number" && picker)
      pos = { x: s, y: 100 - v };
  }
  $$unsubscribe_keyPressed();
  $$unsubscribe_keyPressedCustom();
  return `

${validate_component(components.pickerWrapper || missing_component, "svelte:component").$$render($$result, { focused, toRight }, {}, {
    default: () => {
      return `
	<div class="picker svelte-uiwgvv" tabindex="0" style="${"--color-bg: " + escape(colorBg, true) + ";"}" aria-label="saturation and brightness picker (arrow keyboard navigation)"${add_attribute("aria-valuemin", 0, 0)}${add_attribute("aria-valuemax", 100, 0)} aria-valuetext="${"saturation " + escape(pos.x?.toFixed(), true) + "%, brightness " + escape(pos.y?.toFixed(), true) + "%"}"${add_attribute("this", picker, 0)}>${validate_component(components.pickerIndicator || missing_component, "svelte:component").$$render(
        $$result,
        {
          pos,
          isDark,
          hex: colord({ h, s, v, a: 1 }).toHex()
        },
        {},
        {}
      )}</div>`;
    }
  })}`;
});
const Slider_svelte_svelte_type_style_lang = "";
const css$b = {
  code: ".slider.svelte-14nweqy{--gradient:#ff0000, #ffff00 17.2%, #ffff00 18.2%, #00ff00 33.3%, #00ffff 49.5%, #00ffff 51.5%,\n			#0000ff 67.7%, #ff00ff 83.3%, #ff0000;position:relative;width:100%;height:100%;background:linear-gradient(var(--gradient));outline:none;user-select:none}.to-right.svelte-14nweqy{background:linear-gradient(0.25turn, var(--gradient))}",
  map: null
};
const Slider = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let $$unsubscribe_keyPressed;
  let $$unsubscribe_keyPressedCustom;
  $$unsubscribe_keyPressed = subscribe(keyPressed, (value) => value);
  $$unsubscribe_keyPressedCustom = subscribe(keyPressedCustom, (value) => value);
  let { components } = $$props;
  let { toRight } = $$props;
  let slider;
  let { h } = $$props;
  let pos = 0;
  let focused = false;
  if ($$props.components === void 0 && $$bindings.components && components !== void 0)
    $$bindings.components(components);
  if ($$props.toRight === void 0 && $$bindings.toRight && toRight !== void 0)
    $$bindings.toRight(toRight);
  if ($$props.h === void 0 && $$bindings.h && h !== void 0)
    $$bindings.h(h);
  $$result.css.add(css$b);
  {
    if (typeof h === "number" && slider)
      pos = 100 * h / 360;
  }
  $$unsubscribe_keyPressed();
  $$unsubscribe_keyPressedCustom();
  return `

${validate_component(components.sliderWrapper || missing_component, "svelte:component").$$render($$result, { focused, toRight }, {}, {
    default: () => {
      return `
	<div class="${["slider svelte-14nweqy", toRight ? "to-right" : ""].join(" ").trim()}" tabindex="0" aria-label="hue picker (arrow keyboard navigation)"${add_attribute("aria-valuemin", 0, 0)}${add_attribute("aria-valuemax", 360, 0)}${add_attribute("aria-valuenow", Math.round(h), 0)}${add_attribute("this", slider, 0)}>${validate_component(components.sliderIndicator || missing_component, "svelte:component").$$render($$result, { pos, toRight }, {}, {})}</div>`;
    }
  })}`;
});
const Alpha_svelte_svelte_type_style_lang = "";
const css$a = {
  code: ".alpha.svelte-f2vq53:after{position:absolute;content:'';inset:0;background:linear-gradient(#00000000, var(--alpha-color));z-index:0}.to-right.svelte-f2vq53:after{background:linear-gradient(0.25turn, #00000000, var(--alpha-color))}.alpha.svelte-f2vq53{position:relative;width:100%;height:100%;background-image:linear-gradient(45deg, #eee 25%, transparent 25%, transparent 75%, #eee 75%),\n			linear-gradient(45deg, #eee 25%, transparent 25%, transparent 75%, #eee 75%);background-size:var(--pattern-size-2x, 12px) var(--pattern-size-2x, 12px);background-position:0 0, var(--pattern-size, 6px) var(--pattern-size, 6px);outline:none;user-select:none}",
  map: null
};
const Alpha = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let $$unsubscribe_keyPressed;
  let $$unsubscribe_keyPressedCustom;
  $$unsubscribe_keyPressed = subscribe(keyPressed, (value) => value);
  $$unsubscribe_keyPressedCustom = subscribe(keyPressedCustom, (value) => value);
  let { components } = $$props;
  let { isOpen } = $$props;
  let { a = 1 } = $$props;
  let { hex } = $$props;
  let { toRight } = $$props;
  let alpha;
  let focused = false;
  let pos;
  if ($$props.components === void 0 && $$bindings.components && components !== void 0)
    $$bindings.components(components);
  if ($$props.isOpen === void 0 && $$bindings.isOpen && isOpen !== void 0)
    $$bindings.isOpen(isOpen);
  if ($$props.a === void 0 && $$bindings.a && a !== void 0)
    $$bindings.a(a);
  if ($$props.hex === void 0 && $$bindings.hex && hex !== void 0)
    $$bindings.hex(hex);
  if ($$props.toRight === void 0 && $$bindings.toRight && toRight !== void 0)
    $$bindings.toRight(toRight);
  $$result.css.add(css$a);
  {
    if (typeof a === "number" && alpha)
      pos = 100 * a;
  }
  $$unsubscribe_keyPressed();
  $$unsubscribe_keyPressedCustom();
  return `

${validate_component(components.alphaWrapper || missing_component, "svelte:component").$$render($$result, { focused, toRight }, {}, {
    default: () => {
      return `
	<div class="${["alpha svelte-f2vq53", toRight ? "to-right" : ""].join(" ").trim()}" tabindex="0" style="${"--alpha-color: " + escape(hex?.substring(0, 7), true)}" aria-label="transparency picker (arrow keyboard navigation)"${add_attribute("aria-valuemin", 0, 0)}${add_attribute("aria-valuemax", 100, 0)}${add_attribute("aria-valuenow", Math.round(pos), 0)} aria-valuetext="${escape(pos?.toFixed(), true) + "%"}"${add_attribute("this", alpha, 0)}>${validate_component(components.alphaIndicator || missing_component, "svelte:component").$$render($$result, { pos, toRight }, {}, {})}</div>`;
    }
  })}`;
});
const TextInput_svelte_svelte_type_style_lang = "";
const css$9 = {
  code: ".text-input.svelte-11z4jba{display:flex;flex-direction:column;gap:10px;margin:10px 5px 5px}.input-container.svelte-11z4jba{display:flex;flex:1;gap:10px}input.svelte-11z4jba,button.svelte-11z4jba{flex:1;border:none;background-color:#eee;padding:0;border-radius:5px;height:30px;line-height:30px;text-align:center}input.svelte-11z4jba{width:5px}button.svelte-11z4jba{cursor:pointer;flex:1;margin:0;transition:background-color 0.2s}button.svelte-11z4jba:hover{background-color:#ccc}input.svelte-11z4jba:focus,button.svelte-11z4jba:focus{outline:none}input.svelte-11z4jba:focus-visible,button.svelte-11z4jba:focus-visible{outline:2px solid var(--focus-color, red);outline-offset:2px}",
  map: null
};
const TextInput = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let a;
  let { isAlpha } = $$props;
  let { rgb } = $$props;
  let { hsv } = $$props;
  let { hex } = $$props;
  let { canChangeMode } = $$props;
  const modes = ["HEX", "RGB", "HSV"];
  let mode = 0;
  if ($$props.isAlpha === void 0 && $$bindings.isAlpha && isAlpha !== void 0)
    $$bindings.isAlpha(isAlpha);
  if ($$props.rgb === void 0 && $$bindings.rgb && rgb !== void 0)
    $$bindings.rgb(rgb);
  if ($$props.hsv === void 0 && $$bindings.hsv && hsv !== void 0)
    $$bindings.hsv(hsv);
  if ($$props.hex === void 0 && $$bindings.hex && hex !== void 0)
    $$bindings.hex(hex);
  if ($$props.canChangeMode === void 0 && $$bindings.canChangeMode && canChangeMode !== void 0)
    $$bindings.canChangeMode(canChangeMode);
  $$result.css.add(css$9);
  Math.round(hsv.h);
  Math.round(hsv.s);
  Math.round(hsv.v);
  a = hsv.a === void 0 ? 1 : Math.round(hsv.a * 100) / 100;
  return `<div class="text-input svelte-11z4jba">${`<div class="input-container svelte-11z4jba"><input${add_attribute("value", hex, 0)} style="flex: 3" class="svelte-11z4jba">
			${isAlpha ? `<input aria-label="hexadecimal color"${add_attribute("value", a, 0)} type="number" min="0" max="1" step="0.01" class="svelte-11z4jba">` : ``}</div>`}

	${canChangeMode ? `<button aria-label="${"change inputs to " + escape(modes[(mode + 1) % 3], true)}" class="svelte-11z4jba">${escape(modes[mode])}</button>` : ``}
</div>`;
});
const SliderIndicator_svelte_svelte_type_style_lang$1 = "";
const css$8 = {
  code: "div.svelte-htgglv{position:absolute;width:9.5px;height:9.5px;background-color:white;border-radius:5px;margin-left:1px;pointer-events:none;z-index:1;border:1px solid black;box-sizing:border-box}",
  map: null
};
const SliderIndicator = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let { pos } = $$props;
  let { toRight } = $$props;
  if ($$props.pos === void 0 && $$bindings.pos && pos !== void 0)
    $$bindings.pos(pos);
  if ($$props.toRight === void 0 && $$bindings.toRight && toRight !== void 0)
    $$bindings.toRight(toRight);
  $$result.css.add(css$8);
  return `<div class="slider-indicator svelte-htgglv"${add_attribute("style", `top: calc(${pos / 200 * 186}% + 2px);`, 0)}></div>`;
});
const PickerIndicator_svelte_svelte_type_style_lang$1 = "";
const css$7 = {
  code: "div.svelte-1nw247x{position:absolute;width:10px;height:10px;background-color:white;border-radius:5px;pointer-events:none;z-index:1;transition:box-shadow 0.2s}",
  map: null
};
const PickerIndicator = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let { pos } = $$props;
  let { hex } = $$props;
  let { isDark } = $$props;
  if ($$props.pos === void 0 && $$bindings.pos && pos !== void 0)
    $$bindings.pos(pos);
  if ($$props.hex === void 0 && $$bindings.hex && hex !== void 0)
    $$bindings.hex(hex);
  if ($$props.isDark === void 0 && $$bindings.isDark && isDark !== void 0)
    $$bindings.isDark(isDark);
  $$result.css.add(css$7);
  return `<div class="picker-indicator svelte-1nw247x"${add_attribute("style", `left: calc(${pos.x / 200 * 186}% + 2px); top: calc(${pos.y / 200 * 186}% + 2px); box-shadow: 0 0 4px ${isDark ? "white" : "black"};`, 0)}></div>`;
});
const ArrowKeyHandler = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let $$unsubscribe_keyPressed;
  $$unsubscribe_keyPressed = subscribe(keyPressed, (value) => value);
  $$unsubscribe_keyPressed();
  return ``;
});
const PickerWrapper_svelte_svelte_type_style_lang$1 = "";
const css$6 = {
  code: "div.svelte-1isc651{display:inline-block;margin-right:5px;height:200px;width:200px;border-radius:8px;overflow:hidden;outline:2px solid transparent;outline-offset:3px;transition:outline 0.2s ease-in-out;user-select:none}div.focused.svelte-1isc651{outline:2px solid var(--focus-color, red)}",
  map: null
};
const PickerWrapper = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let { focused } = $$props;
  let { toRight } = $$props;
  if ($$props.focused === void 0 && $$bindings.focused && focused !== void 0)
    $$bindings.focused(focused);
  if ($$props.toRight === void 0 && $$bindings.toRight && toRight !== void 0)
    $$bindings.toRight(toRight);
  $$result.css.add(css$6);
  return `<div class="${["picker-wrapper svelte-1isc651", focused ? "focused" : ""].join(" ").trim()}">${slots.default ? slots.default({}) : ``}
</div>`;
});
const SliderWrapper_svelte_svelte_type_style_lang$1 = "";
const css$5 = {
  code: "div.svelte-1udewng{display:inline-block;margin-right:5px;width:var(--slider-width, 12px);height:var(--picker-height, 200px);border-radius:6px;overflow:hidden;user-select:none;outline:2px solid transparent;outline-offset:3px;transition:outline 0.2s ease-in-out}div.focused.svelte-1udewng{outline:2px solid var(--focus-color, red)}",
  map: null
};
const SliderWrapper = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let { focused } = $$props;
  let { toRight } = $$props;
  if ($$props.focused === void 0 && $$bindings.focused && focused !== void 0)
    $$bindings.focused(focused);
  if ($$props.toRight === void 0 && $$bindings.toRight && toRight !== void 0)
    $$bindings.toRight(toRight);
  $$result.css.add(css$5);
  return `<div class="${["slider-wrapper svelte-1udewng", focused ? "focused" : ""].join(" ").trim()}">${slots.default ? slots.default({}) : ``}
</div>`;
});
const Input_svelte_svelte_type_style_lang = "";
const css$4 = {
  code: "label.svelte-2ybi8r{display:flex;align-items:center;gap:8px;cursor:pointer;border-radius:3px;margin:4px}.container.svelte-2ybi8r{position:relative;display:block;display:flex;align-items:center;justify-content:center}input.svelte-2ybi8r{margin:0;padding:0;border:none;width:4px;height:4px;flex-shrink:0;cursor:pointer;border-radius:50%;margin:0 12px}.alpha.svelte-2ybi8r{clip-path:circle(50%);background-image:linear-gradient(\n				to right,\n				rgba(238, 238, 238, 0.75),\n				rgba(238, 238, 238, 0.75)\n			),\n			linear-gradient(to right, black 50%, white 50%),\n			linear-gradient(to bottom, black 50%, white 50%);background-blend-mode:normal, difference, normal;background-size:12px 12px}.alpha.svelte-2ybi8r,.color.svelte-2ybi8r{position:absolute;width:30px;height:30px;border-radius:15px;user-select:none}input.svelte-2ybi8r:focus{outline:2px solid var(--focus-color, red);outline-offset:15px}",
  map: null
};
const Input = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let { labelWrapper } = $$props;
  let { hex } = $$props;
  let { label } = $$props;
  let { isOpen } = $$props;
  if ($$props.labelWrapper === void 0 && $$bindings.labelWrapper && labelWrapper !== void 0)
    $$bindings.labelWrapper(labelWrapper);
  if ($$props.hex === void 0 && $$bindings.hex && hex !== void 0)
    $$bindings.hex(hex);
  if ($$props.label === void 0 && $$bindings.label && label !== void 0)
    $$bindings.label(label);
  if ($$props.isOpen === void 0 && $$bindings.isOpen && isOpen !== void 0)
    $$bindings.isOpen(isOpen);
  $$result.css.add(css$4);
  return `
<label class="svelte-2ybi8r"${add_attribute("this", labelWrapper, 0)}><div class="container svelte-2ybi8r"><input type="color"${add_attribute("value", hex, 0)} aria-haspopup="dialog" class="svelte-2ybi8r">
		<div class="alpha svelte-2ybi8r"></div>
		<div class="color svelte-2ybi8r" style="${"background: " + escape(hex, true)}"></div></div>
	${escape(label)}
</label>`;
});
const Wrapper_svelte_svelte_type_style_lang$1 = "";
const css$3 = {
  code: "div.svelte-160vdtq{padding:8px 5px 5px 8px;background-color:white;margin:0 10px 10px;border:1px solid black;border-radius:12px;display:none;width:max-content}.isOpen.svelte-160vdtq{display:block}.isPopup.svelte-160vdtq{position:absolute;top:30px;z-index:2}",
  map: null
};
const Wrapper = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let { wrapper } = $$props;
  let { isOpen } = $$props;
  let { isPopup } = $$props;
  let { toRight } = $$props;
  if ($$props.wrapper === void 0 && $$bindings.wrapper && wrapper !== void 0)
    $$bindings.wrapper(wrapper);
  if ($$props.isOpen === void 0 && $$bindings.isOpen && isOpen !== void 0)
    $$bindings.isOpen(isOpen);
  if ($$props.isPopup === void 0 && $$bindings.isPopup && isPopup !== void 0)
    $$bindings.isPopup(isPopup);
  if ($$props.toRight === void 0 && $$bindings.toRight && toRight !== void 0)
    $$bindings.toRight(toRight);
  $$result.css.add(css$3);
  return `<div class="${[
    "wrapper svelte-160vdtq",
    (isOpen ? "isOpen" : "") + " " + (isPopup ? "isPopup" : "")
  ].join(" ").trim()}"${add_attribute("role", isPopup ? "dialog" : void 0, 0)} aria-label="color picker"${add_attribute("this", wrapper, 0)}>${slots.default ? slots.default({}) : ``}
</div>`;
});
const A11yNotice_svelte_svelte_type_style_lang = "";
const css$2 = {
  code: "details.svelte-1s4dluu.svelte-1s4dluu{width:245px;margin:8px auto}summary.svelte-1s4dluu.svelte-1s4dluu{margin-bottom:8px;cursor:pointer;transition:color 0.2s}summary.svelte-1s4dluu.svelte-1s4dluu:hover{color:blue}.not-closable.svelte-1s4dluu summary.svelte-1s4dluu{pointer-events:none}.not-closable.svelte-1s4dluu summary.svelte-1s4dluu{list-style:none}.svelte-1s4dluu.svelte-1s4dluu:focus-visible,span.svelte-1s4dluu :focus-visible{border-radius:2px;outline:2px solid var(--focus-color, red);outline-offset:2px}",
  map: null
};
const A11yNotice = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let closable;
  let _a11yColors;
  let { components } = $$props;
  let { a11yColors } = $$props;
  let { hex } = $$props;
  let { color } = $$props;
  let { a11yGuidelines } = $$props;
  let { isA11yOpen } = $$props;
  let { isA11yClosable } = $$props;
  if ($$props.components === void 0 && $$bindings.components && components !== void 0)
    $$bindings.components(components);
  if ($$props.a11yColors === void 0 && $$bindings.a11yColors && a11yColors !== void 0)
    $$bindings.a11yColors(a11yColors);
  if ($$props.hex === void 0 && $$bindings.hex && hex !== void 0)
    $$bindings.hex(hex);
  if ($$props.color === void 0 && $$bindings.color && color !== void 0)
    $$bindings.color(color);
  if ($$props.a11yGuidelines === void 0 && $$bindings.a11yGuidelines && a11yGuidelines !== void 0)
    $$bindings.a11yGuidelines(a11yGuidelines);
  if ($$props.isA11yOpen === void 0 && $$bindings.isA11yOpen && isA11yOpen !== void 0)
    $$bindings.isA11yOpen(isA11yOpen);
  if ($$props.isA11yClosable === void 0 && $$bindings.isA11yClosable && isA11yClosable !== void 0)
    $$bindings.isA11yClosable(isA11yClosable);
  $$result.css.add(css$2);
  closable = isA11yOpen && !isA11yClosable;
  _a11yColors = a11yColors.map((a11yColor) => ({
    ...a11yColor,
    contrast: color?.contrast(a11yColor.hex)
  }));
  return `<details class="${"a11y-notice " + escape(closable ? "not-closable" : "", true) + " svelte-1s4dluu"}" ${isA11yOpen ? "open" : ""}><summary${add_attribute("tabindex", closable ? -1 : void 0, 0)} class="svelte-1s4dluu">${validate_component(components.a11ySummary || missing_component, "svelte:component").$$render($$result, { a11yColors: _a11yColors, hex }, {}, {})}</summary>
	<div class="svelte-1s4dluu">${each(_a11yColors, ({ contrast, hex: a11yHex, placeholder, reverse, size }) => {
    return `${validate_component(components.a11ySingleNotice || missing_component, "svelte:component").$$render(
      $$result,
      {
        contrast,
        textColor: reverse ? a11yHex : hex,
        bgColor: reverse ? hex : a11yHex,
        placeholder,
        size
      },
      {},
      {}
    )}`;
  })}
		${a11yGuidelines ? `<span class="svelte-1s4dluu"><!-- HTML_TAG_START -->${a11yGuidelines}<!-- HTML_TAG_END --></span>` : ``}</div>
</details>`;
});
const A11ySingleNotice_svelte_svelte_type_style_lang = "";
const css$1 = {
  code: ".a11y-single-notice.svelte-obnxge.svelte-obnxge{display:flex;align-items:center;margin:4px 0;gap:12px;height:48px}.large.svelte-obnxge.svelte-obnxge{font-size:22px}.score.svelte-obnxge.svelte-obnxge{width:95px;flex-shrink:0;margin-bottom:10px}.score.svelte-obnxge p.svelte-obnxge{margin-bottom:2px}.grade.svelte-obnxge.svelte-obnxge{border-radius:50px;padding:2px 8px;background-color:orange;font-weight:bold}.grade-ok.svelte-obnxge.svelte-obnxge{background-color:green;color:white}p.svelte-obnxge.svelte-obnxge{margin:0}.lorem.svelte-obnxge.svelte-obnxge{flex:1;text-align:center;padding:4px 8px;border-radius:4px;border:1px solid black;white-space:nowrap;text-overflow:ellipsis;overflow:hidden}",
  map: null
};
const A11ySingleNotice = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let { textColor } = $$props;
  let { bgColor } = $$props;
  let { placeholder = void 0 } = $$props;
  let { contrast = 1 } = $$props;
  let { size = void 0 } = $$props;
  if ($$props.textColor === void 0 && $$bindings.textColor && textColor !== void 0)
    $$bindings.textColor(textColor);
  if ($$props.bgColor === void 0 && $$bindings.bgColor && bgColor !== void 0)
    $$bindings.bgColor(bgColor);
  if ($$props.placeholder === void 0 && $$bindings.placeholder && placeholder !== void 0)
    $$bindings.placeholder(placeholder);
  if ($$props.contrast === void 0 && $$bindings.contrast && contrast !== void 0)
    $$bindings.contrast(contrast);
  if ($$props.size === void 0 && $$bindings.size && size !== void 0)
    $$bindings.size(size);
  $$result.css.add(css$1);
  return `<div class="a11y-single-notice svelte-obnxge"><p class="${"lorem " + escape(size === "large" && "large", true) + " svelte-obnxge"}" style="${"color: " + escape(textColor, true) + "; background-color: " + escape(bgColor, true)}">${escape(placeholder || "Lorem Ipsum")}</p>
	<div class="score svelte-obnxge"><p class="svelte-obnxge">contrast: ${escape(contrast >= 10 ? contrast.toFixed(1) : contrast)}</p>
		<span class="${[
    "grade svelte-obnxge",
    contrast > (size === "large" ? 3 : 4.5) ? "grade-ok" : ""
  ].join(" ").trim()}">AA</span>
		<span class="${[
    "grade svelte-obnxge",
    contrast > (size === "large" ? 4.5 : 7) ? "grade-ok" : ""
  ].join(" ").trim()}">AAA</span></div>
</div>`;
});
function getNumberOfGradeFailed({ contrast, size }) {
  if (!contrast) {
    return 1;
  }
  if (size === "large") {
    return contrast < 3 ? 2 : contrast < 4.5 ? 1 : 0;
  } else {
    return contrast < 4.5 ? 2 : contrast < 7 ? 1 : 0;
  }
}
const A11ySummary = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let count;
  let message;
  let { a11yColors } = $$props;
  let { hex } = $$props;
  if ($$props.a11yColors === void 0 && $$bindings.a11yColors && a11yColors !== void 0)
    $$bindings.a11yColors(a11yColors);
  if ($$props.hex === void 0 && $$bindings.hex && hex !== void 0)
    $$bindings.hex(hex);
  count = a11yColors.map(getNumberOfGradeFailed).reduce((acc, c) => acc + c);
  message = count ? `⚠️ ${count} contrast grade${count && "s"} fail` : "Contrast grade information";
  return `${escape(message)}`;
});
const ColorPicker_svelte_svelte_type_style_lang = "";
const css = {
  code: "span.svelte-7ntk55{position:relative}",
  map: null
};
const ColorPicker = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  extend([a11yPlugin]);
  let { components = {} } = $$props;
  const dispatch = createEventDispatcher();
  let { label = "Choose a color" } = $$props;
  let { isAlpha = true } = $$props;
  let { isInput = true } = $$props;
  let { isTextInput = true } = $$props;
  let { canChangeMode = true } = $$props;
  let { isA11y = false } = $$props;
  let { a11yColors = [{ hex: "#ffffff" }] } = $$props;
  let { a11yGuidelines = '<p style="margin: 0; font-size: 12px;">Learn more at <a href="https://webaim.org/articles/contrast/" target="_blank">WebAIM contrast guide</a></p>' } = $$props;
  let { isA11yOpen = false } = $$props;
  let { isA11yClosable = true } = $$props;
  let { isPopup = isInput } = $$props;
  let { isOpen = !isInput } = $$props;
  let { toRight = false } = $$props;
  let { rgb = { r: 255, g: 0, b: 0, a: 1 } } = $$props;
  let { hsv = { h: 0, s: 100, v: 100, a: 1 } } = $$props;
  let { hex = "#ff0000" } = $$props;
  let { color = void 0 } = $$props;
  let { isDark = false } = $$props;
  let _rgb = { r: 255, g: 0, b: 0, a: 1 };
  let _hsv = { h: 0, s: 100, v: 100, a: 1 };
  let _hex = "#ff0000";
  let span;
  const default_components = {
    sliderIndicator: SliderIndicator,
    pickerIndicator: PickerIndicator,
    alphaIndicator: SliderIndicator,
    pickerWrapper: PickerWrapper,
    sliderWrapper: SliderWrapper,
    alphaWrapper: SliderWrapper,
    textInput: TextInput,
    a11yNotice: A11yNotice,
    a11ySingleNotice: A11ySingleNotice,
    a11ySummary: A11ySummary,
    input: Input,
    wrapper: Wrapper
  };
  function getComponents() {
    return { ...default_components, ...components };
  }
  let labelWrapper;
  let wrapper;
  function updateColor() {
    if (hsv.h === _hsv.h && hsv.s === _hsv.s && hsv.v === _hsv.v && hsv.a === _hsv.a && rgb.r === _rgb.r && rgb.g === _rgb.g && rgb.b === _rgb.b && rgb.a === _rgb.a && hex === _hex) {
      return;
    }
    if (hsv.a === void 0)
      hsv.a = 1;
    if (_hsv.a === void 0)
      _hsv.a = 1;
    if (rgb.a === void 0)
      rgb.a = 1;
    if (_rgb.a === void 0)
      _rgb.a = 1;
    if (hex?.substring(7) === "ff")
      hex = hex.substring(0, 7);
    if (hex?.substring(7) === "ff")
      hex = hex.substring(0, 7);
    if (hsv.h !== _hsv.h || hsv.s !== _hsv.s || hsv.v !== _hsv.v || hsv.a !== _hsv.a) {
      color = colord(hsv);
      rgb = color.toRgb();
      hex = color.toHex();
    } else if (rgb.r !== _rgb.r || rgb.g !== _rgb.g || rgb.b !== _rgb.b || rgb.a !== _rgb.a) {
      color = colord(rgb);
      hex = color.toHex();
      hsv = color.toHsv();
    } else if (hex !== _hex) {
      color = colord(hex);
      rgb = color.toRgb();
      hsv = color.toHsv();
    }
    if (color) {
      isDark = color.isDark();
    }
    _hsv = Object.assign({}, hsv);
    _rgb = Object.assign({}, rgb);
    _hex = hex;
    dispatch("input", { color, hsv, rgb, hex });
  }
  if ($$props.components === void 0 && $$bindings.components && components !== void 0)
    $$bindings.components(components);
  if ($$props.label === void 0 && $$bindings.label && label !== void 0)
    $$bindings.label(label);
  if ($$props.isAlpha === void 0 && $$bindings.isAlpha && isAlpha !== void 0)
    $$bindings.isAlpha(isAlpha);
  if ($$props.isInput === void 0 && $$bindings.isInput && isInput !== void 0)
    $$bindings.isInput(isInput);
  if ($$props.isTextInput === void 0 && $$bindings.isTextInput && isTextInput !== void 0)
    $$bindings.isTextInput(isTextInput);
  if ($$props.canChangeMode === void 0 && $$bindings.canChangeMode && canChangeMode !== void 0)
    $$bindings.canChangeMode(canChangeMode);
  if ($$props.isA11y === void 0 && $$bindings.isA11y && isA11y !== void 0)
    $$bindings.isA11y(isA11y);
  if ($$props.a11yColors === void 0 && $$bindings.a11yColors && a11yColors !== void 0)
    $$bindings.a11yColors(a11yColors);
  if ($$props.a11yGuidelines === void 0 && $$bindings.a11yGuidelines && a11yGuidelines !== void 0)
    $$bindings.a11yGuidelines(a11yGuidelines);
  if ($$props.isA11yOpen === void 0 && $$bindings.isA11yOpen && isA11yOpen !== void 0)
    $$bindings.isA11yOpen(isA11yOpen);
  if ($$props.isA11yClosable === void 0 && $$bindings.isA11yClosable && isA11yClosable !== void 0)
    $$bindings.isA11yClosable(isA11yClosable);
  if ($$props.isPopup === void 0 && $$bindings.isPopup && isPopup !== void 0)
    $$bindings.isPopup(isPopup);
  if ($$props.isOpen === void 0 && $$bindings.isOpen && isOpen !== void 0)
    $$bindings.isOpen(isOpen);
  if ($$props.toRight === void 0 && $$bindings.toRight && toRight !== void 0)
    $$bindings.toRight(toRight);
  if ($$props.rgb === void 0 && $$bindings.rgb && rgb !== void 0)
    $$bindings.rgb(rgb);
  if ($$props.hsv === void 0 && $$bindings.hsv && hsv !== void 0)
    $$bindings.hsv(hsv);
  if ($$props.hex === void 0 && $$bindings.hex && hex !== void 0)
    $$bindings.hex(hex);
  if ($$props.color === void 0 && $$bindings.color && color !== void 0)
    $$bindings.color(color);
  if ($$props.isDark === void 0 && $$bindings.isDark && isDark !== void 0)
    $$bindings.isDark(isDark);
  $$result.css.add(css);
  let $$settled;
  let $$rendered;
  do {
    $$settled = true;
    {
      if (hsv || rgb || hex) {
        updateColor();
      }
    }
    $$rendered = `${validate_component(ArrowKeyHandler, "ArrowKeyHandler").$$render($$result, {}, {}, {})}



<span class="color-picker svelte-7ntk55"${add_attribute("this", span, 0)}>${isInput ? `${validate_component(getComponents().input || missing_component, "svelte:component").$$render(
      $$result,
      { hex, label, labelWrapper, isOpen },
      {
        labelWrapper: ($$value) => {
          labelWrapper = $$value;
          $$settled = false;
        },
        isOpen: ($$value) => {
          isOpen = $$value;
          $$settled = false;
        }
      },
      {}
    )}` : `<input type="hidden"${add_attribute("value", hex, 0)}>`}

	${validate_component(getComponents().wrapper || missing_component, "svelte:component").$$render(
      $$result,
      { isOpen, isPopup, toRight, wrapper },
      {
        wrapper: ($$value) => {
          wrapper = $$value;
          $$settled = false;
        }
      },
      {
        default: () => {
          return `${validate_component(Picker, "Picker").$$render(
            $$result,
            {
              components: getComponents(),
              h: hsv.h,
              toRight,
              isDark,
              s: hsv.s,
              v: hsv.v,
              isOpen
            },
            {
              s: ($$value) => {
                hsv.s = $$value;
                $$settled = false;
              },
              v: ($$value) => {
                hsv.v = $$value;
                $$settled = false;
              },
              isOpen: ($$value) => {
                isOpen = $$value;
                $$settled = false;
              }
            },
            {}
          )}
		${validate_component(Slider, "Slider").$$render(
            $$result,
            {
              components: getComponents(),
              toRight,
              h: hsv.h
            },
            {
              h: ($$value) => {
                hsv.h = $$value;
                $$settled = false;
              }
            },
            {}
          )}
		${isAlpha ? `${validate_component(Alpha, "Alpha").$$render(
            $$result,
            {
              components: getComponents(),
              hex,
              toRight,
              a: hsv.a,
              isOpen
            },
            {
              a: ($$value) => {
                hsv.a = $$value;
                $$settled = false;
              },
              isOpen: ($$value) => {
                isOpen = $$value;
                $$settled = false;
              }
            },
            {}
          )}` : ``}
		${isTextInput ? `${validate_component(getComponents().textInput || missing_component, "svelte:component").$$render(
            $$result,
            { isAlpha, canChangeMode, hex, rgb, hsv },
            {
              hex: ($$value) => {
                hex = $$value;
                $$settled = false;
              },
              rgb: ($$value) => {
                rgb = $$value;
                $$settled = false;
              },
              hsv: ($$value) => {
                hsv = $$value;
                $$settled = false;
              }
            },
            {}
          )}` : ``}
		${isA11y ? `${validate_component(getComponents().a11yNotice || missing_component, "svelte:component").$$render(
            $$result,
            {
              components: getComponents(),
              a11yColors,
              color,
              hex,
              a11yGuidelines,
              isA11yOpen,
              isA11yClosable
            },
            {},
            {}
          )}` : ``}`;
        }
      }
    )}
</span>`;
  } while (!$$settled);
  return $$rendered;
});
const PickerIndicator_svelte_svelte_type_style_lang = "";
const PickerWrapper_svelte_svelte_type_style_lang = "";
const SliderIndicator_svelte_svelte_type_style_lang = "";
const SliderWrapper_svelte_svelte_type_style_lang = "";
const Wrapper_svelte_svelte_type_style_lang = "";
const A11yHorizontalWrapper_svelte_svelte_type_style_lang = "";
const Color = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  createEventDispatcher();
  let { command = "interaction" } = $$props;
  let { face = "null" } = $$props;
  let { param = "main_color" } = $$props;
  let { label = "Choose colour" } = $$props;
  let rgb;
  if ($$props.command === void 0 && $$bindings.command && command !== void 0)
    $$bindings.command(command);
  if ($$props.face === void 0 && $$bindings.face && face !== void 0)
    $$bindings.face(face);
  if ($$props.param === void 0 && $$bindings.param && param !== void 0)
    $$bindings.param(param);
  if ($$props.label === void 0 && $$bindings.label && label !== void 0)
    $$bindings.label(label);
  let $$settled;
  let $$rendered;
  do {
    $$settled = true;
    $$rendered = `${validate_component(ColorPicker, "ColorPicker").$$render(
      $$result,
      { command, face, param, label, rgb },
      {
        rgb: ($$value) => {
          rgb = $$value;
          $$settled = false;
        }
      },
      {}
    )}`;
  } while (!$$settled);
  return $$rendered;
});
const Facecard = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  createEventDispatcher();
  let { img } = $$props;
  let { alt = "null" } = $$props;
  let { title = "null" } = $$props;
  let { id = "null" } = $$props;
  let { description = "null" } = $$props;
  let { modal_id = "null" } = $$props;
  if ($$props.img === void 0 && $$bindings.img && img !== void 0)
    $$bindings.img(img);
  if ($$props.alt === void 0 && $$bindings.alt && alt !== void 0)
    $$bindings.alt(alt);
  if ($$props.title === void 0 && $$bindings.title && title !== void 0)
    $$bindings.title(title);
  if ($$props.id === void 0 && $$bindings.id && id !== void 0)
    $$bindings.id(id);
  if ($$props.description === void 0 && $$bindings.description && description !== void 0)
    $$bindings.description(description);
  if ($$props.modal_id === void 0 && $$bindings.modal_id && modal_id !== void 0)
    $$bindings.modal_id(modal_id);
  return `
<div class="card card-compact btn-ghost lg:card-side bg-base-300 shadow-xl"><figure><img${add_attribute("src", img, 0)} class="bg-secondary-content width:40px"${add_attribute("alt", alt, 0)}></figure>
  <div class="card-body"><h2 class="card-title">${escape(title)}</h2>
      <p>${escape(description)}</p>
      <div class="card-actions justify-end"><label${add_attribute("for", modal_id, 0)} class="btn btn-ghost bg-base-100" stroke="none"><svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none" viewBox="0 0 16 16" stroke="currentColor"><path stroke-width=".5" d="M8 4.754a3.246 3.246 0 1 0 0 6.492 3.246 3.246 0 0 0 0-6.492zM5.754 8a2.246 2.246 0 1 1 4.492 0 2.246 2.246 0 0 1-4.492 0z"></path><path stroke-width="0.5" d="M9.796 1.343c-.527-1.79-3.065-1.79-3.592 0l-.094.319a.873.873 0 0 1-1.255.52l-.292-.16c-1.64-.892-3.433.902-2.54 2.541l.159.292a.873.873 0 0 1-.52 1.255l-.319.094c-1.79.527-1.79 3.065 0 3.592l.319.094a.873.873 0 0 1 .52 1.255l-.16.292c-.892 1.64.901 3.434 2.541 2.54l.292-.159a.873.873 0 0 1 1.255.52l.094.319c.527 1.79 3.065 1.79 3.592 0l.094-.319a.873.873 0 0 1 1.255-.52l.292.16c1.64.893 3.434-.902 2.54-2.541l-.159-.292a.873.873 0 0 1 .52-1.255l.319-.094c1.79-.527 1.79-3.065 0-3.592l-.319-.094a.873.873 0 0 1-.52-1.255l.16-.292c.893-1.64-.902-3.433-2.541-2.54l-.292.159a.873.873 0 0 1-1.255-.52l-.094-.319zm-2.633.283c.246-.835 1.428-.835 1.674 0l.094.319a1.873 1.873 0 0 0 2.693 1.115l.291-.16c.764-.415 1.6.42 1.184 1.185l-.159.292a1.873 1.873 0 0 0 1.116 2.692l.318.094c.835.246.835 1.428 0 1.674l-.319.094a1.873 1.873 0 0 0-1.115 2.693l.16.291c.415.764-.42 1.6-1.185 1.184l-.291-.159a1.873 1.873 0 0 0-2.693 1.116l-.094.318c-.246.835-1.428.835-1.674 0l-.094-.319a1.873 1.873 0 0 0-2.692-1.115l-.292.16c-.764.415-1.6-.42-1.184-1.185l.159-.291A1.873 1.873 0 0 0 1.945 8.93l-.319-.094c-.835-.246-.835-1.428 0-1.674l.319-.094A1.873 1.873 0 0 0 3.06 4.377l-.16-.292c-.415-.764.42-1.6 1.185-1.184l.292.159a1.873 1.873 0 0 0 2.692-1.115l.094-.319z"></path></svg></label></div></div></div>`;
});
const Textarea = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  createEventDispatcher();
  let { command = "interaction" } = $$props;
  let { param = "text" } = $$props;
  let { placeholder = "Enter text" } = $$props;
  if ($$props.command === void 0 && $$bindings.command && command !== void 0)
    $$bindings.command(command);
  if ($$props.param === void 0 && $$bindings.param && param !== void 0)
    $$bindings.param(param);
  if ($$props.placeholder === void 0 && $$bindings.placeholder && placeholder !== void 0)
    $$bindings.placeholder(placeholder);
  return `<textarea class="textarea textarea-bordered textarea-lg"${add_attribute("placeholder", placeholder, 0)}${add_attribute("param", param, 0)}>${""}</textarea>`;
});
const Button = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  createEventDispatcher();
  let { label = "null" } = $$props;
  let { command = "interaction" } = $$props;
  let { param = "null" } = $$props;
  if ($$props.label === void 0 && $$bindings.label && label !== void 0)
    $$bindings.label(label);
  if ($$props.command === void 0 && $$bindings.command && command !== void 0)
    $$bindings.command(command);
  if ($$props.param === void 0 && $$bindings.param && param !== void 0)
    $$bindings.param(param);
  return `<button class="btn btn-circle btn-outline"><svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12"></path></svg></button>

${escape(label)}`;
});
const Top_nav = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  return `<div class="app">
	<div class="navbar bg-base-100"><div class="flex-none"><label for="my-drawer" class="btn btn-square btn-ghost"><svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" class="inline-block w-6 h-6 stroke-current"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 6h16M4 12h16M4 18h16"></path></svg></label></div>
		<div class="flex-1 normal-case text-xl">LiveGrid Web Interface</div>

		
		<div class="flex-none"><label class="swap swap-rotate">
				<input type="checkbox" data-toggle-theme="dark,light" data-act-class="ACTIVECLASS">
				
				<svg class="swap-on fill-current w-8 h-8" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M5.64,17l-.71.71a1,1,0,0,0,0,1.41,1,1,0,0,0,1.41,0l.71-.71A1,1,0,0,0,5.64,17ZM5,12a1,1,0,0,0-1-1H3a1,1,0,0,0,0,2H4A1,1,0,0,0,5,12Zm7-7a1,1,0,0,0,1-1V3a1,1,0,0,0-2,0V4A1,1,0,0,0,12,5ZM5.64,7.05a1,1,0,0,0,.7.29,1,1,0,0,0,.71-.29,1,1,0,0,0,0-1.41l-.71-.71A1,1,0,0,0,4.93,6.34Zm12,.29a1,1,0,0,0,.7-.29l.71-.71a1,1,0,1,0-1.41-1.41L17,5.64a1,1,0,0,0,0,1.41A1,1,0,0,0,17.66,7.34ZM21,11H20a1,1,0,0,0,0,2h1a1,1,0,0,0,0-2Zm-9,8a1,1,0,0,0-1,1v1a1,1,0,0,0,2,0V20A1,1,0,0,0,12,19ZM18.36,17A1,1,0,0,0,17,18.36l.71.71a1,1,0,0,0,1.41,0,1,1,0,0,0,0-1.41ZM12,6.5A5.5,5.5,0,1,0,17.5,12,5.51,5.51,0,0,0,12,6.5Zm0,9A3.5,3.5,0,1,1,15.5,12,3.5,3.5,0,0,1,12,15.5Z"></path></svg>
				
				<svg class="swap-off fill-current w-8 h-8" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M21.64,13a1,1,0,0,0-1.05-.14,8.05,8.05,0,0,1-3.37.73A8.15,8.15,0,0,1,9.08,5.49a8.59,8.59,0,0,1,.25-2A1,1,0,0,0,8,2.36,10.14,10.14,0,1,0,22,14.05,1,1,0,0,0,21.64,13Zm-9.5,6.69A8.14,8.14,0,0,1,7.08,5.22v.27A10.15,10.15,0,0,0,17.22,15.63a9.79,9.79,0,0,0,2.1-.22A8.11,8.11,0,0,1,12.14,19.73Z"></path></svg></label></div></div>
	


</div>`;
});
const Wifi_form = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  createEventDispatcher();
  let ssid;
  let password;
  return `<h3 class="font-bold text-lg">Connect to WiFi</h3>
<form w-full max-w-xs><input type="text" placeholder="SSID" class="input input-bordered w-full max-w-xs"${add_attribute("value", ssid, 0)}>
  <input type="text" placeholder="password" class="input input-bordered w-full max-w-xs"${add_attribute("value", password, 0)}>
    <button class="btn" type="submit">Submit</button></form>`;
});
const deviceIP = writable(browser);
deviceIP.subscribe((value) => browser);
const Page = create_ssr_component(($$result, $$props, $$bindings, slots) => {
  let $deviceIP, $$unsubscribe_deviceIP;
  $$unsubscribe_deviceIP = subscribe(deviceIP, (value) => $deviceIP = value);
  let tabs = [
    {
      title: "Faces",
      svg: "M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6"
    },
    {
      title: "Text",
      svg: "M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6"
    },
    {
      title: "Settings",
      svg: "M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6"
    }
  ];
  let min_max_limits = {
    brightness_min: -127,
    brightness_max: 127,
    contrast_min: -255,
    contrast_max: 255,
    scale_min: 0,
    scale_max: 100,
    speed_min: 0,
    speed_max: 100
  };
  let backgrounds = [
    {
      title: "Simplex Noise",
      id: "simplex",
      img: "simplexNoise.jpg",
      modal_id: "simplexmodal",
      brightness_slider: "simplexbrightness",
      contrast_slider: "simplexcontrast",
      scale_slider: "simplexscale",
      speed_slider: "simplexspeed",
      color_picker: "simplexcolor",
      rgb_val: "simplexrgb"
    },
    {
      title: "Cellular Noise",
      id: "cellular",
      img: "cellularNoise.jpg",
      modal_id: "cellularmodal",
      brightness_slider: "cellularbrightness",
      contrast_slider: "cellularcontrast",
      scale_slider: "cellularscale",
      speed_slider: "cellularspeed",
      color_picker: "cellularcolor",
      rgb_val: "cellularrgb"
    },
    {
      title: "SimplexRidged Noise",
      id: "simplexridged",
      img: "simplexRNoise.jpg",
      modal_id: "simplexridgedmodal",
      brightness_slider: "simplexridgedbrightness",
      contrast_slider: "simplexridgedcontrast",
      scale_slider: "simplexridgedscale",
      speed_slider: "simplexridgedspeed",
      color_picker: "simplexridgedcolor",
      rgb_val: "simplexridgedrgb"
    },
    {
      title: "Text",
      id: "text",
      img: "text.jpg",
      modal_id: "textmodal",
      color_picker: "textcolor",
      text_rgb_val: "texttextrgb",
      back_rgb_val: "textbackrgb"
    },
    {
      title: "Dashboard",
      id: "dashboard",
      img: "dashboard.jpg",
      modal_id: "dashboardmodal",
      color_picker: "dashboardcolor",
      rgb_val: "dashboardrgb"
    },
    {
      title: "Basic",
      id: "basic",
      img: "dashboard.jpg",
      modal_id: "basicmodal",
      color_picker: "basiccolor",
      rgb_val: "basicrgb"
    },
    {
      title: "Stream",
      id: "stream",
      img: "dashboard.jpg",
      modal_id: "streammodal",
      color_picker: "streamcolor",
      rgb_val: "streamrgb"
    }
  ];
  $$unsubscribe_deviceIP();
  return `${$$result.head += `<!-- HEAD_svelte-11bysz6_START -->${$$result.title = `<title>Faces</title>`, ""}<meta name="Faces" content="Backgrounds"><!-- HEAD_svelte-11bysz6_END -->`, ""}
<body>
	<div class="${["modal", "modal-open"].join(" ").trim()}"><div class="modal-box"><h3 class="font-bold text-lg">Connecting ...</h3>
			<p class="py-4">Enter IP address of the panel</p>
			<input type="text"${add_attribute("placeholder", $deviceIP, 0)} class="input w-full max-w-xs"${add_attribute("value", $deviceIP, 0)}>
			<button class="btn">Connect</button>
			</div></div>
	
	${validate_component(Top_nav, "TopNav").$$render($$result, {}, {}, {})}
	
	<div class="drawer"><input id="my-drawer" type="checkbox" class="drawer-toggle">
		<div class="drawer-content"><div class="lg:p-10 sm:p-1">
				${`<div class="lg:columns-3 sm:columns-1 md:columns-2 space-y-6">
						${each(backgrounds, (back) => {
    return `${validate_component(Facecard, "Facecard").$$render(
      $$result,
      {
        img: back.img,
        title: back.title,
        id: back.id,
        modal_id: back.modal_id
      },
      {},
      {}
    )}`;
  })}</div>
					
					${each(backgrounds, (back) => {
    return `<input type="checkbox"${add_attribute("id", back.modal_id, 0)} class="modal-toggle">
						<div${add_attribute("for", back.modal_id, 0)} class="modal"><div class="modal-box flex flex-col gap-y-5"><h3 class="font-bold text-lg">${escape(back.title)}+ settings</h3>

								${back.title === "Text" ? `<div class="flex flex-row gap-5">${validate_component(Color, "Color").$$render($$result, { face: back.id, param: "text_color" }, {}, {})}
										${validate_component(Color, "Color").$$render(
      $$result,
      {
        face: back.id,
        param: "background_color",
        label: "Choose background color"
      },
      {},
      {}
    )}</div>
									<div>${validate_component(Textarea, "Textarea").$$render($$result, {}, {}, {})}
									</div>` : `${back.title === "Dashboard" ? `<div class="flex flex-row gap-5">${validate_component(Color, "Color").$$render($$result, { face: back.id, param: "main_color" }, {}, {})}
									</div>` : `<div class="flex flex-row gap-5">${validate_component(Color, "Color").$$render($$result, { face: back.id, param: "main_color" }, {}, {})}</div>
									${validate_component(Slider$1, "Slider").$$render(
      $$result,
      {
        face: back.id,
        param: "brightness",
        minValue: min_max_limits.brightness_min,
        maxValue: min_max_limits.brightness_max
      },
      {},
      {}
    )}
									${validate_component(Slider$1, "Slider").$$render(
      $$result,
      {
        face: back.id,
        param: "contrast",
        minValue: min_max_limits.contrast_min,
        maxValue: min_max_limits.contrast_max
      },
      {},
      {}
    )}
									${validate_component(Slider$1, "Slider").$$render(
      $$result,
      {
        face: back.id,
        param: "scale",
        minValue: min_max_limits.scale_min,
        maxValue: min_max_limits.scale_max
      },
      {},
      {}
    )}
									${validate_component(Slider$1, "Slider").$$render(
      $$result,
      {
        face: back.id,
        param: "speed",
        minValue: min_max_limits.speed_min,
        maxValue: min_max_limits.speed_max
      },
      {},
      {}
    )}`}`}
								<div class="modal-action"><label${add_attribute("for", back.modal_id, 0)} class="btn">Done</label>
								</div></div>
						</div>`;
  })}`}

				
				${``}
				<div for="wifi-modal" class="${["modal cursor-pointer", ""].join(" ").trim()}"><div class="modal-box">${validate_component(Wifi_form, "WifiForm").$$render($$result, {}, {}, {})}
						<div class="modal-action"><button class="btn">Close</button></div></div></div></div></div>

		
		<div class="drawer-side"><label for="my-drawer" class="drawer-overlay"></label>
			<ul class="menu p-4 w-80 bg-base-100 text-base-content">
				${each(tabs, (tab) => {
    return `<button class="btn btn-ghost"><svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"${add_attribute("d", tab.svg, 0)}></path></svg>
						${escape(tab.title)}
					</button>`;
  })}
				<li><div>${validate_component(Toggle, "Toggle").$$render($$result, { command: "main", param: "power" }, {}, {})} Power
					</div></li>
				<li><div>${validate_component(Button, "Button").$$render(
    $$result,
    {
      command: "main",
      param: "rotate",
      label: "Rotate display"
    },
    {},
    {}
  )}</div></li>
				<li><div>${validate_component(Slider$1, "Slider").$$render(
    $$result,
    {
      command: "main",
      param: "globalBrightness"
    },
    {},
    {}
  )}</div></li></ul></div></div>
	
</body>`;
});
export {
  Page as default
};
