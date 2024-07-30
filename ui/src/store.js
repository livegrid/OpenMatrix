import { get, writable } from 'svelte/store';

import { fetchWithTimeout } from './helpers';

// Status
export const connected = writable(false);
export const theme = writable('dark');

// State
export const state = writable({
    environment: {
    },
    effects: {
        selected: 2,
    },
    images: {
        selected: 0,
    },
});

// Start state interval
setInterval(async () => {
    try {
        const response = await fetchWithTimeout('./openmatrix/state', {
            method: 'GET',
            timeout: 2000
        });

        if (response.status === 200 && response.headers.get('Content-Type') === 'application/json') {
            const json = await response.json();            
            state.set(json);
        } else {
            state.set({});
        }
    } catch (err) {
        console.error(err);
    }
}, 2000);

export const togglePower = async ({ detail }) => {
    try {

        const response = await fetchWithTimeout('./openmatrix/power', {
            method: 'POST',
            timeout: 2000,
            body: JSON.stringify({
                power: detail
            })
        });
        if (response.status === 200) {
            state.set({
                ...get(state),
                power: detail
            })
        }
    } catch (err) {
        console.error(err);
    }
}

export const updateBrightness = async (value) => {
    try {
        const response = await fetchWithTimeout('./openmatrix/brightness', {
            method: 'POST',
            timeout: 2000,
            body: JSON.stringify({
                brightness: value
            })
        });
        if (response.status === 200) {
            state.set({
                ...get(state),
                // @ts-ignore
                brightness: value
            })
        }
    } catch (err) {
        console.error(err);
    }
}