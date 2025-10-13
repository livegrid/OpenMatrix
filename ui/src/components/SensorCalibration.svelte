<script>
  // @ts-nocheck
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
        if (value !== frozenState?.settings?.calibration?.[key]) {
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
      const res = await fetch(`./openmatrix/settings/calibration`, {
        method: "POST",
        body: JSON.stringify(data),
      });
      if (res.status === 200) {
        const st = get(state);
        state.set({
          ...st,
          settings: {
            ...st?.settings,
            calibration: {
              ...st?.settings?.calibration,
              ...data,
            },
          },
        });
      } else {
        alert("Failed to save Calibration settings.");
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

<SettingsCard on:submit={submitForm} on:click={submitForm} name={"Sensor Calibration"} loading={loading} isDirty={dirty}>
  <div class="flex flex-row items-center gap-x-3 justify-between" title="Adjust temperature in °C to match a reference sensor.">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Temperature Offset (°C)
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.temperatureOffsetC} type="number" step="0.1" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500`}>
    </div>
  </div>
  <div class="flex flex-row items-center gap-x-3 justify-between" title="Adjust humidity (%) to match a reference sensor.">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Humidity Offset (%)
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.humidityOffsetPct} type="number" step="0.1" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500`}>
    </div>
  </div>
  <div class="flex flex-row items-center gap-x-3 justify-between" title="Adjust CO₂ (ppm) to match a reference sensor.">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      CO₂ Offset (ppm)
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.co2OffsetPpm} type="number" step="1" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500`}>
    </div>
  </div>
</SettingsCard>


