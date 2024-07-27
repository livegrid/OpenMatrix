<script>
    export let name, className;
</script>

<div class={`relative flex flex-col gap-y-6 overflow-hidden rounded-md border border-zinc-200 dark:border-zinc-900 bg-white/50 dark:bg-black/30 px-4 py-5 sm:px-6 sm:py-6 ${className}`}>
    <!-- <h3 class="text-base font-semibold leading-6 text-zinc-900 dark:text-zinc-300">Last 7 days</h3> -->
    <div class="flex flex-col gap-y-2">
      <div class="truncate text-lg font-medium text-zinc-800 mb-2 dark:text-zinc-300">
        { name }
      </div>
      <hr class="border-zinc-200 dark:border-zinc-900" />
    </div>
    <slot />
  </div>