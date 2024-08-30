<script>
    import Button from "./Button.svelte";

    export let name, loading = false, className = '', isDirty = false;
</script>

<form on:submit class={`relative flex flex-col gap-y-6 overflow-hidden rounded-md border border-zinc-200 dark:border-zinc-900 bg-white/50 dark:bg-black/30 px-4 py-5 sm:px-6 sm:py-6 ${className}`}>
  <!-- <h3 class="text-base font-semibold leading-6 text-zinc-900 dark:text-zinc-300">Last 7 days</h3> -->
  <div class="flex flex-col gap-y-2">
    <div class="flex flex-row justify-between mb-2">
      <div class="truncate text-lg font-medium text-zinc-800 dark:text-zinc-300">
        { name }
      </div>
      {#if isDirty}
        <Button loading={loading} on:click className="w-16 justify-center bg-green-50 hover:!bg-emerald-100/50 text-emerald-600 !border-emerald-500 !dark:border-emerald-500 !text-emerald-600">Save</Button>
      {/if}
    </div>
    <hr class="border-zinc-200 dark:border-zinc-900" />
  </div>
  <slot />
</form>