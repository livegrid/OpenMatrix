<script>
  // @ts-nocheck
  import Toggle from "@/components/Toggle.svelte";
  import SettingsCard from "@/components/SettingsCard.svelte";
  import { state, updateMQTTSettings } from "@/store";
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
        console.log(key, value, frozenState?.settings?.mqtt[key]);
        if (value !== frozenState?.settings?.mqtt[key]) {
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
      const req = await updateMQTTSettings(data);
      if (req.status === 200) {
        const st = get(state);
        state.set({
          ...st,
          settings: {
            ...st?.settings,
            mqtt: {
              ...st?.settings?.mqtt,
              ...data
            },
          }
        });
      } else {
        alert("Failed to save MQTT settings.");
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

<SettingsCard on:submit={submitForm} on:click={submitForm} name={"MQTT"} loading={loading} isDirty={dirty}>
  <div class="flex flex-row gap-x-3 justify-between">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Status
    </div>
    <div>
      <StatusBadge value={get(state)?.settings?.mqtt?.status} />
    </div>
  </div>
  <div class="flex flex-row items-center gap-x-3 justify-between">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Broker Host
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.host} type="text" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.mqtt?.host !== data.host ? "!border-yellow-500" : "" }`}>
    </div>
  </div>
  <div class="flex flex-row items-center gap-x-3 justify-between">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Port
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.port} type="number" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.mqtt?.port !== data.port ? "!border-yellow-500" : "" }`}>
    </div>
  </div>
  <div class="flex flex-row items-center gap-x-3 justify-between">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Client ID
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.client_id} type="text" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.mqtt?.client_id !== data.client_id ? "!border-yellow-500" : "" }`}>
    </div>
  </div>
  <div class="flex flex-row items-center gap-x-3 justify-between">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Username
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.username} placeholder="(optional)" type="text" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.mqtt?.username !== data.username ? "!border-yellow-500" : "" }`}>
    </div>
  </div>
  <div class="flex flex-row items-center gap-x-3 justify-between">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Password
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.password} placeholder="(optional)" type="text" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.mqtt?.password !== data.password ? "!border-yellow-500" : "" }`}>
    </div>
  </div>
  <div class="flex flex-row items-center gap-x-3 justify-between">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      CO2 Topic
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.co2_topic} type="text" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.mqtt?.co2_topic !== data.co2_topic ? "!border-yellow-500" : "" }`}>
    </div>
  </div>
  <div class="flex flex-row items-center gap-x-3 justify-between">
    <div class="truncate text-sm font-medium text-zinc-800 dark:text-zinc-300">
      Matrix Text Topic
    </div>
    <div>
      <input on:keyup={evaluate} bind:value={data.matrix_text_topic} type="text" class={`bg-zinc-50 border border-zinc-200 text-zinc-900 text-sm rounded-lg focus:border-blue-500 block w-full px-2.5 py-1.5 dark:bg-zinc-900 dark:border-zinc-800 dark:placeholder-zinc-400 dark:text-white dark:focus:border-blue-500 ${ $state?.settings?.mqtt?.matrix_text_topic !== data.matrix_text_topic ? "!border-yellow-500" : "" }`}>
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