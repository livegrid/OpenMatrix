<script>
  import SelectButton from "./SelectButton.svelte";
  export let title, image, subtitle, selected;
</script>

<div class="relative flex flex-row gap-x-6 overflow-hidden rounded-md border border-gray-200 dark:border-zinc-900 bg-white/80 dark:bg-black/30 px-4 py-5 sm:px-6 sm:py-6">
  <SelectButton selected={selected} />
  <!-- <button class="absolute top-12 right-4 text-gray-500 hover:text-gray-600 transition-colors">
    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-5 h-5"><path d="M20 7h-9"/><path d="M14 17H5"/><circle cx="17" cy="17" r="3"/><circle cx="7" cy="7" r="3"/></svg>
  </button> -->
  <div>
    <svelte:component this={image} />
  </div>
  <div>
    <!-- <h3 class="truncate text-sm font-medium text-gray-500">{title}</h3> -->
    <h5 class="truncate text-lg font-medium text-gray-800 dark:text-gray-300">{subtitle}</h5>
  </div>
</div>