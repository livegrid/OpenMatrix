import { writable } from 'svelte/store';

// Status
export const connected = writable(false);
export const theme = writable('dark');

// State
export const state = writable({
    power: false,
    brightness: 25,
    mode: 1,
    enviournment: {
        temperature: {
            value: 25,
            diff: {
                type: 0,
                value: 0,
                inverse: false,
            },
        },
        humidity: {
            value: 43,
            diff: {
                type: 1,
                value: 10,
                inverse: true,
            },
        },
        co2: {
            value: 110,
            diff: {
                type: 2,
                value: 20,
                inverse: true,
            },
        },
    },
    effects: {
        selected: 2,
    },
    images: {
        selected: 0,
    },
});
