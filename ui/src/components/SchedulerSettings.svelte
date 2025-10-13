<script>
  // @ts-nocheck
  import Toggle from "@/components/Toggle.svelte";
  import SettingsCard from "@/components/SettingsCard.svelte";
  import { state } from "@/store";
  import { get } from "svelte/store";

  export let initialValues = {};
  let loading = false;
  let dirty = false;
  let data = initialValues;

  const evaluate = () => {
    let isDirty = false;
    let frozenState = get(state);
    for (const [key, value] of Object.entries(data)) {
      try {
        if (value !== frozenState?.settings?.scheduler?.[key]) {
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
      const res = await fetch(`./openmatrix/settings/scheduler`, {
        method: "POST",
        body: JSON.stringify(data),
      });
      if (res.status === 200) {
        const st = get(state);
        state.set({
          ...st,
          settings: {
            ...st?.settings,
            scheduler: {
              ...st?.settings?.scheduler,
              ...data,
            },
          },
        });
      } else {
        alert("Failed to save Scheduler settings.");
      }
    } catch (err) {
      console.error(err);
      alert(err.message);
    } finally {
      loading = false;
    }
  };

  state.subscribe(() => evaluate());
</script>

<SettingsCard on:submit={submitForm} on:click={submitForm} name={"Auto Power (Dark)"} loading={loading} isDirty={dirty}>
  <div class="flex flex-row gap-x-3 justify-between" title="Automatically turn display off when ambient light falls below threshold; turn on after rising above threshold + hysteresis for a stable period.">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Enable
    </div>
    <div>
      <Toggle checked={data.enableDarkAutoPower} on:change={(v) => { data.enableDarkAutoPower = v.detail; evaluate(); }} />
    </div>
  </div>
  <div class="flex flex-row items-center gap-x-3 justify-between" title="Below this lux value device will consider it 'dark'.">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Dark threshold (lux)
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.darkThresholdLux} type="number" step="0.1" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.scheduler?.darkThresholdLux !== data.darkThresholdLux ? "!border-yellow-500" : "" }`}>
    </div>
  </div>
  <div class="flex flex-row items-center gap-x-3 justify-between" title="Amount of extra lux required above threshold before turning back on.">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Hysteresis (lux)
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.darkHysteresisLux} type="number" step="0.1" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.scheduler?.darkHysteresisLux !== data.darkHysteresisLux ? "!border-yellow-500" : "" }`}>
    </div>
  </div>
  <div class="flex flex-row items-center gap-x-3 justify-between" title="How long the condition must hold before toggling power.">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Stability (seconds)
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.darkStabilitySeconds} type="number" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.scheduler?.darkStabilitySeconds !== data.darkStabilitySeconds ? "!border-yellow-500" : "" }`}>
    </div>
  </div>
</SettingsCard>


