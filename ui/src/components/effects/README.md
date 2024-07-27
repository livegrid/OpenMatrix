## SVG to Svelte Components

To add more effects, you first need to optimize your SVG file and convert it to a svelte component. Here are the steps for doing that:

1. Go to https://svgomg.net
2. Select your SVG file
3. Select all options except "Remove xlmns", "Prettify markup" and "Show original".
4. Click the copy button which is displated in bottom right corner of the screen.
5. Create another `<name>.svelte` file in `effects` directory.
6. Add class attribute to our newly created component: `class="w-24 h-24 rounded-lg"`.
7. Done. Now you can include it in effects page!