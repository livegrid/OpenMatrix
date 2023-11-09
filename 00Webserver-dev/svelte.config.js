import preprocess from "svelte-preprocess";
// import adapter from '@sveltejs/adapter-auto';

// /** @type {import('@sveltejs/kit').Config} */
// const config = {
//     kit: {
// 		// adapter-auto only supports some environments, see https://kit.svelte.dev/docs/adapter-auto for a list.
// 		// If your environment is not supported or you settled on a specific environment, switch out the adapter.
// 		// See https://kit.svelte.dev/docs/adapters for more information about adapters.
// 		adapter: adapter()
// 	},

//     preprocess: [preprocess({
//         postcss: true
//     })]
// };

// export default config;

// --------------------

// import adapter from '@sveltejs/adapter-static';
 
// export default {
//   kit: {
//     adapter: adapter({
//       // default options are shown. On some platforms
//       // these options are set automatically — see below
//       pages: 'build',
//       assets: 'build',
//       fallback: null,
//       precompress: false,
//       strict: true
//     })
//   }
//     //   preprocess: [preprocess({
//     //       postcss: true
//     //   })]
// };


import adapter from '@sveltejs/adapter-static';

import {
    vitePreprocess
} from '@sveltejs/kit/vite';
 
/** @type {import('@sveltejs/kit').Config} */
const config = {
    preprocess: vitePreprocess(),
 
    kit: {
        adapter: adapter( {
            precompress: {
              brotli: false,
              gzip: true,
              files: ["htm", "html"]
            },
        }),
        paths: {
          base: process.env.NODE_ENV === 'production' ? '' : '',
        }
    }
};
 
export default config;