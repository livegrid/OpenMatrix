<script>
  import { onMount } from 'svelte';
  import { theme, state, togglePower, toggleAutoBrightness,updateBrightness } from '@/store';
  import Toggle from "./Toggle.svelte";

  let brightnessTimeout = null;

  const toggleTheme = () => {
    if ($theme === 'dark') {
      theme.set('light');
      document.documentElement.classList.remove('dark');
      // Set localstorage
      localStorage.setItem('theme', 'light');
    } else {
      theme.set('dark');
      document.documentElement.classList.add('dark');
      // Set localstorage
      localStorage.setItem('theme', 'dark');
    }
  }

  const updateBrightnessHandler = ({ target }) => {
    if (brightnessTimeout) {
      clearTimeout(brightnessTimeout);
    }

    brightnessTimeout = setTimeout(() => {
      updateBrightness(parseInt(target.value));
      brightnessTimeout = null;
    }, 200);
  }

  onMount(() => {
    // Get theme
    let th = localStorage.getItem('theme') || 'system';

    if (th === 'system') {
      if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
        th = 'dark';
      } else {
        th = 'light';
      }
    }

    if (th === 'dark') {
      theme.set('dark');
      document.documentElement.classList.add('dark');
    } else {
      theme.set('light');
      document.documentElement.classList.remove('dark');
    }
  });
</script>
<ul class="flex flex-col space-y-6 pb-6">
  <li class="flex justify-between space-x-3">
    <div class="flex flex-row items-center space-x-3 text-sm font-semibold leading-5 text-gray-600 dark:text-zinc-400">
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"  class="w-4 h-4"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></svg>
      <span>
        Dark UI
      </span>
    </div>
    <Toggle checked={$theme === 'dark'} on:change={toggleTheme} />
  </li>
  <li class="flex justify-between space-x-3">
    <div class="flex flex-row items-center space-x-3 text-sm font-semibold leading-5 text-gray-600 dark:text-zinc-400">
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4"><path d="M12 2v10"/><path d="M18.4 6.6a9 9 0 1 1-12.77.04"/></svg>
      <span>
        Power
      </span>
    </div>
    {#if typeof $state?.power !== 'boolean'}
      <div role="status" class="animate-pulse">
        <div class="h-4 bg-gray-300 rounded dark:bg-zinc-700 w-16 mt-2"></div>
      </div>
    {:else}
      <Toggle checked={$state.power} on:change={togglePower} />
    {/if}
  </li>
  <li class="flex justify-between space-x-3">
    <div class="flex flex-row items-center space-x-3 text-sm font-semibold leading-5 text-gray-600 dark:text-zinc-400">
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4"><path d="M12 8a2.83 2.83 0 0 0 4 4 4 4 0 1 1-4-4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.9 4.9 1.4 1.4"/><path d="m17.7 17.7 1.4 1.4"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.3 17.7-1.4 1.4"/><path d="m19.1 4.9-1.4 1.4"/></svg>
      <span>
        Auto Brightness
      </span>
    </div>
    {#if typeof $state?.autobrightness !== 'boolean'}
      <div role="status" class="animate-pulse">
        <div class="h-4 bg-gray-300 rounded dark:bg-zinc-700 w-16 mt-2"></div>
      </div>
    {:else}
      <Toggle checked={$state.autobrightness} on:change={toggleAutoBrightness} />
    {/if}
  </li>
  <li class="flex items-center space-x-3">
    <div class="text-gray-600 dark:text-zinc-400">
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></svg>
    </div>
    {#if typeof $state?.brightness !== 'number'}
      <div role="status" class="w-full animate-pulse">
        <div class="h-4 bg-gray-300 rounded dark:bg-zinc-700 w-full"></div>
      </div>
    {:else}
      <input id="steps-range" type="range" min="0" max="255" value={$state?.brightness || 0} on:change={updateBrightnessHandler} step="1" class="w-full h-2 bg-gray-200 dark:bg-zinc-800 rounded-lg appearance-none cursor-pointer">
    {/if}
  </li>
</ul>