import * as universal from '../entries/pages/_page.js';

export const index = 2;
export const component = async () => (await import('../entries/pages/_page.svelte.js')).default;
export { universal };
export const universal_id = "src/routes/+page.js";
export const imports = ["_app/immutable/nodes/2.d3b0e4f0.js","_app/immutable/chunks/index.b8c32cc7.js","_app/immutable/chunks/index.497fad04.js"];
export const stylesheets = ["_app/immutable/assets/2.7e711497.css"];
export const fonts = [];
