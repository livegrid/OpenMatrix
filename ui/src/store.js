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
    return response;
}

export const updateBrightness = async (value) => {
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
    return response;
}

export const selectMode = async (modeId) => {
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
    return response;
}

export const selectEffect = async (effect) => {
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
    return response;
}

export const selectImage = async ({ name }) => {
    const response = await fetchWithTimeout(`${API_URL}/openmatrix/image`, {
        method: 'POST',
        timeout: 2000,
        body: JSON.stringify({
            name
        })
    });
    if (response.status === 200) {
        state.set({
            ...get(state),
            // @ts-ignore
            image: {
                selected: name
            }
        })
    }
    return response;
}

export const invokePreview = async ({ name }) => {
    const response = await fetchWithTimeout(`${API_URL}/openmatrix/image`, {
        method: 'PATCH',
        timeout: 2000,
        body: JSON.stringify({
            name
        })
    });
    return response;
}

export const updateText = async ({ payload, size }) => {
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
    return response;
}

export const updateMQTTSettings = async (settings) => {
    const response = await fetchWithTimeout(`${API_URL}/openmatrix/settings/mqtt`, {
        method: 'POST',
        timeout: 2000,
        body: JSON.stringify(settings)
    });
    if (response.status === 200) {
        state.set({
            ...get(state),
            settings: {
                ...get(state)?.settings,
                mqtt: settings
            }
        })
    }
    return response;
}

export const updateDMXSettings = async (settings) => {
    const response = await fetchWithTimeout(`${API_URL}/openmatrix/settings/edmx`, {
        method: 'POST',
        timeout: 2000,
        body: JSON.stringify(settings)
    });
    if (response.status === 200) {
        state.set({
            ...get(state),
            settings: {
                ...get(state)?.settings,
                edmx: settings
            }
        })
    }
    return response;
}

export const updateHASSSettings = async (settings) => {
    const response = await fetchWithTimeout(`${API_URL}/openmatrix/settings/hass`, {
        method: 'POST',
        timeout: 2000,
        body: JSON.stringify(settings)
    });
    if (response.status === 200) {
        state.set({
            ...get(state),
            settings: {
                ...get(state)?.settings,
                hass: settings
            }
        })
    }
    return response;
}

export const resetNetwork = async () => {
    const response = await fetchWithTimeout(`${API_URL}/openmatrix/settings/network/reset`, {
        method: 'POST',
        timeout: 2000
    });
    return response;
}
