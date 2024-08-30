<script>
    import { createEventDispatcher } from 'svelte';
    import { fade } from 'svelte/transition';
  
    export let color = '#ffffff';
    export let speed = 50;
    export let scale = 50;
  
    const dispatch = createEventDispatcher();
  
    function handleColorChange(event) {
      dispatch('colorChange', event.target.value);
    }
  
    function handleSpeedChange(event) {
      dispatch('speedChange', parseInt(event.target.value));
    }
  
    function handleScaleChange(event) {
      dispatch('scaleChange', parseInt(event.target.value));
    }
  
    function closePopup() {
      dispatch('close');
    }
  </script>
  
  <div class="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50" on:click|self={closePopup} transition:fade>
    <div class="bg-white dark:bg-gray-800 rounded-lg p-6 w-80 shadow-xl">
      <h3 class="text-lg font-semibold mb-4 text-gray-900 dark:text-gray-100">Effect Settings</h3>
      
      <div class="mb-4">
        <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1" for="color-picker">Color</label>
        <input 
          type="color" 
          id="color-picker" 
          bind:value={color} 
          on:input={handleColorChange}
          class="w-full h-10 rounded cursor-pointer"
        />
      </div>
      
      <div class="mb-4">
        <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1" for="speed-slider">Speed</label>
        <input 
          type="range" 
          id="speed-slider" 
          min="0" 
          max="100" 
          bind:value={speed} 
          on:input={handleSpeedChange}
          class="w-full"
        />
        <span class="text-sm text-gray-500 dark:text-gray-400">{speed}%</span>
      </div>
      
      <div class="mb-4">
        <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1" for="scale-slider">Scale</label>
        <input 
          type="range" 
          id="scale-slider" 
          min="0" 
          max="100" 
          bind:value={scale} 
          on:input={handleScaleChange}
          class="w-full"
        />
        <span class="text-sm text-gray-500 dark:text-gray-400">{scale}%</span>
      </div>
      
      <button 
        on:click={closePopup}
        class="mt-4 w-full bg-blue-500 hover:bg-blue-600 text-white font-bold py-2 px-4 rounded"
      >
        Close
      </button>
    </div>
  </div>