import { get, writable } from 'svelte/store';
import { fetchWithTimeout } from './helpers';

const API_URL = `.`;

// Status
export const connected = writable(false);
export const theme = writable('dark');

// State
export const state = writable({});

export const images = writable(null);

const fetchState = async () => {
    try {
        const response = await fetchWithTimeout(`${API_URL}/openmatrix/state`, {
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
}

export const fetchImages = async () => {
    try {
        const response = await fetchWithTimeout(`${API_URL}/openmatrix/image`, {
            method: 'GET',
            timeout: 2000
        });

        if (response.status === 200 && response.headers.get('Content-Type') === 'application/json') {
            const json = await response.json();
            if (Array.isArray(json)) {
                images.set(json);
            }
        } else {
            images.set([]);
        }
    } catch (err) {
        console.error(err);
    }
}

setTimeout(() => {
    fetchState();
    setInterval(fetchState, 2000);
}, 1000);

export const togglePower = async ({ detail }) => {
    try {

        const response = await fetchWithTimeout(`${API_URL}./openmatrix/power`, {
            method: 'POST',
            timeout: 2000,
            body: JSON.stringify({
                power: detail
            })
        });
        if (response.status === 200) {
            state.set({
                ...get(state),
                // @ts-ignore
                power: detail
            })
        }
    } catch (err) {
        console.error(err);
    }
}

export const updateBrightness = async (value) => {
    try {
        const response = await fetchWithTimeout(`${API_URL}/openmatrix/brightness`, {
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

export const selectMode = async (modeId) => {
    try {
        const response = await fetchWithTimeout(`${API_URL}/openmatrix/mode`, {
            method: 'POST',
            timeout: 2000,
            body: JSON.stringify({
                mode: modeId
            })
        });
        if (response.status === 200) {
            state.set({
                ...get(state),
                // @ts-ignore
                mode: modeId
            })
        }
    } catch (err) {
        console.error(err);
    }
}

export const selectEffect = async (effect) => {
    try {
        const response = await fetchWithTimeout(`${API_URL}/openmatrix/effect`, {
            method: 'POST',
            timeout: 2000,
            body: JSON.stringify({
                effect
            })
        });
        if (response.status === 200) {
            state.set({
                ...get(state),
                // @ts-ignore
                effects: {
                    // @ts-ignore
                    ...get(state)?.effects,
                    selected: effect
                }
            })
        }
    } catch (err) {
        console.error(err);
    }
}

export const selectImage = async ({ id }) => {
    try {
        const response = await fetchWithTimeout(`${API_URL}/openmatrix/image`, {
            method: 'POST',
            timeout: 2000,
            body: JSON.stringify({
                id
            })
        });
        if (response.status === 200) {
            state.set({
                ...get(state),
                // @ts-ignore
                image: {
                    selected: id
                }
            })
        }
    } catch (err) {
        console.error(err);
    }
}

export const invokePreview = async ({ id }) => {
    try {
        const response = await fetchWithTimeout(`${API_URL}/openmatrix/image`, {
            method: 'PATCH',
            timeout: 2000,
            body: JSON.stringify({
                id
            })
        });
        if (response.status === 200) {
            return true;
        }
    } catch (err) {
        console.error(err);
    }
}

export const updateText = async ({ payload, size }) => {
    try {
        const response = await fetchWithTimeout(`${API_URL}/openmatrix/text`, {
            method: 'POST',
            timeout: 2000,
            body: JSON.stringify({
                payload,
                size
            })
        });
        if (response.status === 200) {
            state.set({
                ...get(state),
                // @ts-ignore
                text: {
                    payload,
                    size
                }
            })
        }
    } catch (err) {
        console.error(err);
    }
}