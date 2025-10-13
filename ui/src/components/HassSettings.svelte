<script>
  // @ts-nocheck
  import Toggle from "@/components/Toggle.svelte";
  import SettingsCard from "@/components/SettingsCard.svelte";
  import { state, updateHASSSettings } from "@/store";
  import { onDestroy, onMount } from "svelte";
  import { get } from "svelte/store";
    import StatusBadge from "./StatusBadge.svelte";
  
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
        console.log(key, value, frozenState?.settings?.hass[key]);
        if (value !== frozenState?.settings?.hass[key]) {
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
      const req = await updateHASSSettings(data);
      if (req.status === 200) {
        const st = get(state);
        state.set({
          ...st,
          settings: {
            ...st?.settings,
            hass: {
              ...st?.settings?.hass,
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

<SettingsCard on:submit={submitForm} on:click={submitForm} name={"Home Assistant"} className={'col-span-2'} loading={loading} isDirty={dirty}>
  <div class="flex flex-row gap-x-3 justify-between" title="Show text overlay when using Home Assistant text control.">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Status
    </div>
    <div>
      <StatusBadge value={get(state)?.settings?.hass?.status} />
    </div>
  </div>
  <div class="flex flex-row gap-x-3 justify-between">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Show Text
    </div>
    <div>
      <Toggle checked={data.show_text} on:change={(v) => { data.show_text = v.detail; evaluate(); }} />
    </div>
  </div>
</SettingsCard>