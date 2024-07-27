
<script>
  import Toggle from "@/components/Toggle.svelte";
  import SettingsCard from "@/components/SettingsCard.svelte";
</script>

<div class="mb-6">
  <div class="flex flex-wrap items-center gap-6 sm:flex-nowrap sm:justify-between">
    <h1 class="text-base text-lg font-semibold leading-7 text-zinc-900 dark:text-zinc-300">Settings</h1>
  </div>
</div>
<div class="grid grid-col-1 sm:grid-cols-2 gap-4">


  <SettingsCard name={"MQTT"}>
    <div class="flex flex-row gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Status
      </div>
      <div>
        <span class="bg-zinc-200/50 text-zinc-800 text-xs font-medium me-2 px-3 py-1 rounded-full dark:bg-zinc-700/50 dark:text-zinc-300">Disconnected</span>
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Broker Host
      </div>
      <div>
        <input type="text" class="bg-zinc-50 border border-zinc-300 text-zinc-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500">
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Port
      </div>
      <div>
        <input type="text" class="bg-zinc-50 border border-zinc-300 text-zinc-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500">
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Client ID
      </div>
      <div>
        <input type="text" class="bg-zinc-50 border border-zinc-300 text-zinc-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500">
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Username
      </div>
      <div>
        <input placeholder="(optional)" type="text" class="bg-zinc-50 border border-zinc-300 text-zinc-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500">
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Password
      </div>
      <div>
        <input placeholder="(optional)" type="text" class="bg-zinc-50 border border-zinc-300 text-zinc-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500">
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        CO2 Topic
      </div>
      <div>
        <input type="text" class="bg-zinc-50 border border-zinc-300 text-zinc-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500">
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Matrix Text Topic
      </div>
      <div>
        <input type="text" class="bg-zinc-50 border border-zinc-300 text-zinc-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500">
      </div>
    </div>
    <div class="flex flex-row gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Show Text
      </div>
      <div>
        <Toggle />
      </div>
    </div>
  </SettingsCard>

  <SettingsCard name={"eDMX"}>
    <div class="flex flex-row gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Status
      </div>
      <div>
        <span class="bg-zinc-200/50 text-zinc-800 text-xs font-medium me-2 px-3 py-1 rounded-full dark:bg-zinc-700/50 dark:text-zinc-300">Disconnected</span>
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Protocol
      </div>
      <div>
        <select id="protocol" value={0} class="bg-zinc-50 min-w-[140px] border border-zinc-300 text-zinc-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500">
          <option value={0}>sACN</option>
          <option value={1}>Art-Net</option>
        </select>
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Mode
      </div>
      <div>
        <select id="mode" value={0} class="bg-zinc-50 min-w-[140px] border border-zinc-300 text-zinc-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500">
          <option value={0}>RGB</option>
          <option value={1}>White</option>
        </select>
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Multicast
      </div>
      <div>
        <Toggle />
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Start Universe
      </div>
      <div>
        <Toggle />
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Start Address
      </div>
      <div>
        <input type="text" class="bg-zinc-50 border border-zinc-300 text-zinc-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500">
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Timeout
      </div>
      <div>
        <input type="text" class="bg-zinc-50 border border-zinc-300 text-zinc-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:ring-blue-500 dark:focus:border-blue-500">
      </div>
    </div>
  </SettingsCard>

  <SettingsCard name={"Home Assistant"} className={'col-span-2'}>
    <div class="flex flex-row gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Status
      </div>
      <div>
        <span class="bg-zinc-200/50 text-zinc-800 text-xs font-medium me-2 px-3 py-1 rounded-full dark:bg-zinc-700/50 dark:text-zinc-300">Disconnected</span>
      </div>
    </div>
    <div class="flex flex-row gap-x-3 justify-between">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Show Text
      </div>
      <div>
        <Toggle />
      </div>
    </div>
  </SettingsCard>

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
        <div class="flex flex-row items-center gap-x-2 mt-2 text-xs text-red-500">
          <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-4 h-4"><path d="m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3"/><path d="M12 9v4"/><path d="M12 17h.01"/></svg>
          Your device will be disconnected from current network.
        </div>
      </div>
      <div>
        <button class="text-white bg-red-600 hover:bg-red-700 focus:outline-none focus:ring-4 focus:ring-red-300 font-medium rounded-full text-sm px-5 py-2.5 text-center dark:bg-red-600 dark:hover:bg-red-700 dark:focus:ring-red-800">
          Reset
        </button>
      </div>
    </div>
  </div>

</div>