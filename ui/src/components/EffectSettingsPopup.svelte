<script>
  import { createEventDispatcher } from "svelte";
  import { fade } from "svelte/transition";

  export let color = "#ffffff";
  export let speed = 50;
  export let scale = 50;
  export let effectType = "";

  // Additional settings for specific effects
  export let particleCount = 20; // For Flock
  export let complexity = 3; // For L-System effect

  const dispatch = createEventDispatcher();

  function handleColorChange(event) {
    dispatch("colorChange", event.target.value);
  }

  function handleSpeedChange(event) {
    dispatch("speedChange", parseInt(event.target.value));
  }

  function handleScaleChange(event) {
    dispatch("scaleChange", parseInt(event.target.value));
  }

  function handleParticleCountChange(event) {
    dispatch("particleCountChange", parseInt(event.target.value));
  }

  function handleComplexityChange(event) {
    dispatch("complexityChange", parseInt(event.target.value));
  }

  function closePopup() {
    dispatch("close");
  }
</script>

<div
  class="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50"
  on:click|self={closePopup}
  on:keydown={(e) => { if (e.key === 'Escape') { closePopup(); } }}
  tabindex="0"
  role="button"
  aria-label="Close settings popup"
  transition:fade
>
  <div class="bg-white rounded-md border border-zinc-200 dark:border-zinc-900 bg-white/90 dark:bg-black/90 p-6 w-80 shadow-xl">
    <h3 class="text-lg font-semibold mb-4 text-gray-900 dark:text-gray-100">
      Effect Settings
    </h3>

    <div class="mb-4">
      <label
        class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1"
        for="color-picker">Color</label
      >
      <input
        type="color"
        id="color-picker"
        bind:value={color}
        on:input={handleColorChange}
        class="w-full h-10 rounded cursor-pointer"
      />
    </div>

    <div class="mb-4">
      <label
        class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1"
        for="speed-slider">Speed</label
      >
      <input
        type="range"
        id="speed-slider"
        min="0"
        max="100"
        bind:value={speed}
        on:input={handleSpeedChange}
        class="w-full  h-2 bg-gray-200 dark:bg-zinc-800 rounded-lg appearance-none cursor-pointer"
      />
      <span class="text-sm text-gray-500 dark:text-gray-400">{speed}%</span>
    </div>

    <div class="mb-4">
      <label
        class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1"
        for="scale-slider">Scale</label
      >
      <input
        type="range"
        id="scale-slider"
        min="0"
        max="100"
        bind:value={scale}
        on:input={handleScaleChange}
        class="w-full  h-2 bg-gray-200 dark:bg-zinc-800 rounded-lg appearance-none cursor-pointer"
      />
      <span class="text-sm text-gray-500 dark:text-gray-400">{scale}%</span>
    </div>

    {#if effectType === "Flocking"}
      <div class="mb-4">
        <label
          class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1"
          for="particle-count-slider">Particle Count</label
        >
        <input
          type="range"
          id="particle-count-slider"
          min="10"
          max="500"
          bind:value={particleCount}
          on:input={handleParticleCountChange}
          class="w-full  h-2 bg-gray-200 dark:bg-zinc-800 rounded-lg appearance-none cursor-pointer"
          />
        <span class="text-sm text-gray-500 dark:text-gray-400"
          >{particleCount}</span
        >
      </div>
    {/if}

    {#if effectType === "L-System"}
      <div class="mb-4">
        <label
          class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1"
          for="complexity-slider">Complexity</label
        >
        <input
          type="range"
          id="complexity-slider"
          min="1"
          max="5"
          bind:value={complexity}
          on:input={handleComplexityChange}
          class="w-full  h-2 bg-gray-200 dark:bg-zinc-800 rounded-lg appearance-none cursor-pointer"
          />
        <span class="text-sm text-gray-500 dark:text-gray-400"
          >{complexity}</span
        >
      </div>
    {/if}

    <button
      on:click={closePopup}
      class="mt-4 w-full bg-blue-500 hover:bg-blue-600 text-white font-bold py-2 px-4 rounded"
    >
      Close
    </button>
  </div>
</div>
