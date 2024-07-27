<script>
  import dummyImage from '@/assets/img/dummy.jpg';
  import SelectButton from './SelectButton.svelte';
  export let title, subtitle, selected;


</script>

<div class="relative flex flex-col gap-y-6 overflow-hidden rounded-md border border-gray-200 dark:border-zinc-900 bg-white/80 dark:bg-black/30 px-4 py-5 sm:px-6 sm:py-8">
  <div>
    <img src={dummyImage} alt="icon" class="w-full h-auto rounded-md aspect-square" />
  </div>
  <div>
    <h5 class="truncate text-lg font-medium text-gray-800 dark:text-gray-300">{subtitle}</h5>
  </div>
  <SelectButton selected={selected} />
</div>