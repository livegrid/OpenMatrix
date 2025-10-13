<script>
    // @ts-nocheck
    import Toggle from "@/components/Toggle.svelte";
    import SettingsCard from "@/components/SettingsCard.svelte";
    import { state, updateDMXSettings } from "@/store";
    import { onDestroy, onMount } from "svelte";
    import { get } from "svelte/store";
    
    export let initialValues = {};
    let loading = false;
    let dirty = false;
    let data = initialValues;
    
    const evaluate = () => {
      // dirty = JSON.stringify(get(state)?.settings?.mqtt) !== JSON.stringify(data);
      // Evaluate data one by one 
      let isDirty = false;
      let frozenState = get(state);
      for (const [key, value] of Object.entries(data)) {
        try {
          console.log(key, value, frozenState?.settings?.edmx[key]);
          if (value !== frozenState?.settings?.edmx[key]) {
            isDirty = true;
            break;
          }
        } catch (err) {
          console.error(err);
        }
      }
      dirty = isDirty;
    };
  
    const submitForm = async (e) => {
      e.preventDefault();
      try {
        loading = true;
        const req = await updateDMXSettings(data);
        if (req.status === 200) {
          const st = get(state);
          state.set({
            ...st,
            settings: {
              ...st?.settings,
              edmx: {
                ...st?.settings?.edmx,
                ...data
              },
            }
          });
        } else {
          alert("Failed to save eDMX settings.");
        }
      } catch (err) {
        console.error(err);
        alert(err.message);
      } finally {
        loading = false;
      }
    }
  
    const unsubscribe = state.subscribe((value) => {
      evaluate();
    });
  
    onDestroy(() => {
      unsubscribe();
    });
  </script>
  
  <SettingsCard on:submit={submitForm} on:click={submitForm} name={"eDMX"} loading={loading} isDirty={dirty}>
    <div class="flex flex-row items-center gap-x-3 justify-between" title="Select eDMX protocol: sACN (E1.31) or Art-Net.">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Protocol
      </div>
      <div>
        <select on:keyup={evaluate} bind:value={data.protocol} id="protocol" class={`min-w-[100px] bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.edmx?.protocol !== data.protocol ? "!border-yellow-500" : "" }`}>
          <option value={0}>sACN</option>
          <option value={1}>Art-Net</option>
        </select>
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between" title="RGB maps 3 channels per pixel; White uses single-channel intensity.">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Mode
      </div>
      <div>
        <select on:change={evaluate} bind:value={data.mode} id="mode" class={`min-w-[100px] bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.edmx?.mode !== data.mode ? "!border-yellow-500" : "" }`}>
          <option value={0}>RGB</option>
          <option value={1}>White</option>
        </select>
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between" title="Enable multicast (sACN). Disable for unicast.">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Multicast
      </div>
      <div>
        <Toggle checked={data.multicast} on:change={(v) => { data.multicast = v.detail; evaluate(); }} />
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between" title="First DMX universe number to listen on.">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Start Universe
      </div>
      <div>
        <input on:keyup={evaluate} bind:value={data.start_universe} type="number" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.edmx?.start_universe !== data.start_universe ? "!border-yellow-500" : "" }`}>
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between" title="Start channel address within the universe.">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Start Address
      </div>
      <div>
        <input on:keyup={evaluate} bind:value={data.start_address} type="number" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.edmx?.start_address !== data.start_address ? "!border-yellow-500" : "" }`}>
      </div>
    </div>
    <div class="flex flex-row items-center gap-x-3 justify-between" title="Data timeout before DMX mode releases control (ms).">
      <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
        Timeout
      </div>
      <div>
        <input on:keyup={evaluate} bind:value={data.timeout} type="text" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.edmx?.timeout !== data.timeout ? "!border-yellow-500" : "" }`}>
      </div>
    </div>
  </SettingsCard>