<script>
  import SelectButton from "./SelectButton.svelte";
  export let image, name, selected, loading;
</script>

<div class="relative flex flex-row gap-x-6 overflow-hidden rounded-md border border-gray-200 dark:border-zinc-900 bg-white/80 dark:bg-black/30 px-4 py-5 sm:px-6 sm:py-6 hover:shadow-2xl transition-shadow duration-300 ease-in-out hover:shadow-emerald-500/20 dark:hover:shadow-emerald-900/50">
  <SelectButton on:click loading={loading} selected={selected} />
  <div>
    <svelte:component this={image} />
  </div>
  <div>
    <h5 class="truncate text-lg font-medium text-gray-800 dark:text-gray-300">{name}</h5>
  </div>
</div>