const state = {
    power: false,
    brightness: 100,
    mode: 0,
    environment: {
        temperature: {
            value: 0,
            history_24h: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            diff: {
                type: 0,
                value: 0,
                inverse: false
            }
        },
        humidity: {
            value: 0,
            history_24h: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            diff: {
                type: 0,
                value: 0,
                inverse: false
            }
        },
        co2: {
            value: 0,
            history_24h: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            diff: {
                type: 0,
                value: 0,
                inverse: false
            }
        }
    },
    effects: {
        selected: 1
    }
};

const wait = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

export default [
    {
        url: '/openmatrix/state',
        method: 'get',
        response: () => {
            return {
                ...state
            }
        }
    },
    {
        url: '/openmatrix/power',
        method: 'post',
        response: ({ body }) => {
            state.power = body.power;
            return {
                code: 200,
                data: {
                    message: 'OK'
                }
            }
        }
    },
    {
        url: '/openmatrix/brightness',
        method: 'post',
        response: ({ body }) => {
            state.brightness = body.brightness;
            return {
                code: 200,
                data: {
                    message: 'OK'
                }
            }
        }
    },
    {
        url: '/openmatrix/mode',
        method: 'post',
        timeout: 1000,
        response: ({ body }) => {
            state.mode = body.mode;
            return {
                code: 200,
                data: {
                    message: 'OK'
                }
            }
        }
    },
    {
        url: '/openmatrix/effect',
        method: 'post',
        timeout: 1000,
        response: async ({ body }) => {
            state.effects.selected = body.effect;
            return {
                code: 200,
                data: {
                    message: 'OK'
                }
            }
        }
    }
];