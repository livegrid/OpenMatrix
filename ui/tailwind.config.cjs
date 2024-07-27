module.exports = {
  darkMode: 'selector',
  content: ['./src/**/*.{html,js,svelte,ts}', './index.html'],
  theme: {
    extend: {
      animation: {
        'spin-slow': 'spin 2s linear infinite',
      }
    }
  },
  plugins: [require('@tailwindcss/forms')],
}
