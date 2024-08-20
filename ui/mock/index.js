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
    },
    image: {
        selected: 1,
    },
    text: {
        payload: "Hello! 12345",
        size: 0,
    }
};

const images = [
    {
        id: 1,
        name: 'Image 1',
        size: 200,
    },
    {
        id: 2,
        name: 'Image 2.gif',
        size: 200,
    },
    {
        id: 3,
        name: 'Image 3',
        size: 200,
    },
    {
        id: 4,
        name: 'Image 4',
        size: 200,
    },
    {
        id: 5,
        name: 'Image 5',
        size: 200,
    },
];

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
    },
    {
        url: '/openmatrix/image',
        method: 'get',
        timeout: 1000,
        response: () => {
            return images;
        }
    },
    {
        url: '/openmatrix/image',
        method: 'post',
        timeout: 1000,
        response: async ({ body }) => {
            state.image.selected = body.id;
            return {
                code: 200,
                data: {
                    message: 'OK'
                }
            }
        }  
    },
    {
        url: '/openmatrix/image',
        method: 'patch',
        timeout: 1000,
        response: async (context) => {
            return {
                code: 200,
                data: {
                    message: 'OK'
                }
            }
        }
    },
    {
        url: '/openmatrix/text',
        method: 'post',
        timeout: 1000,
        response: async ({ body }) => {
            state.text.payload = body.payload;
            state.text.size = body.size;

            return {
                code: 200,
                data: {
                    message: 'OK'
                }
            };
        }
    }
];