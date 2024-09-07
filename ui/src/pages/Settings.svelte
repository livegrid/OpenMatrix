
<script>
  // @ts-nocheck
  import Toggle from "@/components/Toggle.svelte";
  import SettingsCard from "@/components/SettingsCard.svelte";
  import MQTTSettings from "@/components/MQTTSettings.svelte";
  import DMXSettings from "@/components/DMXSettings.svelte";
  import { onDestroy, onMount } from "svelte";
  import { get } from "svelte/store";
  import { state, resetNetwork, resetFactory } from "@/store";
  import HassSettings from "@/components/HassSettings.svelte";

  let loading = true;
  let networkResetLoading = false;
  let factoryResetLoading = false;
  let initialValues = null;

  const unsubscribe = state.subscribe((value) => {
    if (loading && value?.settings) {
      initialValues = value?.settings;
      loading = false;
    }
  });

  const networkReset = async () => {
    if (confirm('Please confirm if you would like to reset network settings')) {
      try {
        networkResetLoading = true;
        const response = await resetNetwork();
        if (response.status === 200) {
          alert("Network reset successful. Please connect to the device access point \"LiveGrid\" to configure it.");
        } else {
          alert("Failed to reset network! Please try again.");
        }
      } catch (err) {
        console.error(err);
        alert("Failed to reset network! Please try again.");
      } finally {
        networkResetLoading = false;
      }
    }
  }

  const factoryReset = async () => {
    if (confirm('Please confirm if you would like to factory reset your device. All settings will be erased.')) {
      try {
        factoryResetLoading = true;
        const response = await resetFactory();
        if (response.status === 200) {
          alert("Factory reset successful. Please connect to the device access point \"LiveGrid\" to configure it.");
        } else {
          alert("Failed to factory reset! Please try again.");
        }
      } catch (err) {
        console.error(err);
        alert("Failed to factory reset! Please try again.");
      } finally {
        factoryResetLoading = false;
      }
    }
  }

  onMount(() => {
    if (get(state)?.settings) {      
      initialValues = get(state)?.settings;
      loading = false;
    }
  });

  onDestroy(() => {
    unsubscribe();
  });
</script>

<div class="mb-6">
  <div class="flex flex-wrap items-center gap-6 sm:flex-nowrap sm:justify-between">
    <h1 class="text-base text-lg font-semibold leading-7 text-zinc-900 dark:text-zinc-300">Settings</h1>
  </div>
</div>
{#if loading}
  <div class="grid grid-col-1 sm:grid-cols-2 gap-4">
    <div role="status" class="animate-pulse">
      <div class="bg-gray-300/50 rounded dark:bg-zinc-700/50 w-full h-[600px]"></div>
    </div>
    <div role="status" class="animate-pulse">
      <div class="bg-gray-300/50 rounded dark:bg-zinc-700/50 w-full h-[600px]"></div>
    </div>
  </div>
{:else}
  <div class="grid grid-col-1 sm:grid-cols-2 gap-4">
    <MQTTSettings initialValues={initialValues?.mqtt} />
    <DMXSettings initialValues={initialValues?.edmx} />
    <HassSettings initialValues={initialValues?.hass} />

    <div class="relative flex flex-col gap-y-6 overflow-hidden col-span-2 rounded-md border border-zinc-200 dark:border-zinc-900 bg-white/30 dark:bg-black/30 px-4 py-3 sm:px-6 sm:py-6">
      <!-- <h3 class="text-base font-semibold leading-6 text-zinc-900 dark:text-zinc-300">Last 7 days</h3> -->
      <div class="flex flex-row items-center justify-between gap-y-2">
        <div class="truncate text-lg font-medium text-zinc-800 dark:text-zinc-300">
          Firmware Update
        </div>
        <div>
          <a href="/update" class="text-white bg-emerald-600 hover:bg-emerald-700 focus:outline-none focus:ring-4 focus:ring-emerald-300 font-medium rounded-full text-sm px-5 py-2.5 text-center dark:bg-emerald-600 dark:hover:bg-emerald-700 dark:focus:ring-emerald-800">
            Go to Updater 
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4 inline-block self-center mb-0.5 ms-1"><path d="M15 3h6v6"/><path d="M10 14 21 3"/><path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"/></svg>
          </a>
        </div>
      </div>
    </div>

    <div class="relative flex flex-col gap-y-6 overflow-hidden col-span-2 rounded-md border border-zinc-200 dark:border-zinc-900 bg-white/30 dark:bg-black/30 px-4 py-3 sm:px-6 sm:py-6">
      <!-- <h3 class="text-base font-semibold leading-6 text-zinc-900 dark:text-zinc-300">Last 7 days</h3> -->
      <div class="flex flex-row items-center justify-between gap-y-2">
        <div class="truncate text-lg font-medium text-zinc-800 dark:text-zinc-300">
          Reset Network Settings
          <div class="flex flex-row items-center gap-x-2 mt-2 text-xs text-red-600">
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-3 h-3"><path d="m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3"/><path d="M12 9v4"/><path d="M12 17h.01"/></svg>
            Your device will be disconnected from current network.
          </div>
        </div>
        <div>
          <button on:click={networkReset} disabled={networkResetLoading} class="text-white bg-red-600 hover:bg-red-700 disabled:opacity-50 focus:outline-none focus:ring-4 focus:ring-red-300 font-medium rounded-full text-sm px-5 py-2.5 text-center dark:bg-red-600 dark:hover:bg-red-700 dark:focus:ring-red-800">
            {#if networkResetLoading}
              Resetting...
            {:else}
              Reset
            {/if}
          </button>
        </div>
      </div>
    </div>

    <div class="relative flex flex-col gap-y-6 overflow-hidden col-span-2 rounded-md border border-zinc-200 dark:border-zinc-900 bg-white/30 dark:bg-black/30 px-4 py-3 sm:px-6 sm:py-6">
      <div class="flex flex-row items-center justify-between gap-y-2">
        <div class="truncate text-lg font-medium text-zinc-800 dark:text-zinc-300">
          Factory Reset
          <div class="flex flex-row items-center gap-x-2 mt-2 text-xs text-red-600">
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-3 h-3"><path d="m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3"/><path d="M12 9v4"/><path d="M12 17h.01"/></svg>
            All existing settings and network configuration will be lost on a factory reset.
          </div>
        </div>
        <div>
          <button on:click={factoryReset} disabled={factoryResetLoading} class="text-white bg-red-600 hover:bg-red-700 disabled:opacity-50 focus:outline-none focus:ring-4 focus:ring-red-300 font-medium rounded-full text-sm px-5 py-2.5 text-center dark:bg-red-600 dark:hover:bg-red-700 dark:focus:ring-red-800">
            {#if factoryResetLoading}
              Resetting...
            {:else}
              Reset
            {/if}
          </button>
        </div>
      </div>
    </div>
  </div>
{/if}