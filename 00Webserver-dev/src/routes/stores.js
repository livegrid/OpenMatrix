import { writable } from 'svelte/store';
import { browser } from "$app/environment"

// const stored = localStorage.content;

// export const preferences = persisted('preferences', {
//   ip: '192.168.1.199',
// })

export const deviceIP = writable(browser && (localStorage.getItem("deviceIP") || '192.168.1.199'));

// Anytime the store changes, update the local storage value.
deviceIP.subscribe((value) => browser && localStorage.setItem("deviceIP", value));

//192.168.1.199